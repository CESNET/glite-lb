#ident "$Header$"

#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "glite/lb/consumer.h"
#include "glite/lb/context-int.h"
#include "glite/lb/events_parse.h"
#include "store.h"
#include "lbs_db.h"
#include "lock.h"
#include "il_lbproxy.h"

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
  else
    err = edg_wll_StepIntState(ctx,ev->any.jobId, ev, seq, ctx->isProxy? NULL: &newstat);

  if (edg_wll_UnlockJob(ctx,ev->any.jobId)) goto err;
  if (err) goto err;

  if ( ctx->isProxy ) {
	/*
	 *	send event to the proper BK server
	 */
  	if (   ev->any.type != EDG_WLL_EVENT_REGJOB
		&& edg_wll_EventSendProxy(ctx, ev->any.jobId, event) ) goto err;

	/* LB proxy purge
	 * XXX: Set propper set of states!
	 * TODO: Do the set of states configurable? 
	 */
	switch ( ev->any.type ) {
	case EDG_WLL_EVENT_CLEAR:
	case EDG_WLL_EVENT_ABORT:
	case EDG_WLL_EVENT_CANCEL:
	case EDG_WLL_EVENT_DONE:
		edg_wll_PurgeServerProxy(ctx, ev->any.jobId);
		break;
	}
  } else if ( newstat.state ) {
	  edg_wll_NotifMatch(ctx, &newstat);
	  edg_wll_FreeStatus(&newstat);
  }

  edg_wll_FreeEvent(ev);
  free(ev);

  return(0);

 err:
  if(ev) {
    edg_wll_FreeEvent(ev);
    free(ev);
  }
  
  return edg_wll_Error(ctx,NULL,NULL);
}

