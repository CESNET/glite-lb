#ident "$Header$"

#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "glite/lb/context-int.h"
#include "glite/lb/events.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/lb_maildir.h"
#include "purge.h"
#include "store.h"
#include "lock.h"
#include "il_lbproxy.h"
#include "jobstat.h"

#ifdef LB_PERF
#include "glite/lb/lb_perftest.h"
#include "glite/lb/srv_perf.h"
#endif


/* XXX */
#define use_db	1

extern int unset_proxy_flag(edg_wll_Context, edg_wlc_JobId);
extern int edg_wll_NotifMatch(edg_wll_Context, const edg_wll_JobStat *);

static int db_store_finalize(edg_wll_Context ctx, char *event, edg_wll_Event *ev, edg_wll_JobStat *newstat);


int
db_store(edg_wll_Context ctx,char *ucs, char *event)
{
  edg_wll_Event *ev;
  int	seq;
  int   err;
  edg_wll_JobStat	newstat;
  char 			*srvName = NULL;
  unsigned int		srvPort;


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

  edg_wlc_JobIdGetServerParts(ev->any.jobId, &srvName, &srvPort);

  if(use_db) {
    char	*ed;
    int		code;

    if (edg_wll_LockJob(ctx,ev->any.jobId)) goto err;
    store_job_server_proxy(ctx, ev, srvName, srvPort);
    code = edg_wll_Error(ctx,NULL,&ed);
    edg_wll_UnlockJob(ctx,ev->any.jobId);	/* XXX: ignore error */
    if (code) {
	    edg_wll_SetError(ctx,code,ed);
	    free(ed);
	    goto err;
    }
  }


  /* events logged to proxy and server (DIRECT flag) may be ignored on proxy
   * if jobid prefix hostname matches server hostname -> they will
   * sooner or later arrive to server too and are stored in common DB 
   */
  if (ctx->isProxy && ctx->serverRunning && (ev->any.priority & EDG_WLL_LOGFLAG_DIRECT) ) {
	if (!strcmp(ctx->srvName, srvName)) {
		free(srvName);
		return 0;
	}

  }

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
    if(edg_wll_StoreEvent(ctx, ev,&seq)) {
       edg_wll_UnlockJob(ctx,ev->any.jobId);
       goto err;
    }
  }

  if (!ctx->strict_locking && edg_wll_LockJob(ctx,ev->any.jobId)) goto err;

  if ( ev->any.type == EDG_WLL_EVENT_CHANGEACL ) {
    err = edg_wll_UpdateACL(ctx, ev->any.jobId,
			ev->changeACL.user_id, ev->changeACL.user_id_type,
			ev->changeACL.permission, ev->changeACL.permission_type,
			ev->changeACL.operation);

    edg_wll_UnlockJob(ctx,ev->any.jobId);
  }
  else {
#ifdef LB_PERF
    if(sink_mode == GLITE_LB_SINK_STATE) {
	     glite_wll_perftest_consumeEvent(ev);
	     edg_wll_UnlockJob(ctx,ev->any.jobId);
	     goto err;
    }
#endif

    err = edg_wll_StepIntState(ctx,ev->any.jobId, ev, seq, &newstat);
  }

  /* XXX: in edg_wll_StepIntState() 
   * if (edg_wll_UnlockJob(ctx,ev->any.jobId)) goto err;
   */
  if (err) goto err;

  db_store_finalize(ctx, event, ev, &newstat);

err:

  if(ev) {
    edg_wll_FreeEvent(ev);
    free(ev);
  }

  if (srvName) free(srvName);

  if ( newstat.state ) edg_wll_FreeStatus(&newstat);


  return edg_wll_Error(ctx,NULL,NULL);
}


int
db_parent_store(edg_wll_Context ctx, edg_wll_Event *ev, intJobStat *is)
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

    err = edg_wll_StepIntStateParent(ctx,ev->any.jobId, ev, seq, is, ctx->isProxy? NULL: &newstat);
  }

  if (err) goto err;

  if ( ctx->isProxy ) {
    event = edg_wll_UnparseEvent(ctx, ev);
    assert(event);
  }

  db_store_finalize(ctx, event, ev, &newstat);

err:

  free(event);
  if ( newstat.state ) edg_wll_FreeStatus(&newstat);
  
  return edg_wll_Error(ctx,NULL,NULL);
}

static int db_store_finalize(edg_wll_Context ctx, char *event, edg_wll_Event *ev, edg_wll_JobStat *newstat) {

  if ( ctx->isProxy ) {
	/*
	 *	send event to the proper BK server
	 *	event with priority flag EDG_WLL_LOGFLAG_DIRECT (typically RegJob) is not sent
	 */

#ifdef LB_PERF
	if( sink_mode == GLITE_LB_SINK_SEND ) {
		glite_wll_perftest_consumeEvent(ev);
	} else
#endif

	/* XXX: ending here may break the backward compatibility */
	if (!(ev->any.priority & EDG_WLL_LOGFLAG_PROXY)) {
		edg_wll_UpdateError(ctx, 0, "db_actual_store() WARNING: the event is not PROXY");
		//return edg_wll_SetError(ctx, EDG_WLL_IL_PROTO, "db_actual_store() ERROR: the event is not PROXY");
	}

	if (!(ev->any.priority & EDG_WLL_LOGFLAG_DIRECT)) {
		if (edg_wll_EventSendProxy(ctx, ev->any.jobId, event) )  {
			return edg_wll_SetError(ctx, EDG_WLL_IL_PROTO, "edg_wll_EventSendProxy() error.");
		}
	}

	/* LB proxy purge */
	if (newstat->remove_from_proxy) {
			edg_wll_PurgeServerProxy(ctx, ev->any.jobId);
	}
  } else 
#ifdef LB_PERF
	if( sink_mode == GLITE_LB_SINK_SEND ) {
		glite_wll_perftest_consumeEvent(ev);
	} else 
#endif
  {
	char		*jobIdHost = NULL;
	unsigned int	jobIdPort;


	/* Purge proxy flag */
	edg_wlc_JobIdGetServerParts(ev->any.jobId, &jobIdHost, &jobIdPort);
	if ( newstat->remove_from_proxy && (ctx->srvPort == jobIdPort) && 
		!strcmp(jobIdHost,ctx->srvName) )
	{
			if (unset_proxy_flag(ctx, ev->any.jobId) < 0) {
						free(jobIdHost);
                        	                return(edg_wll_Error(ctx,NULL,NULL));
			}
	}
	free(jobIdHost);

	if ( newstat->state ) {
		edg_wll_NotifMatch(ctx, newstat);
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
