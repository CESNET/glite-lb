#ident "$Header: /cvs/glite/org.glite.lb.server/src/db_store.c,v 1.53 2012/08/24 14:47:01 jfilipov Exp $"
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
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "glite/lbu/maildir.h"
#include "glite/lb/context-int.h"
#include "glite/lb/events.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/ulm_parse.h"
#include "purge.h"
#include "store.h"
#include "il_lbproxy.h"
#include "jobstat.h"
#include "db_supp.h"
#include "il_notification.h"
#include "glite/lbu/trio.h"
#include "glite/lbu/log.h"
#include "glite/lb/process_event.h"

#ifdef LB_PERF
#include "glite/lb/lb_perftest.h"
#include "srv_perf.h"
#endif


extern int unset_proxy_flag(edg_wll_Context, edg_wlc_JobId);
extern int proxy_purge;


static int db_store_finalize(edg_wll_Context ctx, char *event, edg_wll_Event *ev, edg_wll_JobStat *oldstat, edg_wll_JobStat *newstat, int reg_to_JP);
static int log_connectjob_event(edg_wll_Context ctx, glite_jobid_const_t jobif_from, glite_jobid_const_t jobid_to, enum edg_wll_StatJobtype jobtype);
static int log_disconnectjob_event(edg_wll_Context ctx, glite_jobid_const_t jobif_from, glite_jobid_const_t jobid_to);
static enum edg_wll_StatJobtype get_jobtype_from_connectjob(enum edg_wll_ConnectJobJobtype  jobtype);
static enum edg_wll_ConnectJobJobtype get_connectjobtype_from_jobtype(enum edg_wll_StatJobtype jobtype);
static int get_seqno_for_event(edg_wll_Context ctx, edg_wll_Event *e);

