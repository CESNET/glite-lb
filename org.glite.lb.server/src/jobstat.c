#ident "$Header$"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <regex.h>
#include <syslog.h>

#include "glite/lb/producer.h"
#include "glite/lb/context-int.h"
#include "glite/lb/trio.h"

#include "get_events.h"
#include "store.h"
#include "lock.h"
#include "index.h"
#include "jobstat.h"
#include "lb_authz.h"


#define DAG_ENABLE	1

/* TBD: share in whole logging or workload */
#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif
  
#define RET_FAIL	0
#define RET_OK		1
#define RET_FATAL	RET_FAIL
#define RET_SOON	2
#define RET_LATE	3
#define RET_BADSEQ	4
#define RET_SUSPECT	5
#define RET_IGNORE	6
#define RET_INTERNAL	100

#define rep(a,b) { free(a); a = (b == NULL) ? NULL : strdup(b); }
#define mov(a,b) { free(a); a = b; b = NULL; }

static void warn (const char* format, ...) UNUSED_VAR ;
static char *job_owner(edg_wll_Context,char *);
static char* location_string(const char*, const char*, const char*);
static int add_stringlist(char ***, const char *) UNUSED_VAR;
static int add_taglist(edg_wll_TagValue **, const char *, const char *);


int edg_wll_intJobStatus(edg_wll_Context, const edg_wlc_JobId, int, intJobStat *, int);
edg_wll_ErrorCode edg_wll_StoreIntState(edg_wll_Context, intJobStat *, int);
edg_wll_ErrorCode edg_wll_LoadIntState(edg_wll_Context , edg_wlc_JobId , int, intJobStat **);

int js_enable_store = 1;

/*
 * Basic manipulations with the internal representation of job state
 */

static void init_intJobStat(intJobStat *p)
{
	memset(p, 0, sizeof(intJobStat));
	p->pub.jobtype = EDG_WLL_STAT_SIMPLE;
	p->pub.children_hist = (int*) calloc(1+EDG_WLL_NUMBER_OF_STATCODES, sizeof(int));
	p->pub.children_hist[0] = EDG_WLL_NUMBER_OF_STATCODES;
	p->pub.stateEnterTimes = (int*) calloc(1+EDG_WLL_NUMBER_OF_STATCODES, sizeof(int));
	p->pub.stateEnterTimes[0] = EDG_WLL_NUMBER_OF_STATCODES;
	/* TBD: generate */
}

static void destroy_intJobStat_extension(intJobStat *p)
{
	free(p->last_seqcode); p->last_seqcode = NULL;
	free(p->last_cancel_seqcode); p->last_cancel_seqcode = NULL;
			       p->wontresub = 0;
}

void destroy_intJobStat(intJobStat *p)
{
	edg_wll_FreeStatus(&p->pub);
	destroy_intJobStat_extension(p);
	memset(p, 0, sizeof(intJobStat));
}

#if 0
static int eval_expect_update(intJobStat *, int *, char **);
#endif

static int processEvent(intJobStat *, edg_wll_Event *, int, int, char **);

static char* matched_substr(char *, regmatch_t) UNUSED_VAR;

static char* matched_substr(char *in, regmatch_t match)
{
	int len;
	char *s;

	len = match.rm_eo - match.rm_so;
	s = calloc(1, len + 1);
	if (s != NULL) {
		strncpy(s, in + (int)match.rm_so, len);
	}

	return s;
}


