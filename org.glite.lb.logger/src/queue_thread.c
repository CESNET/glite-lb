#ident "$Header$"

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>

#include "interlogd.h"

static 
void 
queue_thread_cleanup(void *q)
{
	struct event_queue *eq = (struct event_queue *)q;

	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "thread %d exits", eq->thread_id);

	/* unlock all held locks */
	/* FIXME: check that the thread always exits when holding these locks;
	   unlock them at appropriate places if this condition is not met  
	   event_queue_unlock(eq);
	   event_queue_cond_unlock(eq);
	*/
  
	/* clear thread id */
	eq->thread_id = 0;
}


static time_t now;

static
int
cmp_expires(struct server_msg *msg, void *data)
{
  time_t *t = (time_t*)data;
  return (msg->expires > 0) && (msg->expires < *t);
}

static
void *
queue_thread(void *q)
{
	struct event_queue *eq = (struct event_queue *)q;
	int ret, exit;
	int retrycnt;
	int close_timeout = 0;
	int exit_timeout = EXIT_TIMEOUT;

	if(init_errors(0) < 0) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, 
				 "Error initializing thread specific data, exiting!");
		pthread_exit(NULL);
	}
  
	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, 
			 "  started new thread for delivery to %s:%d", 
			 eq->dest_name, eq->dest_port);

	pthread_cleanup_push(queue_thread_cleanup, q); 

	event_queue_cond_lock(eq);

	exit = 0;
	retrycnt = 0;
	while(!exit) {
    
		clear_error();

		/* if there are no events, wait for them */
		ret = 0;
		while (event_queue_empty(eq) 
#if defined(INTERLOGD_HANDLE_CMD) && defined(INTERLOGD_FLUSH)
		       && (eq->flushing != 1)
#endif
			) {
			if(lazy_close && close_timeout) {
				ret = event_queue_wait(eq, close_timeout);
				if(ret == 1) {/* timeout? */
					event_queue_close(eq);
					glite_common_log(LOG_CATEGORY_LB_IL, LOG_PRIORITY_DEBUG, 
							 "  connection to %s:%d closed",
							 eq->dest_name, eq->dest_port);
				}
				close_timeout = 0;
			} else {
				ret = event_queue_wait(eq, exit_timeout);
				if(ret == 1) {
					glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, 
							 "  thread idle for more than %d seconds, exiting", 
							 exit_timeout);
					event_queue_close(eq);
					event_queue_cond_unlock(eq);
					pthread_exit((void*)0);
				}
			}
			if(ret < 0) {
				/* error waiting */
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, 
						 "queue_thread: %s", 
						 error_get_msg());
				event_queue_cond_unlock(eq);
				pthread_exit((void*)-1);
			}
		}  /* END while(empty) */
    

		/* allow other threads to signal us, ie. insert new events while
		 * we are sending or request flush operation
		 */
		event_queue_cond_unlock(eq);
		
		/* discard expired events */
		glite_common_log(LOG_CATEGORY_LB_IL, LOG_PRIORITY_DEBUG, "  discarding expired events");
		now = time(NULL);
		event_queue_move_events(eq, NULL, cmp_expires, &now);
		if(!event_queue_empty(eq)) {

			/* deliver pending events */
			glite_common_log(LOG_CATEGORY_LB_IL, LOG_PRIORITY_DEBUG, 
					 "  attempting delivery to %s:%d", 
					 eq->dest_name, eq->dest_port);
			/* connect to server */
			if((ret=event_queue_connect(eq)) == 0) {
				/* not connected */
				if(error_get_maj() != IL_OK)
					glite_common_log(LOG_CATEGORY_LB_IL, LOG_PRIORITY_WARN, 
							 "queue_thread: %s", error_get_msg());
#if defined(IL_NOTIFICATIONS)
				glite_common_log(LOG_CATEGORY_LB_IL, LOG_PRIORITY_INFO, 
						 "    could not connect to client %s, waiting for retry", 
						 eq->dest_name);
#else
				glite_common_log(LOG_CATEGORY_LB_IL, LOG_PRIORITY_INFO, 
						 "    could not connect to bookkeeping server %s, waiting for retry", 
						 eq->dest_name);
#endif
				retrycnt++;
			} else {
				retrycnt = 0;
				/* connected, send events */
				switch(ret=event_queue_send(eq)) {
					
				case 0:
					/* there was an error and we still have events to send */
					if(error_get_maj() != IL_OK)
						glite_common_log(LOG_CATEGORY_LB_IL, LOG_PRIORITY_WARN, 
								 "queue_thread: %s", 
								 error_get_msg());
					glite_common_log(LOG_CATEGORY_LB_IL, LOG_PRIORITY_DEBUG, 
							 "  events still waiting");
					break;
					
				case 1:
					/* hey, we are done for now */
					glite_common_log(LOG_CATEGORY_LB_IL, LOG_PRIORITY_DEBUG, 
							 "  all events for %s sent", 
							 eq->dest_name);
					break;
					
				default:
					/* internal error */
					glite_common_log(LOG_CATEGORY_LB_IL, LOG_PRIORITY_ERROR, 
							 "queue_thread: %s", 
							 error_get_msg());
					exit = 1;      
					break;
					
				} /* switch */
			
				/* we are done for now anyway, so close the queue */
				if((ret == 1) && lazy_close)
					close_timeout = default_close_timeout;
				else {
					event_queue_close(eq);
					glite_common_log(LOG_CATEGORY_LB_IL, LOG_PRIORITY_DEBUG,
							 "  connection to %s:%d closed",
							 eq->dest_name, eq->dest_port);
				}
			}
		} 

