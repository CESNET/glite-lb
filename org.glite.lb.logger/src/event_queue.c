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


/*
 *   - general queue handling routines (insert, get)
 */

#include <netdb.h>
#include <sys/socket.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "glite/jobid/cjobid.h"

#include "interlogd.h"

struct event_queue_msg {
  struct server_msg *msg;
  struct event_queue_msg *prev;
  struct event_queue_msg *next;
};

struct event_queue *
event_queue_create(char *server_name,  struct il_output_plugin *output)
{
  struct event_queue *eq;
  char *p,*s, c;

  if((eq = malloc(sizeof(*eq))) == NULL) {
    set_error(IL_NOMEM, ENOMEM, "event_queue_create: error allocating event queue");
    return(NULL);
  }

  memset(eq, 0, sizeof(*eq));

  eq->dest = strdup(server_name);

  s = strstr(server_name, "://");
  if(s == NULL) {
	  s = server_name;
  } else {
	  s = s + 3;
  }
  p = strrchr(s, ':');
  if(p) {
    *p++ = 0;
    c = ':';
  } else {
	  p = strchr(s, '/');
	  if(p) {
		  *p++ = 0;
		  c = '/';
	  }
  }
  eq->dest_name = strdup(s);
  if(p)
    *(p-1) = c;

#if defined(IL_NOTIFICATIONS) || defined(IL_WS)
  if(p && c == ':') {
	  eq->dest_port = atoi(p);
  } else {
	  eq->dest_port = 0; // use whatever default there is for given url scheme
  }
#else
          eq->dest_port = (p && c == ':') ? atoi(p)+1 : GLITE_JOBID_DEFAULT_PORT+1;
#endif

  /* setup output functions */
  if(output != NULL) {
    eq->event_queue_connect = output->event_queue_connect;
    eq->event_queue_send = output->event_queue_send;
    eq->event_queue_close = output->event_queue_close;
  } else {
    eq->event_queue_connect = event_queue_connect;
    eq->event_queue_send = event_queue_send;
    eq->event_queue_close = event_queue_close;
  }

  /* create all necessary locks */
  if(pthread_rwlock_init(&eq->update_lock, NULL)) {
    set_error(IL_SYS, errno, "event_queue_create: error creating update lock");
    free(eq);
    return(NULL);
  }
  if(pthread_mutex_init(&eq->cond_lock, NULL)) {
    set_error(IL_SYS, errno, "event_queue_create: error creating cond mutex");
    free(eq);
    return(NULL);
  }
  if(pthread_cond_init(&eq->ready_cond, NULL)) {
    set_error(IL_SYS, errno, "event_queue_create: error creating cond variable");
    free(eq);
    return(NULL);
  }

#if defined(INTERLOGD_HANDLE_CMD) && defined(INTERLOGD_FLUSH)
  if(pthread_cond_init(&eq->flush_cond, NULL)) {
    set_error(IL_SYS, errno, "event_queue_create: error creating cond variable");
    free(eq);
    return(NULL);
  }
#endif

  return(eq);
}


int
event_queue_free(struct event_queue *eq)
{
  assert(eq != NULL);

  if(!event_queue_empty(eq))
    return(-1);

  /* 
  if(eq->thread_id)
    pthread_cancel(eq->thread_id);
  */

  pthread_rwlock_destroy(&eq->update_lock);
  pthread_mutex_destroy(&eq->cond_lock);
  pthread_cond_destroy(&eq->ready_cond);
#if defined(INTERLOGD_HANDLE_CMD) && defined(INTERLOGD_FLUSH)
  pthread_cond_destroy(&eq->flush_cond);
#endif

  if(eq->dest_name)
	  free(eq->dest_name);
  if(eq->dest)
	  free(eq->dest);
  if(eq->owner) 
	  free(eq->owner);

  free(eq);

  return(0);
}


int
event_queue_empty(struct event_queue *eq)
{
  int ret;

  assert(eq != NULL);

  event_queue_lock_ro(eq);
  ret = (eq->head == NULL);
  event_queue_unlock(eq);

  return(ret);
}