int edg_wll_JobStatus(
	edg_wll_Context	ctx,
	const edg_wlc_JobId		job,
	int		flags,
	edg_wll_JobStat	*stat)
{

/* Local variables */
	char		*string_jobid;
	char		*md5_jobid;

	intJobStat	jobstat;
	intJobStat	*ijsp;
	int		intErr = 0;
	int		lockErr;
	edg_wll_Acl	acl = NULL;
#if DAG_ENABLE	
	char		*stmt = NULL;
#endif	

	edg_wll_ResetError(ctx);

	string_jobid = edg_wlc_JobIdUnparse(job);
	if (string_jobid == NULL || stat == NULL)
		return edg_wll_SetError(ctx,EINVAL, NULL);
	md5_jobid = edg_wlc_JobIdGetUnique(job);

	if ( !(jobstat.pub.owner = job_owner(ctx,md5_jobid)) ) {
		free(md5_jobid);
		free(string_jobid);
		return edg_wll_Error(ctx,NULL,NULL);
	}

	intErr = edg_wll_GetACL(ctx, job, &acl);
	if (intErr) {
		free(md5_jobid);
		free(string_jobid);
		return edg_wll_Error(ctx,NULL,NULL);
	}

	/* authorization check */
	if ( !(ctx->noAuth) &&
	    (!(ctx->peerName) ||  strcmp(ctx->peerName, jobstat.pub.owner))) {
	      	intErr = (acl == NULL) || edg_wll_CheckACL(ctx, acl, EDG_WLL_PERM_READ);
	      if (intErr) {
		 free(string_jobid);
		 free(md5_jobid);
		 free(jobstat.pub.owner); jobstat.pub.owner = NULL;
	 	 if (acl) {
			edg_wll_FreeAcl(acl);
		 	return edg_wll_Error(ctx, NULL, NULL);
		 } else {
			return edg_wll_SetError(ctx,EPERM, "not owner, no ACL is set");
		 }
	      }
	}

	intErr = edg_wll_LoadIntState(ctx, job, -1 /*all events*/, &ijsp);
	if (!intErr) {
		*stat = ijsp->pub;
		destroy_intJobStat_extension(ijsp);
		free(ijsp);

	} else {
		lockErr = edg_wll_LockJob(ctx,job);
		intErr = edg_wll_intJobStatus(ctx, job, flags,&jobstat, js_enable_store && !lockErr);
		if (!lockErr) {
			edg_wll_UnlockJob(ctx,job);
		}
	
		*stat = jobstat.pub;
		if (intErr) edg_wll_FreeStatus(&jobstat.pub);
		destroy_intJobStat_extension(&jobstat);
	}
	
	if (intErr) {
		free(string_jobid);
		free(md5_jobid);
		if (acl) edg_wll_FreeAcl(acl);
		return edg_wll_Error(ctx, NULL, NULL);
	}

	if (acl) {
		stat->acl = strdup(acl->string);
		edg_wll_FreeAcl(acl);
	}

	if ((flags & EDG_WLL_STAT_CLASSADS) == 0) {
		char *null = NULL;

		mov(stat->jdl, null);
		mov(stat->matched_jdl, null);
		mov(stat->condor_jdl, null);
		mov(stat->rsl, null);
	}

#if DAG_ENABLE
	if (stat->jobtype == EDG_WLL_STAT_DAG) {
		if (1) {
			char *out[2];
			edg_wll_Stmt sh;
			int num_sub, num_f;

			if (stat->children_hist == NULL) {
				stat->children_hist = (int*) calloc(1+EDG_WLL_NUMBER_OF_STATCODES, sizeof(int));
				stat->children_hist[0] = EDG_WLL_NUMBER_OF_STATCODES;
			}
			if ((flags & EDG_WLL_STAT_CHILDREN) == 0) {
				trio_asprintf(&stmt, "SELECT status FROM states "
							"WHERE parent_job='%|Ss' AND version='%|Ss'",
					md5_jobid, INTSTAT_VERSION);
				out[1] = NULL;
			} else {
				trio_asprintf(&stmt, "SELECT s.status,j.dg_jobid FROM states s,jobs j "
						"WHERE s.parent_job='%|Ss' AND s.version='%|Ss' AND s.jobid=j.jobid",
					md5_jobid, INTSTAT_VERSION);
			}
			if (stmt != NULL) {
				num_sub = edg_wll_ExecStmt(ctx, stmt, &sh);
				if (num_sub >=0 ) {
					while ((num_f = edg_wll_FetchRow(sh, out)) == 1
							|| (num_f == 2)) {
						num_f = atoi(out[0]);
						if (num_f > EDG_WLL_JOB_UNDEF && num_f < EDG_WLL_NUMBER_OF_STATCODES)
							stat->children_hist[num_f+1]++;
						if (out[1] !=NULL) add_stringlist(&stat->children, out[1]);
						free(out[0]); free(out[1]);
					}
					edg_wll_FreeStmt(&sh);
				}
				free(stmt);
			} else goto dag_enomem;
		}
		if (flags & EDG_WLL_STAT_CHILDSTAT) {
			char *stat_str, *s_out;
			edg_wll_Stmt sh;
			int num_sub, num_f, i;
			intJobStat *js;

			trio_asprintf(&stmt, "SELECT int_status FROM states WHERE parent_job='%|Ss'"
						" AND version='%|Ss'",
					md5_jobid, INTSTAT_VERSION);
			if (stmt != NULL) {
				num_sub = edg_wll_ExecStmt(ctx, stmt, &sh);
				if (num_sub >=0 ) {
					i = 0;
					stat->children_states = calloc(num_sub+1, sizeof(edg_wll_JobStat));
					if (stat->children_states == NULL) {
						edg_wll_FreeStmt(&sh);
						goto dag_enomem;
					}
					while ((num_f = edg_wll_FetchRow(sh, &stat_str)) == 1
						&& i < num_sub) {
						js = dec_intJobStat(stat_str, &s_out);
						if (s_out != NULL && js != NULL) {
							stat->children_states[i] = js->pub;
							destroy_intJobStat_extension(js);
							free(js);
							i++;
						}
						free(stat_str);
					}
					edg_wll_FreeStmt(&sh);
				}
				free(stmt);
			} else goto dag_enomem;
		}
	}
#endif
	free(string_jobid);
	free(md5_jobid);
	return edg_wll_Error(ctx, NULL, NULL);

#if DAG_ENABLE
dag_enomem:
	free(string_jobid);
	free(md5_jobid);
	edg_wll_FreeStatus(stat);
	free(stmt);
	return edg_wll_SetError(ctx, ENOMEM, NULL);
#endif
}

