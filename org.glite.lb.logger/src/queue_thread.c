#ident "$Header$"
/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/param.h>
#include <string.h>

#include "interlogd.h"

#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
#define il_strerror_r strerror_r
#else
#define il_strerror_r(a,b,c)  if(1) { char *p = strerror_r((a), (b), (c)); if(p && p != (b)) { int len = strlen(p); memset((b), 0, (c)); strncpy((b), p, len > (c) ? (c) : len); } }
#endif

static 
void 
queue_thread_cleanup(void *q)
{
	struct event_queue *eq = (struct event_queue *)q;
	pthread_t my_id = pthread_self();
	int me;

	glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "thread %p exits", my_id);

	/* unlock all held locks */
	/* FIXME: check that the thread always exits when holding these locks;
	   unlock them at appropriate places if this condition is not met  
	   event_queue_unlock(eq);
	   event_queue_cond_unlock(eq);
	*/
  
	/* clear thread id */
	for(me = 0; me < eq->num_threads; me++) {
		if(my_id == eq->thread[me].thread_id) {
			eq->thread[me].thread_id = 0;
			break;
		}
	}
}


static time_t now;

static
int
cmp_expires(struct server_msg *msg, void *data)
{
  time_t *t = (time_t*)data;
  return (msg->expires > 0) && (msg->expires < *t);
}

extern char *file_prefix;

static 
void
event_queue_write_stat(struct event_queue *eq) {
	FILE *statfile;
	char fn[MAXPATHLEN];
	char buf[256];

	snprintf(fn, sizeof(fn), "%s.%s.stat", file_prefix, eq->dest_name);
	statfile = fopen(fn, "w");
	if(NULL == statfile) {
		il_strerror_r(errno, buf, sizeof(buf));
		glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_WARN,
				 "Error opening destination stat file %s: %s", 
				 fn, buf);
		return;
	}
	if(fprintf(statfile, "last_connected=%ld\nlast_sent=%ld\n",
		   eq->last_connected,
		   eq->last_sent) < 0) {
		il_strerror_r(errno, buf, sizeof(buf));
		glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_WARN,
				 "Error writing destination statistics into %s: %s",
				 fn, buf);
	}
	fclose(statfile);
}


static
void *
queue_thread(void *q)
{
	struct event_queue *eq = (struct event_queue *)q;
	struct queue_thread *me;
	pthread_t my_id;
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
			 "  started new thread for delivery to %s",
			 eq->dest);

	my_id = pthread_self();
	for(me = eq->thread; me - eq->thread < eq->num_threads; me++) {
		if(my_id == me->thread_id) {
			break;
		}
	}
	if(me - eq->thread >= eq->num_threads) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR,
				 "Error looking up thread identity, exiting!");
		pthread_exit(NULL);
	}

	pthread_cleanup_push(queue_thread_cleanup, q); 

	event_queue_cond_lock(eq);

	exit = 0;
	retrycnt = 0;
	while(!exit) {
    
		clear_error();

		/* if there are no events, wait for them */
		ret = 0;
		while ((event_queue_empty(eq) || event_queue_get(eq, me, NULL) == 0)
#if defined(INTERLOGD_HANDLE_CMD) && defined(INTERLOGD_FLUSH)
		       && (eq->flushing != 1)
#endif
			) {
			if(lazy_close && close_timeout) {
				ret = event_queue_wait(eq, close_timeout);
				if(ret == 1) {/* timeout? */
					(*eq->event_queue_close)(eq, me);
					glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
							 "  connection to %s closed",
							 eq->dest);
				}
				close_timeout = 0;
			} else {
				ret = event_queue_wait(eq, exit_timeout);
				if(ret == 1) {
					glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, 
							 "  thread %x idle for more than %d seconds, exiting", 
							 me,
							 exit_timeout);
					(*eq->event_queue_close)(eq, me);
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
		
		/* discard expired events - only the first thread */
		if(me == eq->thread) {
		  glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, "  thread %x: discarding expired events", me);
			now = time(NULL);
			event_queue_move_events(eq, NULL, cmp_expires, &now);
		}
		if(!event_queue_empty(eq) && event_queue_get(eq, me, NULL) != 0) {
		  
			/* deliver pending events */
			glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
					 "  thread %x: attempting delivery to %s",
					 me,
					 eq->dest);
			/* connect to server */
			if((ret=(*eq->event_queue_connect)(eq, me)) == 0) {
				/* not connected */
				if(error_get_maj() != IL_OK)
					glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_WARN, 
							 "queue_thread: %s", error_get_msg());
#if defined(IL_NOTIFICATIONS)
				glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_INFO, 
						 "    thread %x: could not connect to client %s, waiting for retry", 
						 me,
						 eq->dest);
