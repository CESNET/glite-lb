#ident "$Header$"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "glite/wmsutils/jobid/cjobid.h"
#include "glite/lb/context.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/il_string.h"

#include "interlogd.h"

int 
enqueue_msg(struct event_queue *eq, struct server_msg *msg)
{
#if defined(IL_NOTIFICATIONS)
	struct event_queue *eq_known;

	/* now we have a new event with possibly changed destination,
	   so check for the already known destination and possibly move 
	   events from the original output queue to a new one */
	eq_known = notifid_map_get_dest(msg->job_id_s);
	if(eq != eq_known) {
		/* client has changed delivery address for this notification */
		if(notifid_map_set_dest(msg->job_id_s, eq) < 0) 
			return(-1);
		/* move all events with this notif_id from eq_known to eq */
		if(eq_known != NULL) 
			event_queue_move_events(eq_known, eq, msg->job_id_s);
	}
#endif

	/* fire thread to take care of this queue */
	if(event_queue_create_thread(eq) < 0) 
		return(-1);
	
#if defined(IL_NOTIFICATIONS)
	/* if there are no data to send, do not send anything 
	   (messsage was just to change the delivery address) */
	if(msg->len == 0) 
		return(0);
#endif
	/* avoid losing signal to thread */
	event_queue_cond_lock(eq);

	/* insert new event */
	if(event_queue_insert(eq, msg) < 0) {
		event_queue_cond_unlock(eq);
		return(-1);
	}
      
	/* signal thread that we have a new message */
	event_queue_signal(eq);

	/* allow thread to continue */
	event_queue_cond_unlock(eq);

	return(0);
}


#if defined(INTERLOGD_HANDLE_CMD) && defined(INTERLOGD_FLUSH)
pthread_mutex_t flush_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t flush_cond = PTHREAD_COND_INITIALIZER;
#endif /* INTERLOGD_FLUSH */

#ifdef INTERLOGD_HANDLE_CMD
static 
int
parse_cmd(char *event, char **job_id_s, long *receipt, int *timeout)
{
	char *token, *r;
	int ret;

	if(strstr(event, "DG.TYPE=\"command\"") == NULL)
		return(-1);

	*job_id_s = NULL;
	*timeout = 0;
	*receipt = 0;
	ret = 0;

	for(token = strtok(event, " "); token != NULL; token = strtok(NULL, " ")) {
		r = index(token, '=');
		if(r == NULL) {
			ret = -1;
			continue;
		}
		if(strncmp(token, "DG.COMMAND", r - token) == 0) {
#if defined(INTERLOGD_FLUSH)			
			if(strcmp(++r, "\"flush\"")) {
#endif
				il_log(LOG_WARNING, "  command %s not implemented\n", r);
				ret = -1;
				continue;
#if defined(INTERLOGD_FLUSH)
			}
#endif
		} else if(strncmp(token, "DG.JOBID", r - token) == 0) {
			char  *p;
      
			r += 2; /* skip =" */
			p = index(r, '"');
			if(p == NULL) { ret = -1; continue; }
			*job_id_s = strndup(r, p-r);

		} else if(strncmp(token, "DG.TIMEOUT", r - token) == 0) {
			sscanf(++r, "\"%d\"", timeout);
		} else if(strncmp(token, "DG.LLLID", r - token) == 0) {
			sscanf(++r, "%ld", receipt);
		}
    
	}
	return(0);
}


/* return value:
 *   0 - not command
 *   1 - success
 *  -1 - failure
 */