int
db_store(edg_wll_Context ctx, char *event)
{
  edg_wll_Event 	*ev = NULL;
  int			seq, reg_to_JP = 0, local_job;
  intJobStat 	       *ijsp;
  edg_wll_JobStat	newstat;
  edg_wll_JobStat	oldstat;
  int			ret;


  edg_wll_ResetError(ctx);
  memset(&newstat,0,sizeof newstat);
  memset(&oldstat,0,sizeof oldstat);

  if(edg_wll_ParseEvent(ctx, event, &ev)) goto err;

  local_job = is_job_local(ctx, ev->any.jobId);

  if(ctx->event_load) {
    char buff[30];
    if(sscanf(event, "DG.ARRIVED=%s %*s", buff) == 1)
      edg_wll_ULMDateToTimeval(buff, &(ev->any.arrived));
  }

  if (!ctx->isProxy && check_store_authz(ctx, ev) != 0)
    goto err;

#ifdef LB_PERF
  if (sink_mode == GLITE_LB_SINK_STORE) {
	  glite_wll_perftest_consumeEvent(ev);
	  edg_wll_FreeEvent(ev);
	  free(ev);
	  return 0;
  }
#endif

  do {
	if (edg_wll_Transaction(ctx)) goto err;

	if (store_job_server_proxy(ctx, ev, &reg_to_JP)) goto rollback;

	/* events logged to proxy and server (DIRECT flag) may be ignored on proxy
	* if jobid prefix hostname matches server hostname -> they will
   	* sooner or later arrive to server too and are stored in common DB 
   	*/
  	if (ctx->isProxy && local_job && (ev->any.priority & EDG_WLL_LOGFLAG_DIRECT)) {
		goto commit;
  	}

	
	seq = get_seqno_for_event(ctx, ev);
	if(edg_wll_LoadIntState(ctx, ev->any.jobId, 0 /* DONT_LOCK */, seq - 1, &ijsp)) {
		/* StepIntState is going to recompute the job status by itself */
		ijsp = NULL;
	}
	switch(edg_wll_FilterEvent(ctx, ijsp, ev, seq)) {
		
	case GLITE_LB_EVENT_FILTER_DROP:
		glite_common_log_msg(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_INFO, "event filtered out, dropping it");
		goto commit;

	case GLITE_LB_EVENT_FILTER_REPLACE:
		/* TODO: replace not implemented yet, store it as usual */

	default:
		ret = edg_wll_StoreEvent(ctx, ev, event, &seq);
		break;

	}
	if (ret ) {
		if (ret == EEXIST) edg_wll_ResetError(ctx);
		goto rollback;
	}
	
	if ( ev->any.type == EDG_WLL_EVENT_CHANGEACL ) {
		if (edg_wll_UpdateACL(ctx, ev->any.jobId,
			ev->changeACL.user_id, ev->changeACL.user_id_type,
			ev->changeACL.permission, ev->changeACL.permission_type,
			ev->changeACL.operation)) goto rollback;		
			
	}
	else {
#ifdef LB_PERF
		if(sink_mode == GLITE_LB_SINK_STATE) {
			glite_wll_perftest_consumeEvent(ev);
			goto commit;
    		}
#endif

  		if ( newstat.state )  {	/* prevent memleaks in case of transaction retry */
			edg_wll_FreeStatus(&newstat);
			newstat.state = EDG_WLL_JOB_UNDEF;
		}
		if (edg_wll_StepIntState(ctx,ev->any.jobId, ijsp, ev, seq, &oldstat, &newstat)) goto rollback;
		else {
			/* not needed anymore, but signalize we do not have to free the parts - this was done in StepIntState() */			
			if(ijsp) { free(ijsp); ijsp = NULL; }
		}
		
		if (proxy_purge && newstat.remove_from_proxy) 
			if (edg_wll_PurgeServerProxy(ctx, ev->any.jobId)) goto rollback;
	}

	if ((ev->any.type == EDG_WLL_EVENT_TAKEPAYLOADOWNERSHIP || ev->any.type == EDG_WLL_EVENT_GRANTPAYLOADOWNERSHIP) &&
		oldstat.payload_owner != newstat.payload_owner)
			edg_wll_UpdateACL(ctx, ev->any.jobId,
				newstat.payload_owner, EDG_WLL_CHANGEACL_DN,
				EDG_WLL_CHANGEACL_TAG, EDG_WLL_CHANGEACL_ALLOW,
				EDG_WLL_CHANGEACL_ADD);


	if (ev->any.type == EDG_WLL_EVENT_REGJOB &&
		(ev->regJob.jobtype == EDG_WLL_REGJOB_DAG ||
		 ev->regJob.jobtype == EDG_WLL_REGJOB_PARTITIONED ||
		 ev->regJob.jobtype == EDG_WLL_REGJOB_COLLECTION ||
		 ev->regJob.jobtype == EDG_WLL_REGJOB_FILE_TRANSFER_COLLECTION) &&
		ev->regJob.nsubjobs > 0) { 

			if (register_subjobs_embryonic(ctx,&ev->regJob)) goto rollback;
			reg_to_JP |= REG_SUBJOBS_TO_JP;
	}

	if (ev->any.type == EDG_WLL_EVENT_CONNECTJOB) {
		edg_wlc_JobId jobId = NULL;
		if (edg_wlc_JobIdParse(ev->connectJob.dest_jobid, &jobId)) {
			edg_wll_SetError(ctx, EDG_WLL_ERROR_JOBID_FORMAT, ev->connectJob.dest_jobid);
			goto err; /* should end here? */
		}
		// create connection A->B 
		edg_wll_jobsconnection_create(ctx, ev->any.jobId, jobId, get_jobtype_from_connectjob(ev->connectJob.jobtype), EDG_WLL_JOBCONNECTION_ACTIVE);
		// and log event to create B->A (if A->B is not logged by LB)
		if (ev->any.source != EDG_WLL_SOURCE_LB_SERVER)
			log_connectjob_event(ctx, jobId, ev->any.jobId, newstat.jobtype);

		edg_wlc_JobIdFree(jobId);
	}

	if (ev->any.type == EDG_WLL_EVENT_DISCONNECTJOB)  {
		edg_wlc_JobId jobId = NULL;
                if (edg_wlc_JobIdParse(ev->connectJob.dest_jobid, &jobId)) {
                        edg_wll_SetError(ctx, EDG_WLL_ERROR_JOBID_FORMAT, ev->connectJob.dest_jobid);
                        goto err; /* should end here? */
                }

		// invalidate connection A->B
		edg_wll_jobsconnection_modify(ctx, ev->any.jobId, jobId, EDG_WLL_JOBCONNECTION_CANCELLED);
		// and log event to invalidate B->A (if A->B is not logged by LB)
                if (ev->any.source != EDG_WLL_SOURCE_LB_SERVER)
                        log_disconnectjob_event(ctx, jobId, ev->any.jobId);
		edg_wlc_JobIdFree(jobId);
	}

	// job is going to halt, set connections to inactive
	if ((newstat.state != oldstat.state) && ((newstat.state == EDG_WLL_JOB_DONE) || (newstat.state == EDG_WLL_JOB_ABORTED) || (newstat.state == EDG_WLL_JOB_CANCELLED)))
		edg_wll_jobsconnection_modifyall(ctx, ev->any.jobId, EDG_WLL_JOBCONNECTION_ACTIVE, EDG_WLL_JOBCONNECTION_INACTIVE);

commit:
rollback:;
	if(ijsp) {  destroy_intJobStat(ijsp); free(ijsp); ijsp = NULL; }

  } while (edg_wll_TransNeedRetry(ctx));

  if (edg_wll_Error(ctx, NULL, NULL)) goto err;


  db_store_finalize(ctx, event, ev, &oldstat, &newstat, reg_to_JP);


err:
  if(ijsp) free(ijsp); 
  if(ev) { edg_wll_FreeEvent(ev); free(ev); }
  if ( newstat.state ) edg_wll_FreeStatus(&newstat);
  if ( oldstat.state ) edg_wll_FreeStatus(&oldstat);

  return edg_wll_Error(ctx,NULL,NULL);
}