int
event_queue_insert(struct event_queue *eq, struct server_msg *msg)
{
  struct event_queue_msg *el;
#if defined(INTERLOGD_EMS)
  struct event_queue_msg *tail;
#endif

  assert(eq != NULL);

  if(queue_size_high > 0 && (eq->cur_len >= queue_size_high || eq->throttling)) {
	  eq->throttling = 1;
	  return 1;
  }

  if((el = malloc(sizeof(*el))) == NULL)
    return(set_error(IL_NOMEM, ENOMEM, "event_queue_insert: not enough room for queue element"));

  el->msg = server_msg_copy(msg);
  if(el->msg == NULL) {
    free(el);
    return(-1);
  };

  /* this is critical section */
  event_queue_lock(eq);
  if(NULL == eq->head) {
	  el->prev = el;
	  el->next = el;
	  eq->head = el;
	  eq->tail_ems = el;
  } else {
	  if(server_msg_is_priority(msg)) {
		  el->next = eq->tail_ems->next;
		  el->prev = eq->tail_ems;
		  if(eq->head == eq->tail_ems) {
			  eq->head = el;
		  }
	  } else {
		  el->next = eq->head->next;
		  el->prev = eq->head;
	  }
	  el->next->prev = el;
	  el->prev->next = el;
  }
#if 0  /* OLD IMPLEMENTATION */
#if defined(INTERLOGD_EMS)
  if(server_msg_is_priority(msg)) {
    /* priority messages go first */
    tail = eq->tail_ems;
    if(tail) {
      el->prev = tail->prev;
      tail->prev = el;
      if (tail == eq->tail)
	eq->tail = el;
    } else {
      el->prev = eq->head;
      eq->head = el;
      if(eq->tail == NULL)
	eq->tail = el;
    }
    eq->tail_ems = el;
  } else
#endif
  {
    /* normal messages */
    if(eq->tail)
      eq->tail->prev = el;
    else
      eq->head = el;
    eq->tail = el;
    el->prev = NULL;
  }
#if defined(INTERLOGD_EMS)
  /* if we are inserting message between mark_prev and mark_this,
     we have to adjust mark_prev accordingly */
  if(eq->mark_this && (el->prev == eq->mark_this))
    eq->mark_prev = el;
#endif
#endif /* OLD IMPLEMENTATION */

  if(++eq->cur_len > eq->max_len)
	  eq->max_len = eq->cur_len;

  event_queue_unlock(eq);
  /* end of critical section */

  return(0);
}


int
event_queue_get(struct event_queue *eq, struct queue_thread *me, struct server_msg **msg)
{
  struct event_queue_msg *el;
  int found;

  assert(eq != NULL);

  event_queue_lock(eq);
  if(me->jobid) {
	  me->jobid = NULL;
	  me->current = NULL;
  }
  if(NULL == eq->head) {
	  event_queue_unlock(eq);
	  return 0;
  }
  found = 0;
  el = eq->head;
  do {
	  char *jobid = el->msg->job_id_s;
	  int i;
	  
	  for(i = 0; i < eq->num_threads; i++) {
		  if(me == eq->thread + i) {
			  continue;
		  }
		  if(eq->thread[i].jobid && strcmp(jobid, eq->thread[i].jobid) == 0) {
			  break;
		  }
	  }
	  if(i >= eq->num_threads) {
		  /* no other thread is working on this job */
		  found = 1;
		  break;
	  }
	  el = el->prev;
  } while(el != eq->head);
  if(found && msg) {
	  me->current = el;
	  me->jobid = el->msg->job_id_s;
	  *msg = el->msg;
  }
  event_queue_unlock(eq);

  return found;
}


int
event_queue_remove(struct event_queue *eq, struct queue_thread *me)
{
  struct event_queue_msg *el;
#if defined(INTERLOGD_EMS)
  struct event_queue_msg *prev;
#endif

  assert(eq != NULL);
  
  el = me->current;
  if(NULL == el) {
	  return -1;
  }

  /* this is critical section */
  event_queue_lock(eq);

  if(el == el->prev) {
	  /* last element */
	  eq->head = NULL;
	  eq->tail_ems = NULL;
  } else {
	  el->next->prev = el->prev;
	  el->prev->next = el->next;
	  if(el == eq->head) {
		  eq->head = el->prev;
	  }
	  if(el == eq->tail_ems) {
		  eq->tail_ems = el->prev;
	  }
  }

#if 0 /* OLD IMPLEMENTATION */
#if defined(INTERLOGD_EMS)
  el = eq->mark_this;
  prev = eq->mark_prev;

  if(el == NULL) {
    event_queue_unlock(eq);
    return(-1);
  }

  if(prev == NULL) {
    /* removing from head of the queue */
    eq->head = el->prev;
  } else {
    /* removing from middle of the queue */
    prev->prev = el->prev;
  }
  if(el == eq->tail) {
    /* we are removing the last message */
    eq->tail = prev;
  }
  if(el == eq->tail_ems) {
    /* we are removing last priority message */
    eq->tail_ems = prev;
  }

  eq->mark_this = NULL;
  eq->mark_prev = NULL;
#else
  el = eq->head;
  if(el == NULL) {
	  event_queue_unlock(eq);
	  return(-1);
  }
  eq->head = el->prev;
  if(el == eq->tail) {
	  eq->tail = NULL;
  }
#endif
#endif /* OLD IMPLEMENTATION */

  if(--eq->cur_len == 0)
	  eq->times_empty++;

  if(eq->cur_len <= queue_size_low) {
	  eq->throttling = 0;
  }
  
  me->current = NULL;
  me->jobid = NULL;

  event_queue_unlock(eq);
  /* end of critical section */

  server_msg_free(el->msg);
  free(el);

  return(0);
}


