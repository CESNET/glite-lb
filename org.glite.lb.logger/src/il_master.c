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


#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "glite/jobid/cjobid.h"
#include "glite/lb/context.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/il_string.h"

#include "interlogd.h"

#ifdef LB_PERF
#include "glite/lb/lb_perftest.h"
#endif

#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
#define il_strerror_r strerror_r
#else
#define il_strerror_r(a,b,c)  if(1) { char *p = strerror_r((a), (b), (c)); if(p && p != (b)) { int len = strlen(p); memset((b), 0, (c)); strncpy((b), p, len > (c) ? (c) : len); } }
#endif

int
enqueue_msg(struct event_queue *eq, struct server_msg *msg)
/* global: parallel */
{
	int ret;

	/* fire thread to take care of this queue */
	if(event_queue_create_thread(eq, parallel) < 0)
		return(-1);

#if defined(IL_NOTIFICATIONS)
	/* if there are no data to send, do not send anything
	   (messsage was just to change the delivery address) */
	/* CORRECTION - let the message pass through the output queue
	   to commit it properly and keep event_store in sync */
	/* if(msg->len == 0)
		return(0);
	*/
#endif
	/* avoid losing signal to thread */
	event_queue_cond_lock(eq);

	/* insert new event */
	if((ret = event_queue_insert(eq, msg)) < 0) {
		event_queue_cond_unlock(eq);
		return ret;
	}

	/* signal thread that we have a new message */
	event_queue_signal(eq);

	/* allow thread to continue */
	event_queue_cond_unlock(eq);

	return ret;
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
				glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_WARN, "command %s not implemented", 
						 r);
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
handle_cmd(il_octet_string_t *event, long offset)
{
	char *job_id_s;
	struct event_queue *eq;
	int  num_replies, num_threads = 0;
	int  timeout, result;
	long receipt;
	struct timespec endtime;
	struct timeval  tv;

	/* parse command */
	if(parse_cmd(event->data, &job_id_s, &receipt, &timeout) < 0)
		return(0);

#if defined(INTERLOGD_FLUSH)
	glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, "received FLUSH command");

	/* catchup with all neccessary event files */
	if(job_id_s) {
		struct event_store *es = event_store_find(job_id_s, NULL);

		if(es == NULL) {
			goto cmd_error;
		}
		result = event_store_recover(es);
		/* NOTE: if flush had been stored in file, there would have been
		   no need to lock the event_store at all */
		event_store_release(es);
		if(result < 0) {
			glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_ERROR, 
					 "  error trying to catch up with event file: %s",
					 error_get_msg());
			clear_error();
		}
	} else
	  /* this call does not fail :-) */
	  event_store_recover_all();

	glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, "  alerting threads to report status");

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
			char buf[256];

			il_strerror_r(errno, buf, sizeof(buf));
			glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_ERROR, 
					 "    error waiting for thread reply: %s", 
					 buf);
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
						glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
								 "    thread reply: %d", 
								 eq->flush_result);
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
		glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_ERROR, 
				 "handle_cmd: error sending status: %s", 
				 error_get_msg());
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
handle_msg(il_octet_string_t *event, long offset)
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
		glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_WARN, 
				 "    handle_msg: error parsing event '%s': %s", 
				 event->data, error_get_msg());
		return(0);
	}

	/* sync event store with IPC (if neccessary)
	 * This MUST be called before inserting event into output queue! */
	if((es = event_store_find(msg->job_id_s, NULL)) == NULL)
		return(-1);
	msg->es = es;

#ifdef LB_PERF
	if(nosync)
		ret = 1;
	else
#endif
		ret = event_store_sync(es, offset);
	/* no longer informative:
	il_log(LOG_DEBUG, "  syncing event store at %d with event at %d, result %d\n", es->offset, offset, ret);
	*/
	if(ret < 0) {
		glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_ERROR, 
				 "    handle_msg: error syncing event store: %s", 
				 error_get_msg());
		/* XXX should error during event store recovery cause us to drop the message? */
		/* Probably no, because the attempt to recover means we have missed some events,
		   and delivery of this one will not move offset ahead. So try our best and deliver it
		   even if it may cause duplicates on server. */
		/* COMMENTED OUT: uncommented again */
		server_msg_free(msg);
		event_store_release(es);
		return(0);
		/* */
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
		glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_ERROR, 
				 "    handle_msg: apropriate queue not found: %s", 
				 error_get_msg());
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
		il_octet_string_t *msg;
		long offset;
		int ret;

		do_handle_signal();
		if(killflg)
			return (0);

		clear_error();
		if((ret = input_queue_get(&msg, &offset, INPUT_TIMEOUT)) < 0)
		{
			if(error_get_maj() == IL_PROTO) {
				glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
						 "  premature EOF while receiving event");
				/* problems with socket input, try to catch up from files */
#ifndef PERF_EMPTY
				event_store_recover_all();
#endif
				continue;
			} else
				return(-1);
		}
		else if(ret == 0) {
			continue;
		}

#ifdef PERF_EMPTY
		glite_wll_perftest_consumeEventString(msg->data);
		free(msg->data);
		continue;
#endif

#ifdef INTERLOGD_HANDLE_CMD
		ret = handle_cmd(msg, offset);
		if(ret == 0)
#endif
			ret = handle_msg(msg, offset);
		if(msg->data) free(msg->data);
		if(ret < 0)
			switch (error_get_maj()) {
				case IL_SYS:
				case IL_NOMEM:
					return (ret);
					break;
				default:
    					glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_ERROR, 
							 "Error: %s", 
							 error_get_msg());
					break;
			}
	} /* while */
}