int edg_wll_intJobStatus(
	edg_wll_Context	ctx,
	const edg_wlc_JobId	job,
	int			flags,
	intJobStat	*intstat,
	int		update_db)
{

/* Local variables */
	char		*string_jobid;
	char		*md5_jobid;

	int		num_events;
	edg_wll_Event	*events = NULL;

	int		i, intErr = 0;
	int		res;
	int		be_strict = 0;
	char		*errstring = NULL;

	edg_wll_QueryRec	jqr[2];
	edg_wll_QueryRec        **jqra;

/* Processing */
	edg_wll_ResetError(ctx);
	init_intJobStat(intstat);

	string_jobid = edg_wlc_JobIdUnparse(job);
	if (string_jobid == NULL || intstat == NULL)
		return edg_wll_SetError(ctx,EINVAL, NULL);

	/* can be already filled by public edg_wll_JobStat() */
	if (intstat->pub.owner == NULL) {
		md5_jobid = edg_wlc_JobIdGetUnique(job);
		if ( !(intstat->pub.owner = job_owner(ctx,md5_jobid)) ) {
			free(md5_jobid);
			free(string_jobid);
			return edg_wll_Error(ctx,NULL,NULL);
		}
		free(md5_jobid);
	}
	
        jqr[0].attr = EDG_WLL_QUERY_ATTR_JOBID;
        jqr[0].op = EDG_WLL_QUERY_OP_EQUAL;
        jqr[0].value.j = job;
        jqr[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	jqra = (edg_wll_QueryRec **) malloc (2 * sizeof(edg_wll_QueryRec **));
	jqra[0] = jqr;
	jqra[1] = NULL;

	if (edg_wll_QueryEventsServer(ctx,1, (const edg_wll_QueryRec **)jqra, NULL, &events)) {
		free(string_jobid);
		free(jqra);
                return edg_wll_Error(ctx, NULL, NULL);
	}
	free(jqra);

	for (num_events = 0; events[num_events].type != EDG_WLL_EVENT_UNDEF;
		num_events++);

	if (num_events == 0) {
		free(string_jobid);
		return edg_wll_SetError(ctx,ENOENT,NULL);
	}

	edg_wll_SortEvents(events);

	for (i = 0; i < num_events; i++) {
		res = processEvent(intstat, &events[i], i, be_strict, &errstring);
		if (res == RET_FATAL || res == RET_INTERNAL) { /* !strict */
			intErr = 1; break;
		}
	}
	if (intstat->pub.state == EDG_WLL_JOB_UNDEF) {
		intstat->pub.state = EDG_WLL_JOB_UNKNOWN;
	}

	free(string_jobid);

	for (i=0; i < num_events ; i++) edg_wll_FreeEvent(&events[i]);
	free(events);


	if (intErr) {
		destroy_intJobStat(intstat);
		return edg_wll_SetError(ctx, EDG_WLL_ERROR_SERVER_RESPONSE, NULL);
	} else {
		/* XXX intstat->pub.expectUpdate = eval_expect_update(intstat, &intstat->pub.expectFrom); */
		intErr = edg_wlc_JobIdDup(job, &intstat->pub.jobId);
		if (!intErr) {
			if (update_db) {
				int tsq = num_events - 1;
				edg_wll_StoreIntState(ctx, intstat, tsq);
				/* recheck
				 * intJobStat *reread;
				 * edg_wll_LoadIntState(ctx, job, tsq, &reread);
				 * destroy_intJobStat(reread);
				*/
			}
		}
		return edg_wll_SetError(ctx, intErr, NULL);
	}

}

static int badEvent(intJobStat *js UNUSED_VAR, edg_wll_Event *e, int ev_seq UNUSED_VAR)
{
	char *str;

	str = edg_wll_EventToString(e->any.type);
	fprintf(stderr, "edg_wll_JobStatus:  bad event: type %d (%s)\n",
		e->any.type, (str == NULL) ? "unknown" : str);
	free(str);
	return RET_FATAL;
}

#define USABLE(res,strict) ((res) == RET_OK || ( (res) == RET_SOON && !strict))
#define USABLE_DATA(res,strict) ((res) == RET_OK || ( (res) != RET_FATAL && !strict))
#define LRMS_STATE(state) ((state) == EDG_WLL_JOB_RUNNING || (state) == EDG_WLL_JOB_DONE)


static int processEvent(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{

	edg_wll_JobStatCode	old_state = js->pub.state;
	edg_wll_JobStatCode	new_state = EDG_WLL_JOB_UNKNOWN;
	int		res = RET_OK;

	if (old_state == EDG_WLL_JOB_ABORTED ||
		old_state == EDG_WLL_JOB_CANCELLED ||
		old_state == EDG_WLL_JOB_CLEARED) {
		res = RET_LATE;
	}

	if (js->last_seqcode != NULL &&
		edg_wll_compare_seq(e->any.seqcode, js->last_seqcode) < 0) {
		res = RET_LATE;
	}

	switch (e->any.type) {
		case EDG_WLL_EVENT_TRANSFER:
			if (e->transfer.result == EDG_WLL_TRANSFER_OK) {
				switch (e->transfer.source) {
					case EDG_WLL_SOURCE_USER_INTERFACE:
						new_state = EDG_WLL_JOB_WAITING; break;
					case EDG_WLL_SOURCE_JOB_SUBMISSION:
						/* if (LRMS_STATE(old_state)) res = RET_LATE; */
						new_state = EDG_WLL_JOB_READY; break;
					case EDG_WLL_SOURCE_LOG_MONITOR:
						if (LRMS_STATE(old_state)) {
							js->pub.stateEnterTimes[1 + EDG_WLL_JOB_SCHEDULED] =
								e->any.timestamp.tv_sec;
							res = RET_LATE;
						}
						new_state = EDG_WLL_JOB_SCHEDULED; break;
					default:
						goto bad_event; break;
				}
			} else if (e->transfer.result == EDG_WLL_TRANSFER_FAIL) {
				/* transfer failed */ 
				switch (e->transfer.source) {
					case EDG_WLL_SOURCE_USER_INTERFACE:
						new_state = EDG_WLL_JOB_SUBMITTED; break;
					case EDG_WLL_SOURCE_JOB_SUBMISSION:
						if (LRMS_STATE(old_state)) res = RET_LATE;
						new_state = EDG_WLL_JOB_READY; break;
					case EDG_WLL_SOURCE_LOG_MONITOR:
						if (LRMS_STATE(old_state)) res = RET_LATE;
						new_state = EDG_WLL_JOB_READY; break;
					default:
						goto bad_event; break;
				}
			} else {
				/* e->transfer.result == EDG_WLL_TRANSFER_START */
				res = RET_IGNORE;
			}
			if (USABLE(res, strict)) {
				js->pub.state = new_state;
				rep(js->pub.reason, e->transfer.reason);

				free(js->pub.location);
				if (e->transfer.result == EDG_WLL_TRANSFER_OK) {
					js->pub.location = location_string(
						edg_wll_SourceToString(e->transfer.destination),
						e->transfer.dest_host,
						e->transfer.dest_instance);
				} else {
					js->pub.location = location_string(
						edg_wll_SourceToString(e->transfer.source),
						e->transfer.host,
						e->transfer.src_instance);
				}
			}
			if (USABLE_DATA(res, strict)) {
				switch (e->transfer.source) {
					case EDG_WLL_SOURCE_USER_INTERFACE:
						rep(js->pub.jdl, e->transfer.job); break;
					case EDG_WLL_SOURCE_JOB_SUBMISSION:
						rep(js->pub.condor_jdl, e->transfer.job); break;
					case EDG_WLL_SOURCE_LOG_MONITOR:
						rep(js->pub.rsl, e->transfer.job); break;
					default:
						goto bad_event; break;
						
				}
			}
			break;
		case EDG_WLL_EVENT_ACCEPTED:
			switch (e->accepted.source) {
				case EDG_WLL_SOURCE_NETWORK_SERVER:
					new_state = EDG_WLL_JOB_WAITING; break;
				case EDG_WLL_SOURCE_LOG_MONITOR:
					if (LRMS_STATE(old_state)) res = RET_LATE;
					new_state = EDG_WLL_JOB_READY; break;
				case EDG_WLL_SOURCE_LRMS:
					new_state = EDG_WLL_JOB_SCHEDULED; break;
				default:
					goto bad_event; break;
			}
			if (USABLE(res, strict)) {
				js->pub.state = new_state;
				free(js->pub.location);
				js->pub.location = location_string(
						edg_wll_SourceToString(e->accepted.source),
						e->accepted.host,
						e->accepted.src_instance);
			}
			if (USABLE_DATA(res, strict)) {
				switch (e->accepted.source) {
					case EDG_WLL_SOURCE_NETWORK_SERVER:
						break; /* no WM id */
					case EDG_WLL_SOURCE_LOG_MONITOR:
						rep(js->pub.condorId, e->accepted.local_jobid); break;
					case EDG_WLL_SOURCE_LRMS:
						/* XXX localId */
						rep(js->pub.globusId, e->accepted.local_jobid); break;
					default:
						goto bad_event; break;
				}
			}
			break;
		case EDG_WLL_EVENT_REFUSED:
			switch (e->refused.source) {
				case EDG_WLL_SOURCE_NETWORK_SERVER:
					new_state = EDG_WLL_JOB_SUBMITTED; break;
				case EDG_WLL_SOURCE_LOG_MONITOR:
					new_state = EDG_WLL_JOB_READY; break;
				case EDG_WLL_SOURCE_LRMS:
					new_state = EDG_WLL_JOB_READY; break;
				default:
					goto bad_event; break;
			}
			if (USABLE(res, strict)) {
				js->pub.state = new_state;
				rep(js->pub.reason, e->refused.reason);

				free(js->pub.location);
				js->pub.location = location_string(
						edg_wll_SourceToString(e->refused.from),
						e->refused.from_host,
						e->refused.from_instance);
			}
			break;
		case EDG_WLL_EVENT_ENQUEUED:
			if (e->enQueued.result == EDG_WLL_ENQUEUED_OK) {
				switch (e->enQueued.source) {
					case EDG_WLL_SOURCE_NETWORK_SERVER:
						new_state = EDG_WLL_JOB_WAITING; break;
					case EDG_WLL_SOURCE_WORKLOAD_MANAGER:
						if (LRMS_STATE(old_state)) res = RET_LATE;
						new_state = EDG_WLL_JOB_READY; break;
					case EDG_WLL_SOURCE_LOG_MONITOR:
						new_state = EDG_WLL_JOB_WAITING; break;
					default:
						goto bad_event; break;
				}
			} else if (e->enQueued.result == EDG_WLL_ENQUEUED_FAIL) {
				switch (e->enQueued.source) {
					case EDG_WLL_SOURCE_NETWORK_SERVER:
						new_state = EDG_WLL_JOB_WAITING; break;
					case EDG_WLL_SOURCE_WORKLOAD_MANAGER:
						new_state = EDG_WLL_JOB_WAITING; break;
					case EDG_WLL_SOURCE_LOG_MONITOR:
						new_state = old_state; break;
					default:
						goto bad_event; break;
				}
			} else { 
				/* e->enQueued.result == EDG_WLL_ENQUEUED_START */
				res = RET_IGNORE;
			}
			if (USABLE(res, strict)) {
				js->pub.state = new_state;
				rep(js->pub.reason, e->enQueued.reason);

				free(js->pub.location);
				if (e->enQueued.result == EDG_WLL_ENQUEUED_OK) {
					js->pub.location = location_string(
						e->enQueued.queue,
						e->enQueued.host,
						e->enQueued.src_instance);
					if (e->enQueued.source == EDG_WLL_SOURCE_LOG_MONITOR)
						js->pub.resubmitted = 1;
				} else {
					js->pub.location = location_string(
						edg_wll_SourceToString(e->enQueued.source),
						e->enQueued.host,
						e->enQueued.src_instance);
				}
			}
			if (USABLE_DATA(res, strict)) {
				switch (e->enQueued.source) {
					case EDG_WLL_SOURCE_NETWORK_SERVER:
						rep(js->pub.jdl, e->enQueued.job); break;
					case EDG_WLL_SOURCE_WORKLOAD_MANAGER:
						rep(js->pub.matched_jdl,  e->enQueued.job); break;
					case EDG_WLL_SOURCE_LOG_MONITOR:
						/* no interim JDL here */
						break;
					default:
						goto bad_event; break;
				}
			}
			break;
		case EDG_WLL_EVENT_DEQUEUED:
			switch (e->deQueued.source) {
				case EDG_WLL_SOURCE_WORKLOAD_MANAGER:
					new_state = EDG_WLL_JOB_WAITING; break;
				case EDG_WLL_SOURCE_JOB_SUBMISSION:
					if (LRMS_STATE(old_state)) res = RET_LATE;
					new_state = EDG_WLL_JOB_READY; break;
				default:
					goto bad_event; break;
			}
			if (USABLE(res, strict)) {
				js->pub.state = new_state;
				free(js->pub.location);
				js->pub.location = location_string(
						edg_wll_SourceToString(e->deQueued.source),
						e->deQueued.host,
						e->deQueued.src_instance);
			}
			if (USABLE_DATA(res, strict)) {
				/* no WM/JSS local jobid */
			}
			break;
		case EDG_WLL_EVENT_HELPERCALL:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_WAITING;
				free(js->pub.location);
				js->pub.location = location_string(
						e->helperCall.helper_name,
						e->helperCall.host,
						e->helperCall.src_instance);
				/* roles and params used only for debugging */
			}
			break;
		case EDG_WLL_EVENT_HELPERRETURN:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_WAITING;
				free(js->pub.location);
				js->pub.location = location_string(
						edg_wll_SourceToString(EDG_WLL_SOURCE_WORKLOAD_MANAGER),
						e->helperReturn.host,
						e->helperReturn.src_instance);
				/* roles and retvals used only for debugging */
			}
			break;
		case EDG_WLL_EVENT_RUNNING:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_RUNNING;
				free(js->pub.location);
				js->pub.location = location_string(
						edg_wll_SourceToString(EDG_WLL_SOURCE_LRMS),
						"worknode",
						e->running.node);
			}
			if (USABLE_DATA(res, strict)) {
				rep(js->pub.ce_node, e->running.node);
			}
			break;
		case EDG_WLL_EVENT_RESUBMISSION:
			if (USABLE(res, strict)) {
				if (e->resubmission.result == EDG_WLL_RESUBMISSION_WONTRESUB) {
					rep(js->pub.reason, e->resubmission.reason);
				}
			}
			if (USABLE_DATA(res, strict)) {
				if (e->resubmission.result == EDG_WLL_RESUBMISSION_WONTRESUB) {
					js->wontresub = 1;
				}
			}
			break;
		case EDG_WLL_EVENT_DONE:
			if (e->any.source == EDG_WLL_SOURCE_LRMS) {
				/* Done from JobWrapper is not sufficient for transition
				 * to DONE state according its current definition */
				break;
			}
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_DONE;
				rep(js->pub.reason, e->done.reason);
				switch (e->done.status_code) {
					case EDG_WLL_DONE_CANCELLED:
						js->pub.state = EDG_WLL_JOB_CANCELLED;
					case EDG_WLL_DONE_OK:
						rep(js->pub.location, "none"); break;
					default:
						free(js->pub.location);
						js->pub.location = location_string(
							edg_wll_SourceToString(e->done.source),
							e->done.host,
							e->done.src_instance);
				}
			}
			if (USABLE_DATA(res, strict)) {
				switch (e->done.status_code) {
					case EDG_WLL_DONE_OK:
						js->pub.exit_code = e->done.exit_code;
						js->pub.done_code = EDG_WLL_STAT_OK; break;
					case EDG_WLL_DONE_CANCELLED:
						js->pub.exit_code = 0;
						js->pub.done_code = EDG_WLL_STAT_CANCELLED; break;
					case EDG_WLL_DONE_FAILED:
						js->pub.exit_code = 0;
						js->pub.done_code = EDG_WLL_STAT_FAILED; break;
					default:
						goto bad_event; break;
				}
			}
			break;
		case EDG_WLL_EVENT_CANCEL:
			if (js->last_cancel_seqcode != NULL &&
				edg_wll_compare_seq(e->any.seqcode, js->last_cancel_seqcode) < 0) {
				res = RET_LATE;
			} 
			if (USABLE(res, strict)) {
				switch (e->cancel.status_code) {
					case EDG_WLL_CANCEL_REQ:
						js->pub.cancelling = 1; break;
					case EDG_WLL_CANCEL_DONE:
						js->pub.state = EDG_WLL_JOB_CANCELLED;
						rep(js->pub.reason, e->cancel.reason);
						rep(js->last_seqcode, e->any.seqcode);
						rep(js->pub.location, "none");
						/* fall though */
					case EDG_WLL_CANCEL_ABORT:
						js->pub.cancelling = 0; break;
					default:
						/* do nothing */
						break;

				}
			}
			if (USABLE_DATA(res, strict)) {
				rep(js->pub.cancelReason, e->cancel.reason);
			}
			break;
		case EDG_WLL_EVENT_ABORT:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_ABORTED;
				rep(js->pub.reason, e->abort.reason);
				rep(js->pub.location, "none");
			}
			break;

		case EDG_WLL_EVENT_CLEAR:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_CLEARED;
				rep(js->pub.location, "none");
				switch (e->clear.reason) {
					case EDG_WLL_CLEAR_USER:
						rep(js->pub.reason, "user retrieved output sandbox");
						break;
					case EDG_WLL_CLEAR_TIMEOUT:
						rep(js->pub.reason, "timed out, resource purge forced");
						break;
					case EDG_WLL_CLEAR_NOOUTPUT:
						rep(js->pub.reason, "no output was generated");
						break;
					default:
						goto bad_event; break;

				}
			}
			break;
		case EDG_WLL_EVENT_PURGE:
			/* ignore, meta-information only */
			break;
		case EDG_WLL_EVENT_MATCH:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_WAITING;
				js->pub.location = location_string(
						edg_wll_SourceToString(EDG_WLL_SOURCE_WORKLOAD_MANAGER),
						e->match.host,
						e->match.src_instance);
			}
			if (USABLE_DATA(res, strict)) {
				rep(js->pub.destination, e->match.dest_id);
			}
			break;
		case EDG_WLL_EVENT_PENDING:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_WAITING;
				rep(js->pub.reason, e->pending.reason);
				js->pub.location = location_string(
						edg_wll_SourceToString(EDG_WLL_SOURCE_WORKLOAD_MANAGER),
						e->match.host,
						e->match.src_instance);
			}
			break;
		case EDG_WLL_EVENT_REGJOB:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_SUBMITTED;
			}
			if (USABLE_DATA(res, strict)) {
				rep(js->pub.jdl, e->regJob.jdl);
				edg_wlc_JobIdFree(js->pub.parent_job);
				edg_wlc_JobIdDup(e->regJob.parent,
							&js->pub.parent_job);
				rep(js->pub.network_server, e->regJob.ns);
				js->pub.children_num = e->regJob.nsubjobs;
				if (e->regJob.jobtype == EDG_WLL_REGJOB_DAG
					|| e->regJob.jobtype == EDG_WLL_REGJOB_PARTITIONED) {
					js->pub.jobtype = EDG_WLL_STAT_DAG;
				}
				rep(js->pub.seed, e->regJob.seed);
			}
			break;
		case EDG_WLL_EVENT_USERTAG:
			if (USABLE_DATA(res, strict)) {
				if (e->userTag.name != NULL && e->userTag.value != NULL) {
					add_taglist(&js->pub.user_tags, 
						    e->userTag.name, e->userTag.value);
				} else {
					goto bad_event;
				}
			}
			break;
		case EDG_WLL_EVENT_LISTENER:
			/* ignore, listener port is not part of job status */
			break;
		case EDG_WLL_EVENT_CURDESCR:
		case EDG_WLL_EVENT_CHKPT:
		case EDG_WLL_EVENT_CHANGEACL:
			/* ignore, only for event log */
			break;
		
		default:
			goto bad_event;
			break;
	}

	if (USABLE(res,strict)) {
		js->pub.lastUpdateTime = e->any.timestamp;
		if (old_state != js->pub.state) {
			js->pub.stateEnterTime = js->pub.lastUpdateTime;
			js->pub.stateEnterTimes[1 + js->pub.state]
				= (int)js->pub.lastUpdateTime.tv_sec;
		}
	}

	if (USABLE_DATA(res,strict)) {
		if (e->any.source == EDG_WLL_SOURCE_NETWORK_SERVER &&
			js->pub.network_server == NULL) {
			char *inst;
			inst = e->any.src_instance;
			asprintf(&js->pub.network_server, "%s%s%s",
				e->any.host,
				inst != NULL ? ":" : " ",
				inst != NULL ? inst : "");
		}
	}
	
	if (e->any.type == EDG_WLL_EVENT_CANCEL) {
		rep(js->last_cancel_seqcode, e->any.seqcode);
	} else {
		rep(js->last_seqcode, e->any.seqcode);
	}

	return res;