#if defined(INTERLOGD_HANDLE_CMD) && defined(INTERLOGD_FLUSH)
		if(pthread_mutex_lock(&flush_lock) < 0)
			abort();
		event_queue_cond_lock(eq);

		/* Check if we are flushing and if we are, report status to master */
		if(eq->flushing == 1) {
			glite_common_log(LOG_CATEGORY_LB_IL, LOG_PRIORITY_DEBUG, 
					 "    flushing mode detected, reporting status");
			/* 0 - events waiting, 1 - events sent, < 0 - some error */
			eq->flush_result = ret;
			eq->flushing = 2;
			if(pthread_cond_signal(&flush_cond) < 0)
				abort();
		}
		if(pthread_mutex_unlock(&flush_lock) < 0)
			abort();
#else
#endif

		/* if there was some error with server, sleep for a while */
		/* iff !event_queue_empty() */
		/* also allow for one more try immediately after server disconnect,
		   which may cure server kicking us out after given number of connections */
#ifndef LB_PERF
		if((ret == 0) && (retrycnt > 0)) {
			glite_common_log(LOG_CATEGORY_LB_IL, LOG_PRIORITY_DEBUG, 
					 "    sleeping");
			event_queue_sleep(eq);
		}
#endif

#if defined(INTERLOGD_HANDLE_CMD) && defined(INTERLOGD_FLUSH)
#else
		event_queue_cond_lock(eq);
#endif

		if(exit) {
			/* we have to clean up before exiting */
			event_queue_cond_unlock(eq);
		}
    
	} /* while */

	pthread_cleanup_pop(1);

	return(eq);
}


int
event_queue_create_thread(struct event_queue *eq)
{
	assert(eq != NULL);

	event_queue_lock(eq);

	/* if there is a thread already, just return */
	if(eq->thread_id != 0) {
		event_queue_unlock(eq);
		return(0);
	}

	/* create the thread itself */
	if(pthread_create(&eq->thread_id, NULL, queue_thread, eq) < 0) {
		eq->thread_id = 0;
		set_error(IL_SYS, errno, "event_queue_create_thread: error creating new thread");
		event_queue_unlock(eq);
		return(-1);
	}

	/* the thread is never going to be joined */
	pthread_detach(eq->thread_id);
	
	event_queue_unlock(eq);

	return(1);
}



int
event_queue_lock(struct event_queue *eq)
{
	assert(eq != NULL);

	if(pthread_rwlock_wrlock(&eq->update_lock)) {
		/*** abort instead, this is too serious
		set_error(IL_SYS, errno, "event_queue_lock: error acquiring write lock");
		return(-1);
		*/
		abort();
	}

	return(0);
}


