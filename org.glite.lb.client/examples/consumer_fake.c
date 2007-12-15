/* 
 * fake implementation of the consumer API 
 */

#include <stddef.h>
#include <string.h>

#include "glite/lb/consumer.h"
#include "glite/lb/context-int.h"
#include "consumer_fake.h"


#define NUM_EVENTS 3


static edg_wll_QueryEvents_cb_f *QueryEvents_cb = NULL;
static edg_wll_QueryListener_cb_f *QueryListener_cb = NULL;


/* register the query callback */
int edg_wll_RegisterTestQueryEvents(edg_wll_QueryEvents_cb_f *cb) {
  if (QueryEvents_cb) return 0;

  QueryEvents_cb = cb;
  return 1;
}


/* register the listener callback */
int edg_wll_RegisterTestQueryListener(edg_wll_QueryListener_cb_f *cb) {
  if (QueryListener_cb) return 0;
  
  QueryListener_cb = cb;
  return 1;
}


/* unregister the query callback */
void edg_wll_UnregisterTestQueryEvents() {
  QueryEvents_cb = NULL;
}


/* unregister the listener callback */
void edg_wll_UnregisterTestQueryListener() {
  QueryEvents_cb = NULL;
}


/* (belongs to common/src/events.c.T) */
static void edg_wll_PrepareEvent(edg_wll_EventCode eventcode, edg_wll_Event *event) {
  edg_wll_Event *tmpevent;

  // hide not clean code here :-)
  tmpevent = edg_wll_InitEvent(eventcode);
  memcpy(event, tmpevent, sizeof(edg_wll_Event));
  free(tmpevent);
}


/* fake implementation of QueryEvents() */
int edg_wll_QueryEvents(
  edg_wll_Context         context,
  const edg_wll_QueryRec  *job_conditions,
  const edg_wll_QueryRec  *event_conditions,
  edg_wll_Event          **events
) {
  edg_wll_EventCode event_code;
  int i, j, err;
  edg_wlc_JobId jobid;
  
  edg_wll_ResetError(context);

  // determine type of the returned events, ignore _QUERY_OP_*:
  //   - asked event type for _QUERY_ATTR_EVENT_TYPE
  //   - _EVENT_CHKPT for other
  i = 0;
  while (event_conditions[i].attr != EDG_WLL_QUERY_ATTR_UNDEF && (event_conditions[i].attr != EDG_WLL_QUERY_ATTR_EVENT_TYPE)) i++;
  if (event_conditions[i].attr == EDG_WLL_QUERY_ATTR_UNDEF)
    event_code = EDG_WLL_EVENT_CHKPT;
  else
    event_code = event_conditions[i].value.i;

  // create events
  *events = calloc(NUM_EVENTS + 1, sizeof(edg_wll_Event));
  for (i = 0; i < NUM_EVENTS; i++) {
    edg_wll_PrepareEvent(event_code, &(*events)[i]);
  }
  (*events)[NUM_EVENTS].type = EDG_WLL_EVENT_UNDEF;

  // adjust events according to the query parameters
  i = 0;
  while (job_conditions[i].attr != EDG_WLL_QUERY_ATTR_UNDEF) {
    if (job_conditions[i].attr == EDG_WLL_QUERY_ATTR_JOBID && job_conditions[i].op == EDG_WLL_QUERY_OP_EQUAL) {
      jobid = job_conditions[i].value.j;
      for (j = 0; j < NUM_EVENTS; j++) {
         if ((err = edg_wlc_JobIdDup(jobid, &(*events)[i].any.jobId)) != 0) goto error;
      }
      break;
    }
    i++;
  }

  // adjusting callback
  if (QueryEvents_cb)
    QueryEvents_cb(context, events);

  if ((err = edg_wll_Error(context, NULL, NULL)) == 0) return 0;

error:
  i = 0;
  while ((*events)[i].type != EDG_WLL_EVENT_UNDEF) {
    edg_wll_FreeEvent(&(*events)[i]);
    i++;
  }
  free(*events);

  return edg_wll_SetError(context, err, NULL);
}


/* fake implementation of QueryListener() */
int edg_wll_QueryListener(
  edg_wll_Context context,
  edg_wlc_JobId    jobId,
  const char      *name,
  char           **host,
  uint16_t        *port
) {
  edg_wll_ResetError(context);
  
  if (QueryListener_cb) return QueryListener_cb(context, host, port);
  else {
    *host = strdup("localhost");
    *port = 12345;
  
    return edg_wll_Error(context, NULL, NULL);
  }
}


/* cut'nd pasted from consumer.c */
int edg_wll_JobLog(
	edg_wll_Context ctx,
	edg_wlc_JobId	job,
	edg_wll_Event **eventsOut)
{
	edg_wll_QueryRec	j[2], e[2];

	memset(j,0,sizeof j);
	memset(e,0,sizeof e);

	j[0].attr = EDG_WLL_QUERY_ATTR_JOBID;
	j[0].op = EDG_WLL_QUERY_OP_EQUAL;
	j[0].value.j = job;

	e[0].attr = EDG_WLL_QUERY_ATTR_LEVEL;
	e[0].op = EDG_WLL_QUERY_OP_LESS;
	e[0].value.i = ctx->p_level + 1;

	return edg_wll_QueryEvents(ctx,j,e,eventsOut);
}