bad_event:
	badEvent(js,e,ev_seq);
	return RET_SUSPECT;
}

/*
 * Helper for warning printouts
 */

static void warn(const char* format, ...)
{
	va_list l;
	va_start(l, format);

	/*
	fprintf(stderr, "Warning: ");
	vfprintf(stderr, format, l);
	fputc('\n', stderr);
	*/

	va_end(l);
}

static char *job_owner(edg_wll_Context ctx,char *md5_jobid)
{
	char	*stmt = NULL,*out = NULL;
	edg_wll_Stmt	sh;
	int	f = -1;
	
	edg_wll_ResetError(ctx);
	trio_asprintf(&stmt,"select cert_subj from users,jobs "
		"where users.userid = jobs.userid "
		"and jobs.jobid = '%|Ss'",md5_jobid);

	if (stmt==NULL) {
		edg_wll_SetError(ctx,ENOMEM, NULL);
		return NULL;
	}
	if (edg_wll_ExecStmt(ctx,stmt,&sh) >= 0) {
		f=edg_wll_FetchRow(sh,&out);
		if (f == 0) {
			if (out) free(out);
			out = NULL;
			edg_wll_SetError(ctx,ENOENT,md5_jobid);
		}
	}
	edg_wll_FreeStmt(&sh);
	free(stmt);

	return out;
}


