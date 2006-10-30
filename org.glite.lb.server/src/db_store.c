#ident "$Header$"

#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "glite/lb/consumer.h"
#include "glite/lb/context-int.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/lb_maildir.h"
#include "glite/lb/purge.h"
#include "purge.h"
#include "store.h"
#include "lbs_db.h"
#include "lock.h"
#include "il_lbproxy.h"

#ifdef LB_PERF
#include "glite/lb/lb_perftest.h"
#include "glite/lb/srv_perf.h"
#endif


/* XXX */
#define use_db	1

extern int edg_wll_NotifMatch(edg_wll_Context, const edg_wll_JobStat *);

int
db_store(edg_wll_Context ctx,char *ucs, char *event)
{
  edg_wll_Event *ev;
  int	seq;
  int   err;
  edg_wll_JobStat	newstat;

  ev = NULL;

  edg_wll_ResetError(ctx);
  memset(&newstat,0,sizeof newstat);

  if(edg_wll_ParseEvent(ctx, event, &ev))
    goto err;

#ifdef LB_PERF
  if (sink_mode == GLITE_LB_SINK_STORE) {
	  glite_wll_perftest_consumeEvent(ev);
	  edg_wll_FreeEvent(ev);
	  free(ev);
	  return 0;
  }
#endif


  /* XXX: if event type is user tag, convert the tag name to lowercase!
   * 	  (not sure whether to convert a value too is reasonable
   * 	  or keep it 'case sensitive')
   */
  if ( ev->any.type == EDG_WLL_EVENT_USERTAG )
  {
	int i;
	for ( i = 0; ev->userTag.name[i] != '\0'; i++ )
	  ev->userTag.name[i] = tolower(ev->userTag.name[i]);
  }
  
  if(ev->any.user == NULL)
    ev->any.user = strdup(ucs);

  if(use_db) {
    if (ctx->strict_locking && edg_wll_LockJob(ctx,ev->any.jobId)) goto err;
    if(edg_wll_StoreEvent(ctx, ev,&seq))
      goto err;
  }

  if (!ctx->strict_locking && edg_wll_LockJob(ctx,ev->any.jobId)) goto err;

  if ( ev->any.type == EDG_WLL_EVENT_CHANGEACL )
    err = edg_wll_UpdateACL(ctx, ev->any.jobId,
			ev->changeACL.user_id, ev->changeACL.user_id_type,
			ev->changeACL.permission, ev->changeACL.permission_type,
			ev->changeACL.operation);
  else {
#ifdef LB_PERF
    if(sink_mode == GLITE_LB_SINK_STATE) {
	     glite_wll_perftest_consumeEvent(ev);
	     edg_wll_UnlockJob(ctx,ev->any.jobId);
	     goto err;
    }
#endif

    err = edg_wll_StepIntState(ctx,ev->any.jobId, ev, seq, ctx->isProxy? NULL: &newstat);
  }

  if (edg_wll_UnlockJob(ctx,ev->any.jobId)) goto err;
  if (err) goto err;

  db_actual_store(ctx, event, ev, &newstat);

err:

  if(ev) {
    edg_wll_FreeEvent(ev);
    free(ev);
  }
  
  return edg_wll_Error(ctx,NULL,NULL);
}