int
event_queue_move_events(struct event_queue *eq_s,
			struct event_queue *eq_d,
			int (*cmp_func)(struct server_msg *, void *),
			void *data)
{
	struct event_queue_msg *p, *q, *last;
	/* struct event_queue_msg **source_prev, **dest_tail; */

	assert(eq_s != NULL);
	assert(data != NULL);

	event_queue_lock(eq_s);

	p = eq_s->head;
	if(NULL == p) {
		event_queue_unlock(eq_s);
		return 0;
	}
	if(eq_d) {
		event_queue_lock(eq_d);
	}

	last = p->next;
	do {
		if((*cmp_func)(p->msg, data)) {
			glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
					 "      moving event at offset %d(%d) from %s:%d to %s:%d",
					 p->msg->offset, p->msg->generation, eq_s->dest_name, eq_s->dest_port,
					 eq_d ? eq_d->dest_name : "trash", eq_d ? eq_d->dest_port : -1);
			q = p->prev;
			/* remove message from the source queue */
			if(p->next == p->prev) {
				/* removing last message */
				eq_s->head = NULL;
				eq_s->tail_ems = NULL;
			} else {
				p->next->prev = p->prev;
				p->prev->next = p->next;
				if(p == eq_s->head) {
					eq_s->head = p->prev;
				}
				if(p == eq_s->tail_ems) {
					eq_s->tail_ems = p->prev;
				}
			}
			eq_s->cur_len--;
			if(eq_d) {
				/* append message to the destination queue */
				if(eq_d->head == NULL) {
					eq_d->head = p;
					eq_d->tail_ems = p;
					p->next = p;
					p->prev = p;
				} else {
					/* priorities are ignored */
					p->next = eq_d->head->next;
					p->prev = eq_d->head;
					p->next->prev = p;
					p->prev->next = p;

				}
				if(++eq_d->cur_len > eq_d->max_len) {
					eq_d->max_len = eq_d->cur_len;
				}
			} else {
				/* signal that the message was 'delivered' */
				event_store_commit(p->msg->es, p->msg->ev_len, queue_list_is_log(eq_s),
						   p->msg->generation);
				/* free the message */
				server_msg_free(p->msg);
				free(p);
			}
			p = q;
		} else {
			p = p->prev;
		}
	} while (eq_s->head && p != last);

#if 0 /* OLD IMPLEMENTATION */
	if(eq_d) {
		event_queue_lock(eq_d);
		/* dest tail is set to point to the last (NULL) pointer in the list */
		dest_tail = (eq_d->head == NULL) ? &(eq_d->head) : &(eq_d->tail->prev);
	}
	source_prev = &(eq_s->head);
	p = *source_prev;
	eq_s->tail = NULL;
	while(p) {
		if((*cmp_func)(p->msg, data)) {
			glite_common_log(IL_LOG_CATEGORY, LOG_PRIORITY_DEBUG, 
					 "      moving event at offset %ld(%d) from %s:%d to %s:%d",
					 p->msg->offset, p->msg->generation, eq_s->dest_name, eq_s->dest_port,
			   eq_d ? eq_d->dest_name : "trash", eq_d ? eq_d->dest_port : -1);
			/* il_log(LOG_DEBUG, "  current: %x, next: %x\n", p, p->prev); */
			/* remove the message from the source list */
			*source_prev = p->prev;
			assert(eq_s->cur_len > 0);
			eq_s->cur_len--;
			if(eq_d) {
				/* append the message at the end of destination list */
				p->prev = NULL;
				*dest_tail = p;
				dest_tail = &(p->prev);
				eq_d->tail = p;
				if(++eq_d->cur_len > eq_d->max_len) {
					eq_d->max_len = eq_d->cur_len;
				}
			} else {
				/* signal that the message was 'delivered' */
				event_store_commit(p->msg->es, p->msg->ev_len, queue_list_is_log(eq_s),
						   p->msg->generation);
				/* free the message */
				server_msg_free(p->msg);
				free(p);
			}
		} else {
			/* message stays */
			source_prev = &(p->prev);
			eq_s->tail = p;
		}
		p = *source_prev;
	}
#endif /* OLD IMPLEMENTATION */

	if(eq_s->cur_len <= queue_size_low) {
		eq_s->throttling = 0;
	}
	if(eq_d) event_queue_unlock(eq_d);
	event_queue_unlock(eq_s);
	return(0);
}