int
event_queue_lock_ro(struct event_queue *eq)
{
	assert(eq != NULL);

	if(pthread_rwlock_rdlock(&eq->update_lock)) {
		/*** abort instead, this is too serious
		set_error(IL_SYS, errno, "event_queue_lock_ro: error acquiring read lock");
		return(-1);
		*/
		abort();
	}

	return(0);
}


int
event_queue_unlock(struct event_queue *eq)
{
	assert(eq != NULL);

	if(pthread_rwlock_unlock(&eq->update_lock)) {
		/*** abort instead, this is too serious
		set_error(IL_SYS, errno, "event_queue_unlock: error releasing lock");
		return(-1);
		*/
		abort();
	}

	return(0);
}


int
event_queue_signal(struct event_queue *eq)
{
	assert(eq != NULL);

	if(pthread_cond_signal(&eq->ready_cond)) {
		/*** abort instead, this is too serious
		set_error(IL_SYS, errno, "event_queue_signal: error signaling queue thread");
		return(-1);
		*/
		abort();
	}
	return(0);
}


int
event_queue_wait(struct event_queue *eq, int timeout)
{
	assert(eq != NULL);

	if(timeout) {
		struct timespec endtime;
		int  ret = 0;

		endtime.tv_sec = time(NULL) + timeout;
		endtime.tv_nsec = 0;
		
		if((ret=pthread_cond_timedwait(&eq->ready_cond, &eq->cond_lock, &endtime))) {
			if(ret == ETIMEDOUT) 
				return(1);
			/*** abort instead, this is too serious
			set_error(IL_SYS, errno, "event_queue_wait: error waiting on condition variable");
			return(-1);
			*/
			abort();
		}
	} else {
		if(pthread_cond_wait(&eq->ready_cond, &eq->cond_lock)) {
			/*** abort instead, this is too serious
			set_error(IL_SYS, errno, "event_queue_wait: error waiting on condition variable");
			return(-1);
			*/
			abort();
		}
	}
	return(0);
}


int event_queue_sleep(struct event_queue *eq)
{
#if defined(INTERLOGD_HANDLE_CMD) && defined(INTERLOGD_FLUSH)
	struct timespec ts;
	struct timeval tv;
	int ret;

	assert(eq != NULL);

	gettimeofday(&tv, NULL);
	ts.tv_sec = tv.tv_sec + eq->timeout;
	ts.tv_nsec = 1000 * tv.tv_usec;
	if((ret=pthread_cond_timedwait(&eq->flush_cond, &eq->cond_lock, &ts)) < 0) {
		if(ret != ETIMEDOUT) {
			/*** abort instead, this is too serious
			set_error(IL_SYS, errno, "event_queue_sleep: error waiting on condition");
			return(-1);
			*/
			abort();
		}
	}
#else
	sleep(eq->timeout);
#endif
	return(0);
}


#if defined(INTERLOGD_HANDLE_CMD)
int event_queue_wakeup(struct event_queue *eq)
{
	assert(eq != NULL);

	if(pthread_cond_signal(&eq->ready_cond)) {
		/**
		set_error(IL_SYS, errno, "event_queue_wakeup: error signaling queue thread");
		return(-1);
		*/
		abort();
	}
#if defined(INTERLOGD_FLUSH)
	if(pthread_cond_signal(&eq->flush_cond)) {
		/**
		set_error(IL_SYS, errno, "event_queue_wakeup: error signaling queue thread");
		return(-1);
		*/
		abort();
	}
#endif
	return(0);
}
#endif

int event_queue_cond_lock(struct event_queue *eq)
{
	assert(eq != NULL);

	if(pthread_mutex_lock(&eq->cond_lock)) {
		/**
		set_error(IL_SYS, errno, "event_queue_cond_lock: error locking condition mutex");
		return(-1);
		*/
		abort();
	}

	return(0);
}


int event_queue_cond_unlock(struct event_queue *eq)
{
	assert(eq != NULL);

	if(pthread_mutex_unlock(&eq->cond_lock)) {
		/**
		set_error(IL_SYS, errno, "event_queue_cond_unlock: error locking condition mutex");
		return(-1);
		*/
		abort();
	}

	return(0);
}

/* Local Variables:           */
/* c-indentation-style: linux */
/* End:                       */