/* Called only when CollectionStateEvent generated */
int
db_parent_store(edg_wll_Context ctx, edg_wll_Event *ev, intJobStat *is)
{
  char  *event = NULL;
  int	seq;
  int   err;
  edg_wll_JobStat	newstat;
  edg_wll_JobStat	oldstat;


  edg_wll_ResetError(ctx);
  memset(&newstat,0,sizeof newstat);
  memset(&oldstat,0,sizeof oldstat);

  /* Transaction opened from db_store */

#ifdef LB_PERF
  if (sink_mode == GLITE_LB_SINK_STORE) {
	  glite_wll_perftest_consumeEvent(ev);
	  edg_wll_FreeEvent(ev);
	  free(ev);
	  return 0;
  }
#endif


  assert(ev->any.user);

    // locked from edg_wll_LoadIntState() <- load_parent_intJobStat() <- update_parent_status()
    // XXX: maybe it can be locked InShareMode there and re-locked ForUpdate here?

    if(edg_wll_StoreEvent(ctx, ev, NULL, &seq))
      goto err;

#ifdef LB_PERF
  if(sink_mode == GLITE_LB_SINK_STATE) {
	     glite_wll_perftest_consumeEvent(ev);
	     goto err;
  }
#endif

  err = edg_wll_StepIntStateParent(ctx,ev->any.jobId, ev, seq, is, &oldstat, ctx->isProxy? NULL: &newstat);

  if (err) goto err;

  if ( ctx->isProxy ) {
    event = edg_wll_UnparseEvent(ctx, ev);
    assert(event);
  }

  db_store_finalize(ctx, event, ev, &oldstat, &newstat, 0);

err:

  free(event);
  if ( newstat.state ) edg_wll_FreeStatus(&newstat);
  if ( oldstat.state ) edg_wll_FreeStatus(&oldstat);
  
  return edg_wll_Error(ctx,NULL,NULL);
}