static 
int 
handle_cmd(char *event, long offset)
{
	char *job_id_s;
	struct event_queue *eq;
	int  num_replies, num_threads = 0;
	int  timeout, result;
	long receipt;
	struct timespec endtime;
	struct timeval  tv;

	/* parse command */
	if(parse_cmd(event, &job_id_s, &receipt, &timeout) < 0) 
		return(0);

#if defined(INTERLOGD_FLUSH)
	il_log(LOG_DEBUG, "  received FLUSH command\n");

	/* catchup with all neccessary event files */
	if(job_id_s) {
		struct event_store *es = event_store_find(job_id_s);

		if(es == NULL) {
			goto cmd_error;
		}
		result = event_store_recover(es);
		/* NOTE: if flush had been stored in file, there would have been
		   no need to lock the event_store at all */
		event_store_release(es);
		if(result < 0) {
			il_log(LOG_ERR, "  error trying to catch up with event file: %s\n",
			       error_get_msg());
			clear_error();
		}
	} else 
	  /* this call does not fail :-) */
	  event_store_recover_all();

	il_log(LOG_DEBUG, "  alerting threads to report status\n");

	/* prevent threads from reporting too early */
	if(pthread_mutex_lock(&flush_lock) < 0) {
		/*** this error is considered too serious to allow the program run anymore!
		     set_error(IL_SYS, errno, "pthread_mutex_lock: error locking flush lock");
		     goto cmd_error;
		*/
		abort();
	}

	/* wake up all threads */
	if(job_id_s) {
		/* find appropriate queue */
		eq = queue_list_get(job_id_s);
		if(eq == NULL) goto cmd_error;
		if(!event_queue_empty(eq) && !queue_list_is_log(eq)) {
			num_threads++;
			event_queue_cond_lock(eq);
			eq->flushing = 1;
			event_queue_wakeup(eq);
			event_queue_cond_unlock(eq);
		}
	} else {
		/* iterate over event queues */
		for(eq=queue_list_first(); eq != NULL; eq=queue_list_next()) {
			if(!event_queue_empty(eq) && !queue_list_is_log(eq)) {
				num_threads++;
				event_queue_cond_lock(eq);
				eq->flushing = 1;
				event_queue_wakeup(eq);
				event_queue_cond_unlock(eq);
			}
		}
	}
	if(!bs_only) {
		eq = queue_list_get(NULL);
		if(eq == NULL) goto cmd_error;
		if(!event_queue_empty(eq)) {
			num_threads++;
			event_queue_cond_lock(eq);
			eq->flushing = 1;
			event_queue_wakeup(eq);
			event_queue_cond_unlock(eq);
		}
	}

	/* wait for thread replies */
	num_replies = 0;
	result = 1;
	gettimeofday(&tv, NULL);
	endtime.tv_sec = tv.tv_sec + timeout;
	endtime.tv_nsec = 1000 * tv.tv_usec;
	while(num_replies < num_threads) {
		int ret;
		if((ret=pthread_cond_timedwait(&flush_cond, &flush_lock, &endtime)) < 0) {
			il_log(LOG_ERR, "    error waiting for thread reply: %s\n", strerror(errno));
			result = (ret == ETIMEDOUT) ? 0 : -1;
			break;
		}
		
		/* collect results from reporting threads */
		if(job_id_s) {
			/* find appropriate queue */
			eq = queue_list_get(job_id_s);
			if(eq == NULL) goto cmd_error;
			if(!queue_list_is_log(eq)) {
				event_queue_cond_lock(eq);
				if(eq->flushing == 2) {
					eq->flushing = 0;
					num_replies++;
					result = ((result == 1) || (eq->flush_result < 0))  ? 
						eq->flush_result : result;
				}
				event_queue_cond_unlock(eq);
			}
		} else {
			/* iterate over event queues */
			for(eq=queue_list_first(); eq != NULL; eq=queue_list_next()) {
				if(!queue_list_is_log(eq)) {
					event_queue_cond_lock(eq);
					if(eq->flushing == 2) {
						eq->flushing = 0;
						num_replies++;
						il_log(LOG_DEBUG, "    thread reply: %d\n", eq->flush_result);
						result = ((result == 1) || (eq->flush_result < 0))  ? 
							eq->flush_result : result;
					}
					event_queue_cond_unlock(eq);
				}
			}
		}
		if(!bs_only) {
			eq = queue_list_get(NULL);
			if(eq == NULL) goto cmd_error;
			event_queue_cond_lock(eq);
			if(eq->flushing == 2) {
				eq->flushing = 0;
				num_replies++;
				result = ((result == 1) || (eq->flush_result < 0))  ? 
					eq->flush_result : result;
			}
			event_queue_cond_unlock(eq);
		}
	}

	/* prevent deadlock in next flush */
	if(pthread_mutex_unlock(&flush_lock) < 0) 
		abort();


	/* report back to local logger */
	switch(result) {
	case 1:
		result = 0; break;
	case 0:
		result = EDG_WLL_IL_EVENTS_WAITING; break;
	default:
		result = EDG_WLL_IL_SYS; break;
	}
	if(job_id_s) free(job_id_s);
	result = send_confirmation(receipt, result);
	if(result <= 0) 
		il_log(LOG_ERR, "handle_cmd: error sending status: %s\n", error_get_msg());
	return(1);


cmd_error:
	if(job_id_s) free(job_id_s);
	return(-1);
#else
	return(0);
#endif /* INTERLOGD_FLUSH */
}
#endif /* INTERLOGD_HANDLE_CMD */