int
db_parent_store(edg_wll_Context ctx, edg_wll_Event *ev)
{
  char  *event = NULL;
  int	seq;
  int   err;
  edg_wll_JobStat	newstat;


  edg_wll_ResetError(ctx);
  memset(&newstat,0,sizeof newstat);


#ifdef LB_PERF
  if (sink_mode == GLITE_LB_SINK_STORE) {
	  glite_wll_perftest_consumeEvent(ev);
	  edg_wll_FreeEvent(ev);
	  free(ev);
	  return 0;
  }
#endif


  /* XXX: if event type is user tag, convert the tag name to lowercase!
   * 	  (not sure whether to convert a value too is reasonable
   * 	  or keep it 'case sensitive')
   */
  if ( ev->any.type == EDG_WLL_EVENT_USERTAG )
  {
	int i;
	for ( i = 0; ev->userTag.name[i] != '\0'; i++ )
	  ev->userTag.name[i] = tolower(ev->userTag.name[i]);
  }
  
  assert(ev->any.user);

  if(use_db) {
    if(edg_wll_StoreEvent(ctx, ev,&seq))
      goto err;
  }

  if ( ev->any.type == EDG_WLL_EVENT_CHANGEACL )
    err = edg_wll_UpdateACL(ctx, ev->any.jobId,
			ev->changeACL.user_id, ev->changeACL.user_id_type,
			ev->changeACL.permission, ev->changeACL.permission_type,
			ev->changeACL.operation);
  else {
#ifdef LB_PERF
    if(sink_mode == GLITE_LB_SINK_STATE) {
	     glite_wll_perftest_consumeEvent(ev);
	     goto err;
    }
#endif

    err = edg_wll_StepIntState(ctx,ev->any.jobId, ev, seq, ctx->isProxy? NULL: &newstat);
  }

  if (err) goto err;

  if ( ctx->isProxy ) {
    event = edg_wll_UnparseEvent(ctx, ev);
    assert(event);
  }

  db_actual_store(ctx, event, ev, &newstat);

err:

  free(event);
  
  return edg_wll_Error(ctx,NULL,NULL);
}

int db_actual_store(edg_wll_Context ctx, char *event, edg_wll_Event *ev, edg_wll_JobStat *newstat) {

  if ( ctx->isProxy ) {
	/*
	 *	send event to the proper BK server
	 */
	/* XXX: RegJob events, which were logged also directly, are duplicated at server,
		but it should not harm */

#ifdef LB_PERF
	if( sink_mode == GLITE_LB_SINK_SEND ) {
		glite_wll_perftest_consumeEvent(ev);
	} else
#endif

	if (edg_wll_EventSendProxy(ctx, ev->any.jobId, event) )  {
		return edg_wll_SetError(ctx, EDG_WLL_IL_PROTO, "edg_wll_EventSendProxy() error.");
	}

	/* LB proxy purge
	 * XXX: Set propper set of states!
	 * TODO: Do the set of states configurable? 
	 */
	switch ( ev->any.type ) {
	case EDG_WLL_EVENT_CLEAR:
	case EDG_WLL_EVENT_ABORT:
		edg_wll_PurgeServerProxy(ctx, ev->any.jobId);
		break;
	case EDG_WLL_EVENT_CANCEL:
		if (ev->cancel.status_code == EDG_WLL_CANCEL_DONE) 
			edg_wll_PurgeServerProxy(ctx, ev->any.jobId);
		break;
	default: break;
	}
  } else 
#ifdef LB_PERF
	if( sink_mode == GLITE_LB_SINK_SEND ) {
		glite_wll_perftest_consumeEvent(ev);
	} else 
#endif
  {
	if ( newstat->state ) {
		edg_wll_NotifMatch(ctx, newstat);
		edg_wll_FreeStatus(newstat);
	}
	if ( ctx->jpreg_dir && ev->any.type == EDG_WLL_EVENT_REGJOB ) {
		char *jids, *msg;
		
		if ( !(jids = edg_wlc_JobIdUnparse(ev->any.jobId)) ) {
			return edg_wll_SetError(ctx, errno, "Can't unparse jobid when registering to JP");
		}
		if ( !(msg = realloc(jids, strlen(jids)+strlen(ev->any.user)+2)) ) {
			free(jids);
			return edg_wll_SetError(ctx, errno, "Can't allocate buffer when registering to JP");
		}
		strcat(msg, "\n");
		strcat(msg, ev->any.user);
		if ( edg_wll_MaildirStoreMsg(ctx->jpreg_dir, ctx->srvName, msg) ) {
			free(msg);
			return edg_wll_SetError(ctx, errno, lbm_errdesc);
		}
		free(msg);
	}
  }
  return edg_wll_Error(ctx,NULL,NULL);
}