/* Send regitration to JP 
 */
static int register_to_JP(edg_wll_Context ctx, edg_wlc_JobId jobid, char *user)
{
	char *jids, *msg;
	
	
	if ( !(jids = edg_wlc_JobIdUnparse(jobid)) ) {
		return edg_wll_SetError(ctx, errno, "Can't unparse jobid when registering to JP");
	}
	if ( !(msg = calloc(strlen(jids)+strlen(user)+2, sizeof(char) )) ) {
		free(jids);
		return edg_wll_SetError(ctx, errno, "Can't allocate buffer when registering to JP");
	}
	strcat(msg, jids);
	free(jids);
	strcat(msg, "\n");
	strcat(msg, user);
	if ( glite_lbu_MaildirStoreMsg(ctx->jpreg_dir, ctx->srvName, msg) ) {
		free(msg);
		return edg_wll_SetError(ctx, errno, lbm_errdesc);
	}
	free(msg);

	return edg_wll_Error(ctx,NULL,NULL);
}


static int register_subjobs_to_JP(edg_wll_Context ctx, edg_wll_Event *ev)
{
	edg_wlc_JobId	*subjobs = NULL;
	int		i = 0, j;


	if (edg_wll_GenerateSubjobIds(ctx, ev->regJob.jobId, 
			ev->regJob.nsubjobs, ev->regJob.seed, &subjobs)) 
		goto err;

	for (i=0; i<ev->regJob.nsubjobs; i++) {
		if (register_to_JP(ctx, subjobs[i], ev->any.user))
			goto err;
	}

err:
	for (j=i; j<ev->regJob.nsubjobs; j++) edg_wlc_JobIdFree(subjobs[j]);
	free(subjobs);

	return edg_wll_Error(ctx,NULL,NULL);
}


static int forward_event_to_server(edg_wll_Context ctx, char *event, edg_wll_Event *ev, int local_job)
{
	if ( ctx->isProxy ) {
		/*
		 *	send event to the proper BK server
		 *	event with priority flag EDG_WLL_LOGFLAG_DIRECT (typically RegJob) is not sent
		 */

		/* XXX: ending here may break the backward compatibility */
		if (!(ev->any.priority & EDG_WLL_LOGFLAG_PROXY)) {
			edg_wll_UpdateError(ctx, 0, "db_actual_store() WARNING: the event is not PROXY");
			//return edg_wll_SetError(ctx, EDG_WLL_IL_PROTO, "db_actual_store() ERROR: the event is not PROXY");
		}

		if (!(ev->any.priority & (EDG_WLL_LOGFLAG_DIRECT | EDG_WLL_LOGFLAG_INTERNAL)) && !local_job) {
			if (edg_wll_EventSendProxy(ctx, ev->any.jobId, event) )  {
				return edg_wll_SetError(ctx, EDG_WLL_IL_PROTO, "edg_wll_EventSendProxy() error.");
			}
		}
	}

	return edg_wll_Error(ctx,NULL,NULL);
}


static int db_store_finalize(edg_wll_Context ctx, char *event, edg_wll_Event *ev, edg_wll_JobStat *oldstat, edg_wll_JobStat *newstat, int reg_to_JP) 
{
	int 	local_job = is_job_local(ctx, ev->any.jobId);


#ifdef LB_PERF
	if( sink_mode == GLITE_LB_SINK_SEND ) {
		glite_wll_perftest_consumeEvent(ev);
		return edg_wll_Error(ctx,NULL,NULL);
	}
#endif
	
	if (ctx->jpreg_dir) {
		if (reg_to_JP & REG_JOB_TO_JP) 
			if (register_to_JP(ctx,ev->any.jobId,ev->any.user)) goto err;
		if (reg_to_JP & REG_SUBJOBS_TO_JP)
			if (register_subjobs_to_JP(ctx,ev)) goto err;
	}

	if (forward_event_to_server(ctx, event, ev, local_job)) goto err;
	
	if (newstat->state) {
		if ( ctx->isProxy ) {
			if ((ev->any.priority & EDG_WLL_LOGFLAG_DIRECT) || local_job) 
				/* event will not arrive to server, only flag was set		*/
				/* check whether some pending notifications are not triggered	*/
				edg_wll_NotifMatch(ctx, oldstat, newstat);
			}
		else {
				edg_wll_NotifMatch(ctx, oldstat, newstat);
		}
	}

err:
	return edg_wll_Error(ctx,NULL,NULL);
}