#if 0
/* XXX went_through went out */
static int eval_expect_update(intJobStat *js, int* went_through, char **expect_from)
{
	int em = 0;
	int ft = 0; /* fall through following tests */

	if (ft || (went_through[ EDG_WLL_JOB_OUTPUTREADY ] && !js->done_failed)) ft = 1;
	if (ft || (went_through[ EDG_WLL_JOB_DONE ] && !js->done_failed)) ft = 1;
	if (ft || went_through[ EDG_WLL_JOB_CHECKPOINTED ]) ft = 1;
	if (ft || went_through[ EDG_WLL_JOB_RUNNING ]) {
		if (js->pub.node == NULL) em |= EXPECT_MASK_JOBMGR;
		ft = 1;
	}
	if (ft || went_through[ EDG_WLL_JOB_SCHEDULED ]) {
			if (js->pub.jssId == NULL) em |= EXPECT_MASK_JSS;
			if (js->pub.rsl == NULL) em |= EXPECT_MASK_JSS;
			if (js->pub.globusId == NULL) em |= EXPECT_MASK_JOBMGR;
			if (js->pub.localId == NULL) em |= EXPECT_MASK_JOBMGR;
			if (js->pub.jss_jdl == NULL) em |= EXPECT_MASK_RB;
			ft = 1;
	}
	if (ft || went_through[ EDG_WLL_JOB_READY ]) {
		if (js->pub.destination == NULL) em |= EXPECT_MASK_RB;
		ft = 1;
	}
	if (ft || went_through[ EDG_WLL_JOB_SUBMITTED ]) {
		if (js->pub.jdl == NULL) em |= EXPECT_MASK_UI;
		ft = 1;
	}
	
	if (em == 0) 
		*expect_from = NULL;
	else {
		asprintf(expect_from, "%s%s%s%s%s%s%s",
			(em & EXPECT_MASK_UI ) ? EDG_WLL_SOURCE_UI : "",
			(em & EXPECT_MASK_UI ) ? " " : "",
			(em & EXPECT_MASK_RB ) ? EDG_WLL_SOURCE_RB : "",
			(em & EXPECT_MASK_RB ) ? " " : "",
			(em & EXPECT_MASK_JSS ) ? EDG_WLL_SOURCE_JSS : "",
			(em & EXPECT_MASK_JSS ) ? " " : "",
			(em & EXPECT_MASK_JOBMGR ) ? EDG_WLL_SOURCE_JOBMGR : ""
			);
	}

	return (em == 0) ? 0 : 1;
}
#endif