#else
				glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_INFO, 
						 "    thread %x: could not connect to bookkeeping server %s, waiting for retry", 
						 me,
						 eq->dest);
#endif
				retrycnt++;
			} else {
				retrycnt = 0;
				/* connected, send events */
				switch(ret=(*eq->event_queue_send)(eq, me)) {
					
				case 0:
					/* there was an error and we still have events to send */
					if(error_get_maj() != IL_OK)
						glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_WARN, 
								 "queue_thread: %s", 
								 error_get_msg());
					glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
							 "  thread %x: events still waiting", me);
					break;
					
				case 1:
					/* hey, we are done for now */
					glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
							 "  thread %x: all events for %s sent", 
							 me,
							 eq->dest);
					break;
					
				default:
					/* internal error */
					glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_ERROR, 
							 "queue_thread: %s", 
							 error_get_msg());
					exit = 1;      
					break;
					
				} /* switch */
			
				/* we are done for now anyway, so close the queue */
				if((ret == 1) && lazy_close)
					close_timeout = default_close_timeout;
				else {
					(*eq->event_queue_close)(eq, me);
					glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG,
							 "  thread %x: connection to %s closed",
							 me,
							 eq->dest);
				}
			}
			event_queue_write_stat(eq);
		} 

#if defined(INTERLOGD_HANDLE_CMD) && defined(INTERLOGD_FLUSH)
		if(pthread_mutex_lock(&flush_lock) < 0)
			abort();
		event_queue_cond_lock(eq);

		/* Check if we are flushing and if we are, report status to master */
		if(eq->flushing == 1) {
			glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
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

		event_queue_lock(eq);
		if(me->jobid) {
			me->jobid = NULL;
			me->current = NULL;
		}
		event_queue_unlock(eq);


		/* if there was some error with server, sleep for a while */
		/* iff !event_queue_empty() */
		/* also allow for one more try immediately after server disconnect,
		   which may cure server kicking us out after given number of connections */
#ifndef LB_PERF
		if((ret == 0) && (retrycnt > 0)) {
			glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
					 "    thread %x: sleeping", me);
			event_queue_sleep(eq, me->timeout);
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
event_queue_create_thread(struct event_queue *eq, int num_threads)
{
	pthread_attr_t attr;
	int i;

	assert(eq != NULL);

	event_queue_lock(eq);

	eq->num_threads = num_threads;

	/* create thread data */
	if(NULL == eq->thread) {
		eq->thread = calloc(num_threads, sizeof(*eq->thread));
		if(NULL == eq->thread) {
			set_error(IL_NOMEM, ENOMEM, "event_queue_create: error allocating data for threads");
			event_queue_unlock(eq);
			return -1;
		}
	}

	/* create the threads itself */
	for(i = 0; i < eq->num_threads; i++) {
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, 65536); 
		if(eq->thread[i].thread_id == 0) {
			if(pthread_create(&eq->thread[i].thread_id, &attr, queue_thread, eq) < 0) {
				eq->thread[i].thread_id = 0;
				set_error(IL_SYS, errno, "event_queue_create_thread: error creating new thread");
				event_queue_unlock(eq);
				return(-1);
			}
		}
		/* the thread is never going to be joined */
		pthread_detach(eq->thread[i].thread_id);
	}

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


int event_queue_sleep(struct event_queue *eq, int timeout)
{
#if defined(INTERLOGD_HANDLE_CMD) && defined(INTERLOGD_FLUSH)
	struct timespec ts;
	struct timeval tv;
	int ret;

	assert(eq != NULL);

	gettimeofday(&tv, NULL);
	ts.tv_sec = tv.tv_sec + timeout;
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
	sleep(timeout);
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