static int log_connectjob_event(edg_wll_Context ctx, glite_jobid_const_t jobif_from, glite_jobid_const_t jobid_to, enum edg_wll_StatJobtype jobtype){
	int     ret = 0;
	char	*event_str;

	edg_wll_Event  *event = edg_wll_InitEvent(EDG_WLL_EVENT_CONNECTJOB);

	event->any.priority = EDG_WLL_LOGFLAG_INTERNAL;
	if (ctx->serverIdentity)
		event->any.user = strdup(ctx->serverIdentity);
	else
		event->any.user = strdup("LBProxy");
	event->any.seqcode = edg_wll_GetSequenceCode(ctx);
	edg_wlc_JobIdDup(jobif_from, &(event->any.jobId));
	gettimeofday(&event->any.timestamp,0);
	event->any.source = EDG_WLL_SOURCE_LB_SERVER;

	event->connectJob.dest_jobid = edg_wlc_JobIdUnparse(jobid_to);
	event->connectJob.jobtype = get_connectjobtype_from_jobtype(jobtype);

	event_str = edg_wll_UnparseEvent(ctx, event);

	ret = db_store(ctx, event_str);

	edg_wll_FreeEvent(event);
        free(event);
	free(event_str);

	return ret;
}

static int log_disconnectjob_event(edg_wll_Context ctx, glite_jobid_const_t jobif_from, glite_jobid_const_t jobid_to){
        int     ret = 0;
        char    *event_str;

        edg_wll_Event  *event = edg_wll_InitEvent(EDG_WLL_EVENT_DISCONNECTJOB);

        event->any.priority = EDG_WLL_LOGFLAG_INTERNAL;
        if (ctx->serverIdentity)
                event->any.user = strdup(ctx->serverIdentity);
        else
                event->any.user = strdup("LBProxy");
        event->any.seqcode = edg_wll_GetSequenceCode(ctx);
        edg_wlc_JobIdDup(jobif_from, &(event->any.jobId));
        gettimeofday(&event->any.timestamp,0);
        event->any.source = EDG_WLL_SOURCE_LB_SERVER;

        event->connectJob.dest_jobid = edg_wlc_JobIdUnparse(jobid_to);

        event_str = edg_wll_UnparseEvent(ctx, event);

        ret = db_store(ctx, event_str);

        edg_wll_FreeEvent(event);
        free(event);
        free(event_str);

        return ret;
}


/* this is ugly, however, shift to match second structure could make incompatibility when any of them is permuted */
static enum edg_wll_StatJobtype get_jobtype_from_connectjob(enum edg_wll_ConnectJobJobtype  jobtype){
	enum edg_wll_StatJobtype ret;

