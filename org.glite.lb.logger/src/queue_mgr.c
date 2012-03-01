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


#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>

#include "glite/jobid/cjobid.h"
#include "glite/lb/context.h"

#include "interlogd.h"

struct queue_list {
  struct queue_list *queues;
  struct event_queue *queue;
  char   *dest;
  struct queue_list *next;
#if defined(IL_NOTIFICATIONS)
  time_t expires;
#endif
};

#if !defined(IL_NOTIFICATIONS)
static struct event_queue *log_queue;
#endif
static struct queue_list  *queues;


static 
int
queue_list_create()
{
  queues = NULL;

  return(0);
}


static
int
queue_list_find(struct queue_list *ql, const char *dest, struct queue_list **el, struct queue_list **prev)
{
  struct queue_list *q, *p;

  assert(el != NULL);

  *el = NULL;
  if(prev)
    *prev = NULL;

  if(ql == NULL) 
    return(0);

  q = NULL;
  p = ql;

  while(p) {
    if(strcmp(p->dest, dest) == 0) {
      *el = p;
      if(prev)
	*prev = q;
      return(1);
    }

    q = p;
    p = p->next;
  };

  return(0);
}


static
int
queue_list_add(struct queue_list **ql, const char *dest, struct event_queue *eq)
{
  struct queue_list *el;

  assert(dest != NULL);
  assert(ql != NULL);

  el = malloc(sizeof(*el));
  if(el == NULL) {
    set_error(IL_NOMEM, ENOMEM, "queue_list_add: not enough room for new queue");
    return(-1);
  }

  el->dest = strdup(dest);
  if(el->dest == NULL) {
    free(el);
    set_error(IL_NOMEM, ENOMEM, "queue_list_add: not enough memory for new queue");
    return(-1);
  }
  el->queue = eq;
  el->next = *ql;
#if defined(IL_NOTIFICATIONS)
  el->expires = 0;
#endif
  *ql = el;
  return 0;
}


#if !defined(IL_NOTIFICATIONS)
static
char *
jobid2dest(edg_wlc_JobId jobid)
{
  char *server_name,*out;
  unsigned int	server_port;

  if (!jobid) {
    set_error(IL_PROTO, EDG_WLL_ERROR_PARSE_BROKEN_ULM, "jobid2dest: invalid job id");
    return(NULL);
  }
  edg_wlc_JobIdGetServerParts(jobid,&server_name,&server_port);

  asprintf(&out,"%s:%d",server_name,server_port);
  free(server_name);
  if(!out)
    set_error(IL_SYS, ENOMEM, "jobid2dest: error creating server name");
  return(out);
}
#endif

struct event_queue *
queue_list_get(char *job_id_s)
{
  char *dest;
  struct queue_list *q;
  struct event_queue *eq;
  struct il_output_plugin *outp;

#if !defined(IL_NOTIFICATIONS)
  IL_EVENT_ID_T job_id;

  if(job_id_s == NULL || strcmp(job_id_s, "default") == 0)
    return(log_queue);

  if(edg_wlc_JobIdParse(job_id_s, &job_id)) {
    set_error(IL_LBAPI, EDG_WLL_ERROR_PARSE_BROKEN_ULM, "queue_list_get: invalid job id");
    return(NULL);
  }

  dest = jobid2dest(job_id);
  edg_wlc_JobIdFree(job_id);
  outp = NULL;
#else
  dest = job_id_s;
  outp = plugin_get(dest);
#endif

  if(dest == NULL) 
    return(NULL);
  
  if(queue_list_find(queues, dest, &q, NULL)) {
    /* queue found for given destination */
    eq = q->queue;
  } else {
    /* no queue for given destination found */
    eq = event_queue_create(dest, outp);
    if(eq)
      queue_list_add(&queues, dest, eq);
  }
#if !defined(IL_NOTIFICATIONS)
  free(dest);
#endif
  return eq;
}


int
queue_list_is_log(struct event_queue *eq)
{
	return(eq == queue_list_get(NULL));
}


int
queue_list_init(char *ls)
{
#if !defined(IL_NOTIFICATIONS)
  /* create queue for log server */
  log_queue = event_queue_create(ls, NULL);
  if(log_queue == NULL)
    return(-1);
#endif

  return(queue_list_create());
}


static struct queue_list *current;


struct event_queue *
queue_list_first()
{
  current = queues;
  return(current ? current->queue : NULL);
}


struct event_queue *
queue_list_next()
{
  current = current ? current->next : NULL;
  return(current ? current->queue : NULL);
}


int
queue_list_remove_queue(struct event_queue *eq)
{
  assert(eq != NULL);

  free(eq);
  return(1);
}


#if defined(IL_NOTIFICATIONS)

static struct queue_list  *notifid_map = NULL;

struct event_queue *
notifid_map_get_dest(const char * notif_id)
{
	struct queue_list *q = NULL;

	queue_list_find(notifid_map, notif_id, &q, NULL);
	return(q ? q->queue : NULL);
}


/* returns 1 if mapping was changed, 0 if new one had to be created, -1 on error */
int
notifid_map_set_dest(const char *notif_id, struct event_queue *eq)
{
	struct queue_list *q;

	if(queue_list_find(notifid_map, notif_id, &q, NULL)) {
		q->queue = eq;
		return(1);
	} else {
		return(queue_list_add(&notifid_map, notif_id, eq));
	}
}


time_t
notifid_map_get_expiration(const char * notif_id)
{
  struct queue_list *q;

  queue_list_find(notifid_map, notif_id, &q, NULL);
  return(q ? q->expires : 0);
}


int
notifid_map_set_expiration(const char *notif_id, time_t exp)
{
  struct queue_list *q;

  if(queue_list_find(notifid_map, notif_id, &q, NULL)) {
    q->expires = exp;
    return(1);
  } else {
    return(0);
  }
}

#endif

/* Local Variables:           */
/* c-indentation-style: gnu   */
/* End:                       */
