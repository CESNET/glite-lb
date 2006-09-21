#ident "$Header$"

#include <string.h>
#include <errno.h>
#include <assert.h>

#include "glite/lb/consumer.h"

#include "interlogd.h"

struct queue_list {
  struct event_queue *queue;
  char   *dest;
  struct queue_list *next;
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
  el->next = queues;
  *ql = el;
  return 0;
}


/*
static
int
queue_list_remove(struct queue_list *el, struct queue_list *prev)
{
  assert(el != NULL);

  if(prev) 
    prev->next = el->next;
  else
    queues = el->next;

  free(el);
  return(1);
}
*/


#if !defined(IL_NOTIFICATIONS)
static
char *
jobid2dest(glite_lbu_JobId jobid)
{
  char *server_name,*out;
  unsigned int	server_port;

  if (!jobid) {
    set_error(IL_PROTO, EDG_WLL_ERROR_PARSE_BROKEN_ULM, "jobid2dest: invalid job id");
    return(NULL);
  }
  glite_lbu_JobIdGetServerParts(jobid,&server_name,&server_port);

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
#if !defined(IL_NOTIFICATIONS)
  IL_EVENT_ID_T job_id;

  if(job_id_s == NULL || strcmp(job_id_s, "default") == 0)
    return(log_queue);

  if(glite_lbu_JobIdParse(job_id_s, &job_id)) {
    set_error(IL_LBAPI, EDG_WLL_ERROR_PARSE_BROKEN_ULM, "queue_list_get: invalid job id");
    return(NULL);
  }

  dest = jobid2dest(job_id);
  glite_lbu_JobIdFree(job_id);
#else
  dest = job_id_s;
#endif

  if(dest == NULL) 
    return(NULL);
  
  if(queue_list_find(queues, dest, &q, NULL)) {
#if !defined(IL_NOTIFICATIONS)
    free(dest);
#endif
    return(q->queue);
  } else {
    eq = event_queue_create(dest);
    if(eq)
      queue_list_add(&queues, dest, eq);
#if !defined(IL_NOTIFICATIONS)
    free(dest);
#endif
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
#if !defined(IL_NOTIFICATIONS)
  /* create queue for log server */
  log_queue = event_queue_create(ls);
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

#endif