	switch(jobtype){
		case EDG_WLL_CONNECTJOB_SIMPLE:
			ret = EDG_WLL_STAT_SIMPLE;
			break;
		case EDG_WLL_CONNECTJOB_DAG:
			ret = EDG_WLL_STAT_DAG;
			break;
		case EDG_WLL_CONNECTJOB_PARTITIONABLE:
			ret = EDG_WLL_STAT__PARTITIONABLE_UNUSED; /* for completeness */
			break;
		case EDG_WLL_CONNECTJOB_PARTITIONED:
			ret = EDG_WLL_STAT__PARTITIONED_UNUSED; /* for completeness */
			break;
		case EDG_WLL_CONNECTJOB_COLLECTION:
			ret = EDG_WLL_STAT_COLLECTION;
			break;
		case EDG_WLL_CONNECTJOB_PBS:
			ret = EDG_WLL_STAT_PBS;
			break;
		case EDG_WLL_CONNECTJOB_CONDOR:
			ret = EDG_WLL_STAT_CONDOR;
			break;
		case EDG_WLL_CONNECTJOB_CREAM:
			ret = EDG_WLL_STAT_CREAM;
			break;
		case EDG_WLL_CONNECTJOB_FILE_TRANSFER_COLLECTION:
			ret = EDG_WLL_STAT_FILE_TRANSFER_COLLECTION;
			break;
		case EDG_WLL_CONNECTJOB_FILE_TRANSFER:
			ret = EDG_WLL_STAT_FILE_TRANSFER;
			break;
		case EDG_WLL_CONNECTJOB_VIRTUAL_MACHINE:
			ret = EDG_WLL_STAT_VIRTUAL_MACHINE;
			break;
		default :
			break;
	}

	return ret;
}

static enum edg_wll_ConnectJobJobtype get_connectjobtype_from_jobtype(enum edg_wll_StatJobtype jobtype){
	enum edg_wll_ConnectJobJobtype ret;

	switch(jobtype){
                case EDG_WLL_STAT_SIMPLE:
                        ret = EDG_WLL_CONNECTJOB_SIMPLE;
                        break;
                case EDG_WLL_STAT_DAG:
                        ret = EDG_WLL_CONNECTJOB_DAG;
                        break;
                case EDG_WLL_STAT__PARTITIONABLE_UNUSED:
                        ret = EDG_WLL_CONNECTJOB_PARTITIONABLE; /* for completeness */
                        break;
                case EDG_WLL_STAT__PARTITIONED_UNUSED:
                        ret = EDG_WLL_CONNECTJOB_PARTITIONED; /* for completeness */
                        break;
                case EDG_WLL_STAT_COLLECTION:
                        ret = EDG_WLL_CONNECTJOB_COLLECTION;
                        break;
                case EDG_WLL_STAT_PBS:
                        ret = EDG_WLL_CONNECTJOB_PBS;
                        break;
                case EDG_WLL_STAT_CONDOR:
                        ret = EDG_WLL_CONNECTJOB_CONDOR;
                        break;
                case EDG_WLL_STAT_CREAM:
                        ret = EDG_WLL_CONNECTJOB_CREAM;
                        break;
                case EDG_WLL_STAT_FILE_TRANSFER_COLLECTION:
                        ret = EDG_WLL_CONNECTJOB_FILE_TRANSFER_COLLECTION;
                        break;
                case EDG_WLL_STAT_FILE_TRANSFER:
                        ret = EDG_WLL_CONNECTJOB_FILE_TRANSFER;
                        break;
                case EDG_WLL_STAT_VIRTUAL_MACHINE:
                        ret = EDG_WLL_CONNECTJOB_VIRTUAL_MACHINE;
                        break;
                default :
                        break;
        }

	return ret;
}


static int get_seqno_for_event(edg_wll_Context ctx, edg_wll_Event *e)
{
	char			*jobid, *stmt, *max;
	glite_lbu_Statement	sh = NULL;
	int                     next = 0;

	jobid = stmt = max = NULL;

/* obtain number of stored events */
	jobid = edg_wlc_JobIdGetUnique(e->any.jobId);
	trio_asprintf(&stmt,
		"select nevents from jobs "
		"where jobid = '%|Ss'",jobid);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	if (edg_wll_ExecSQL(ctx,stmt,&sh) < 0 ||
	    edg_wll_FetchRow(ctx,sh,1,NULL,&max) < 0) goto clean;
	next = (max && *max) ? atoi(max) : 0;
	free(max);

clean:
	if(sh) glite_lbu_FreeStmt(&sh); 
	free(stmt);

	return next;
}