static char* location_string(const char *source, const char *host, const char *instance)
{
	char *ret;
	asprintf(&ret, "%s/%s/%s", source, host, instance);
	return ret;
}

static int add_stringlist(char ***lptr, const char *new_item)
{
	char **itptr;
	int		i;

	if (*lptr == NULL) {
		itptr = (char **) malloc(2*sizeof(char *));
		itptr[0] = strdup(new_item);
		itptr[1] = NULL;
		*lptr = itptr;
		return 1;
	} else {
		for (i = 0, itptr = *lptr; itptr[i] != NULL; i++);
		itptr = (char **) realloc(*lptr, (i+2)*sizeof(char *));
		if (itptr != NULL) {
			itptr[i] = strdup(new_item);
			itptr[i+1] = NULL;
			*lptr = itptr;
			return 1;
		} else {
			return 0;
		}
	}
}

static int add_taglist(edg_wll_TagValue **lptr, const char *new_item, const char *new_item2)
{
	edg_wll_TagValue 	*itptr;
	int			i;

	if (*lptr == NULL) {
		itptr = (edg_wll_TagValue *) calloc(2,sizeof(edg_wll_TagValue));
		itptr[0].tag = strdup(new_item);
		itptr[0].value = strdup(new_item2);
		*lptr = itptr;
		return 1;
	} else {
		for (i = 0, itptr = *lptr; itptr[i].tag != NULL; i++)
			if ( !strcasecmp(itptr[i].tag, new_item) )
			{
				free(itptr[i].value);
				itptr[i].value = strdup(new_item2);
				return 1;
			}
		itptr = (edg_wll_TagValue *) realloc(*lptr, (i+2)*sizeof(edg_wll_TagValue));
		if (itptr != NULL) {
			itptr[i].tag = strdup(new_item);
			itptr[i].value = strdup(new_item2);
			itptr[i+1].tag = NULL;
			itptr[i+1].value = NULL;
			*lptr = itptr;
			return 1;
		} else {
			return 0;
		}
	}
}