static 
int
handle_msg(char *event, long offset)
{ 
	struct server_msg *msg = NULL;
#if !defined(IL_NOTIFICATIONS)
	struct event_queue *eq_l;
#endif
	struct event_queue *eq_s;
	struct event_store *es;

	int ret;

	/* convert event to message for server */
	if((msg = server_msg_create(event, offset)) == NULL) {
		il_log(LOG_ERR, "    handle_msg: error parsing event '%s':\n      %s\n", event, error_get_msg());
		return(0);
	}
  
	/* sync event store with IPC (if neccessary)
	 * This MUST be called before inserting event into output queue! */
	if((es = event_store_find(msg->job_id_s)) == NULL) 
		return(-1);
	msg->es = es;

	ret = event_store_sync(es, offset);
	il_log(LOG_DEBUG, "  syncing event store at %d with event at %d, result %d\n", es->offset, offset, ret);
	if(ret < 0) {
		il_log(LOG_ERR, "    handle_msg: error syncing event store:\n      %s\n", error_get_msg());
		event_store_release(es);
		return(0);
	} else if(ret == 0) {
		/* we have seen this event already */
		server_msg_free(msg);
		event_store_release(es);
		return(1);
	}

	/* find apropriate queue for this event */
#if defined(IL_NOTIFICATIONS)
	eq_s = queue_list_get(msg->dest);
#else
	eq_s = queue_list_get(msg->job_id_s);
#endif
	if(eq_s == NULL) { 
		il_log(LOG_ERR, "    handle_msg: apropriate queue not found: %s\n", error_get_msg());
		clear_error();
	} else {
		if(enqueue_msg(eq_s, msg) < 0)
			goto err;
	}

#if !defined(IL_NOTIFICATIONS)
	eq_l = queue_list_get(NULL);
	if(!bs_only && eq_l != eq_s) {
		/* send to default queue (logging server) as well */
		if(enqueue_msg(eq_l, msg) < 0)
			goto err;
	}
#endif

	/* if there was no error, set the next expected event offset */
	event_store_next(es, offset, msg->ev_len);

	/* allow cleanup thread to check on this event_store */
	event_store_release(es);

	/* free the message */
	server_msg_free(msg);
	return(1);

err:
	event_store_release(es);
	server_msg_free(msg);
	return(-1);
}



int 
loop()
{
	/* receive events */
	while(1) {
		char *msg;
		long offset;
		int ret;
    
		clear_error();
		if((ret = input_queue_get(&msg, &offset, INPUT_TIMEOUT)) < 0) 
		{
			if(error_get_maj() == IL_PROTO) {
				il_log(LOG_DEBUG, "  premature EOF while receiving event\n");
				/* problems with socket input, try to catch up from files */
				event_store_recover_all();
				continue;
			} else 
				return(-1);
		}
		else if(ret == 0) {
			continue;
		}

#ifdef INTERLOGD_HANDLE_CMD		
		ret = handle_cmd(msg, offset);
		if(ret == 0)
#endif
			ret = handle_msg(msg, offset);
		free(msg);
		if(ret < 0)
			switch (error_get_maj()) {
				case IL_SYS:
				case IL_NOMEM:
					return (ret);
					break;
				default: 
    					il_log(LOG_ERR, "Error: %s\n", error_get_msg());
					break;
			}
	} /* while */
}
