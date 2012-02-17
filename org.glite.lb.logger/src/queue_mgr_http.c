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

#include "glite/jobid/cjobid.h"
#include "glite/lb/context.h"

#include "interlogd.h"

struct queue_list {
  struct event_queue *queue;
  char   *dest;
  struct queue_list *next;
  time_t expires;
};

static struct event_queue *log_queue;
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
  assert(eq != NULL);
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
  *ql = el;
  return 0;
}


struct event_queue *
queue_list_get(char *job_id_s)
{
  char *dest;
  struct queue_list *q;
  struct event_queue *eq;
  dest = job_id_s;

  if(dest == NULL) 
    return(NULL);
  
  if(queue_list_find(queues, dest, &q, NULL)) {
    return(q->queue);
  } else {
    eq = event_queue_create(dest);
    if(eq)
      queue_list_add(&queues, dest, eq);
    return(eq);
  }
}


int
queue_list_is_log(struct event_queue *eq)
{
	return(eq == queue_list_get(NULL));
}


int
queue_list_init(char *ls)
{
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



/* Local Variables:           */
/* c-indentation-style: gnu   */
/* End:                       */