/* XXX more thorough malloc, calloc, and asprintf failure handling */
/* XXX indexes in {short,long}_fields */
/* XXX strict mode */
/* XXX caching */

/*
 * Store current job state to states and status_tags DB tables.
 * Should be called with the job locked.
 */

edg_wll_ErrorCode edg_wll_StoreIntState(edg_wll_Context ctx,
				     intJobStat *stat,
				     int seq)
{
	char *jobid_md5, *stat_enc, *parent_md5 = NULL;
	char *stmt;
	edg_wll_TagValue *tagp;
	int update;
	int dbret;
	char *icnames, *icvalues;

	update = (seq > 0);
	jobid_md5 = edg_wlc_JobIdGetUnique(stat->pub.jobId);
	stat_enc = enc_intJobStat(strdup(""), stat);

	tagp = stat->pub.user_tags;
	if (tagp) {
		while ((*tagp).tag != NULL) {
			trio_asprintf(&stmt, "insert into status_tags"
					"(jobid,seq,name,value) values "
					"('%|Ss',%d,'%|Ss','%|Ss')",
					jobid_md5, seq, (*tagp).tag, (*tagp).value);
			if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) {
				if (EEXIST == edg_wll_Error(ctx, NULL, NULL)) {
				/* XXX: this should not happen */
					edg_wll_ResetError(ctx);
					tagp++;
					continue;
				}
				else
					goto cleanup;
			}
			tagp++;
		}
	}

	parent_md5 = edg_wlc_JobIdGetUnique(stat->pub.parent_job);
	if (parent_md5 == NULL) parent_md5 = strdup("*no parent job*");


	edg_wll_IColumnsSQLPart(ctx, ctx->job_index_cols, stat, 0, NULL, &icvalues);

	trio_asprintf(&stmt,
		"update states set "
		"status=%d,seq=%d,int_status='%|Ss',version='%|Ss'"
			",parent_job='%|Ss'%s "
		"where jobid='%|Ss'",
		stat->pub.state, seq, stat_enc, INTSTAT_VERSION,
		parent_md5, icvalues,
		jobid_md5);
	free(icvalues);

	if ((dbret = edg_wll_ExecStmt(ctx,stmt,NULL)) < 0) goto cleanup;

	if (dbret == 0) {
		edg_wll_IColumnsSQLPart(ctx, ctx->job_index_cols, stat, 1, &icnames, &icvalues);
		trio_asprintf(&stmt,
			"insert into states"
			"(jobid,status,seq,int_status,version"
				",parent_job%s) "
			"values ('%|Ss',%d,%d,'%|Ss','%|Ss','%|Ss'%s)",
			icnames,
			jobid_md5, stat->pub.state, seq, stat_enc,
			INTSTAT_VERSION, parent_md5, icvalues);
		free(icnames); free(icvalues);

		if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) goto cleanup;
	}

	if (update) {
		trio_asprintf(&stmt, "delete from states "
			"where jobid ='%|Ss' and ( seq<%d or version !='%|Ss')",
			jobid_md5, seq, INTSTAT_VERSION);
		if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) goto cleanup;
	}
	if (update) {
		trio_asprintf(&stmt, "delete from status_tags "
			"where jobid ='%|Ss' and seq<%d", jobid_md5, seq);
		if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) goto cleanup;
	}

	if (ctx->rgma_export) write2rgma_status(&stat->pub);

cleanup:
	free(stmt); 
	free(jobid_md5); free(stat_enc);
	free(parent_md5);
	return edg_wll_Error(ctx,NULL,NULL);
}

/*
 * Retrieve stored job state from states and status_tags DB tables.
 * Should be called with the job locked.
 */

edg_wll_ErrorCode edg_wll_LoadIntState(edg_wll_Context ctx,
				    edg_wlc_JobId jobid,
				    int seq,
				    intJobStat **stat)
{
	char *jobid_md5;
	char *stmt;
	edg_wll_Stmt sh;
	char *res, *res_rest;
	int nstates;

	edg_wll_ResetError(ctx);
	jobid_md5 = edg_wlc_JobIdGetUnique(jobid);

	if (seq == -1) {
		/* any sequence number */
		trio_asprintf(&stmt,
			"select int_status from states "
			"where jobid='%|Ss' and version='%|Ss'",
			jobid_md5, INTSTAT_VERSION);
	} else {
		trio_asprintf(&stmt,
			"select int_status from states "
			"where jobid='%|Ss' and seq='%d' and version='%|Ss'",
			jobid_md5, seq, INTSTAT_VERSION);
	}

	if (stmt == NULL) {
		return edg_wll_SetError(ctx, ENOMEM, NULL);
	}

	if ((nstates = edg_wll_ExecStmt(ctx,stmt,&sh)) < 0) goto cleanup;
	if (nstates == 0) {
		edg_wll_SetError(ctx,ENOENT,"no state in DB");
		goto cleanup;
	}
	if (edg_wll_FetchRow(sh,&res) < 0) goto cleanup;

	*stat = dec_intJobStat(res, &res_rest);
	if (res_rest == NULL) {
		edg_wll_SetError(ctx, EDG_WLL_ERROR_DB_CALL,
				"error decoding DB intJobStatus");
	}

	free(res);
cleanup:
	free(jobid_md5);
	free(stmt); edg_wll_FreeStmt(&sh);
	return edg_wll_Error(ctx,NULL,NULL);
}

/*
 * update stored state according to the new event
 * (must be called with the job locked)
 */

edg_wll_ErrorCode edg_wll_StepIntState(edg_wll_Context ctx,
					edg_wlc_JobId job,
					edg_wll_Event *e,
					int seq,
					edg_wll_JobStat	*stat_out)
{
	intJobStat 	*ijsp;
	int		intErr = 0;
	int		flags = 0;
	int		res;
	int		be_strict = 0;
	char		*errstring = NULL;
	intJobStat	jobstat;

	if (seq != 0) {
		intErr = edg_wll_LoadIntState(ctx, job, seq - 1, &ijsp);
	}
	if (seq != 0 && !intErr) {
		res = processEvent(ijsp, e, seq, be_strict, &errstring);
		if (res == RET_FATAL || res == RET_INTERNAL) { /* !strict */
			return edg_wll_SetError(ctx, EINVAL, errstring);
		}
		edg_wll_StoreIntState(ctx, ijsp, seq);
		if (stat_out) {
			memcpy(stat_out,&ijsp->pub,sizeof *stat_out);
			destroy_intJobStat_extension(ijsp);
		}
		else destroy_intJobStat(ijsp);
		free(ijsp);
	} else {
		edg_wll_intJobStatus(ctx, job, flags,&jobstat, js_enable_store);
		if (stat_out) {
			memcpy(stat_out,&jobstat.pub,sizeof *stat_out);
			destroy_intJobStat_extension(&jobstat);
		}
		else destroy_intJobStat(&jobstat);
	}
	return edg_wll_Error(ctx, NULL, NULL);
}
