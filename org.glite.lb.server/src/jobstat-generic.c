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

#include "glite/lb-utils/db.h"
#include "glite/lb/producer.h"
#include "glite/lb/context-int.h"
#include "glite/lb-utils/trio.h"

#include "db_supp.h"
#include "get_events.h"
#include "store.h"
#include "lock.h"
#include "index.h"
#include "jobstat.h"
#include "lb_authz.h"
#include "stats.h"

#define DAG_ENABLE	1

/* TBD: share in whole logging or workload */
#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif
  

#define mov(a,b) { free(a); a = b; b = NULL; }

static void warn (const char* format, ...) UNUSED_VAR ;
static char *job_owner(edg_wll_Context,char *);


int js_enable_store = 1;

/*
 * Basic manipulations with the internal representation of job state
 */

#if 0
static int eval_expect_update(intJobStat *, int *, char **);
#endif

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

int glite_lb_JobStatus(
	glite_lb_SrvContext	ctx,
	const char		*job,
	const glite_lb_StateOpts	*opts,
	glite_lb_State	**stat)
{
	glite_lbu_Blob	state_blob;

/* Local variables */
	char		*md5_jobid;

	intJobStat	jobstat;
	intJobStat	*ijsp;
	int		intErr = 0;
	int		lockErr;
	edg_wll_Acl	acl = NULL;
#if DAG_ENABLE	
	char		*stmt = NULL;
#endif	

	enum { OK, ERR_DECODE, ERR_COMP } bad_blob = OK;

	edg_wll_ResetError(ctx);

	/* XXX: unused md5_jobid = ctx->flesh->JobIdGetUnique(job); */

/* XXX: authz */
#if 0
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
#endif 

	intErr = glite_lb_LoadStateBlob(ctx,job,&state_blob);
       
	if (!intErr) {
		if (ctx->flesh->ComputeStateBlob(ctx,&state_blob,opts,stat)) {
			glite_lbu_FreeBlob(&state_blob);
			bad_blob = ERR_DECODE;
		}
	}
	if (intErr == ENOENT || bad_blob) {
		lockErr = edg_wll_LockJob(ctx,job);
		if (ctx->flesh->ComputeStateFull(ctx,job,opts,stat,&state_blob) == 0) bad_blob = OK;
		else { 
			bad_blob = ERR_COMP; 
			compErr = edg_wll_Error(ctx,NULL,&compDesc);
		}
		if (!lockErr) {
			switch (bad_blob) {
				case OK: glite_lb_StoreStateBlob(ctx,job,&state_blob);
					 break;
				case ERR_DECODE: glite_lb_DeleteStateBlob(ctx,job);
				case ERR_COMP: break;
			}
			edg_wll_UnlockJob(ctx,job); /* TODO: WARN */
		}
		else {
			/* TODO: syslog WARN */
		}

		if (bad_blob == ERR_COMP) {
			edg_wll_SetError(ctx,compErr,compDesc);
			free(compDesc);
		}

	}

	glite_lbu_FreeBlob(&state_blob);
	
/* TODO */
#if 0
	if (acl) {
		stat->acl = strdup(acl->string);
		edg_wll_FreeAcl(acl);
	}
#endif

	return edg_wll_Error(ctx);
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
	glite_lbu_Statement	sh;
	int	f = -1;
	
	edg_wll_ResetError(ctx);
	trio_asprintf(&stmt,"select cert_subj from users,jobs "
		"where users.userid = jobs.userid "
		"and jobs.jobid = '%|Ss'",md5_jobid);

	if (stmt==NULL) {
		edg_wll_SetError(ctx,ENOMEM, NULL);
		return NULL;
	}
	if (glite_lbu_ExecSQL(ctx->dbctx,stmt,&sh) >= 0) {
		f=glite_lbu_FetchRow(sh,1,NULL,&out);
		if (f == 0) {
			if (out) free(out);
			out = NULL;
			edg_wll_SetError(ctx,ENOENT,md5_jobid);
		}
		glite_lbu_FreeStmt(&sh);
	}
	free(stmt);
	edg_wll_SetErrorDB(ctx);

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
	jobid_md5 = glite_lbu_JobIdGetUnique(stat->pub.jobId);
	stat_enc = enc_intJobStat(strdup(""), stat);

	tagp = stat->pub.user_tags;
	if (tagp) {
		while ((*tagp).tag != NULL) {
			trio_asprintf(&stmt, "insert into status_tags"
					"(jobid,seq,name,value) values "
					"('%|Ss',%d,'%|Ss','%|Ss')",
					jobid_md5, seq, (*tagp).tag, (*tagp).value);
			if (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL) < 0) {
				if (EEXIST == edg_wll_SetErrorDB(ctx)) {
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

	parent_md5 = glite_lbu_JobIdGetUnique(stat->pub.parent_job);
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

	if ((dbret = glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL)) < 0) goto dberror;

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

		if (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL) < 0) goto dberror;
	}

	if (update) {
		trio_asprintf(&stmt, "delete from states "
			"where jobid ='%|Ss' and ( seq<%d or version !='%|Ss')",
			jobid_md5, seq, INTSTAT_VERSION);
		if (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL) < 0) goto dberror;
	}
	if (update) {
		trio_asprintf(&stmt, "delete from status_tags "
			"where jobid ='%|Ss' and seq<%d", jobid_md5, seq);
		if (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL) < 0) goto dberror;
	}
	goto cleanup;

dberror:
	edg_wll_SetErrorDB(ctx);
cleanup:
	free(stmt); 
	free(jobid_md5); free(stat_enc);
	free(parent_md5);
	return edg_wll_Error(ctx,NULL,NULL);
}


edg_wll_ErrorCode edg_wll_StoreIntStateEmbryonic(edg_wll_Context ctx,
        glite_lbu_JobId jobid,
        char *icnames, 
	char *values,
	glite_lbu_bufInsert bi)
{
	char *stmt = NULL;

/* TODO
		edg_wll_UpdateStatistics(ctx, NULL, e, &jobstat.pub);
		if (ctx->rgma_export) write2rgma_status(&jobstat.pub);
*/

#ifdef LB_BUF
	if (glite_lbu_bufferedInsert(bi, values))
		goto cleanup;
#else

	trio_asprintf(&stmt,
		"insert into states"
		"(jobid,status,seq,int_status,version"
			",parent_job%s) "
		"values (%s)",
		icnames, values);

	if (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL) < 0) goto cleanup;
#endif

cleanup:
	free(stmt); 

	return edg_wll_SetErrorDB(ctx);
}

/*
 * Retrieve stored job state from states and status_tags DB tables.
 * Should be called with the job locked.
 */

edg_wll_ErrorCode edg_wll_LoadIntState(edg_wll_Context ctx,
				    glite_lbu_JobId jobid,
				    int seq,
				    intJobStat **stat)
{
	char *jobid_md5;
	char *stmt;
	glite_lbu_Statement sh;
	char *res, *res_rest;
	int nstates;

	edg_wll_ResetError(ctx);
	jobid_md5 = glite_lbu_JobIdGetUnique(jobid);

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

	if ((nstates = glite_lbu_ExecSQL(ctx->dbctx,stmt,&sh)) < 0) goto dberror;
	if (nstates == 0) {
		edg_wll_SetError(ctx,ENOENT,"no state in DB");
		goto cleanup;
	}
	if (glite_lbu_FetchRow(sh,1,NULL,&res) < 0) goto dberror;

	*stat = dec_intJobStat(res, &res_rest);
	free(res);
	if (res_rest == NULL) {
		edg_wll_SetError(ctx, EDG_WLL_ERROR_DB_CALL,
				"error decoding DB intJobStatus");
	}

	goto cleanup;
dberror:
	edg_wll_SetErrorDB(ctx);
cleanup:
	free(jobid_md5);
	free(stmt); glite_lbu_FreeStmt(&sh);
	return edg_wll_Error(ctx,NULL,NULL);
}

/*
 * update stored state according to the new event
 * (must be called with the job locked)
 */

edg_wll_ErrorCode edg_wll_StepIntState(edg_wll_Context ctx,
					glite_lbu_JobId job,
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
	edg_wll_JobStat	oldstat;
	char 		*oldstat_rgmaline = NULL;

	memset(&oldstat,0,sizeof oldstat);
	if (seq != 0) {
		intErr = edg_wll_LoadIntState(ctx, job, seq - 1, &ijsp);
	}
	if (seq != 0 && !intErr) {
		edg_wll_CpyStatus(&ijsp->pub,&oldstat);

		if (ctx->rgma_export) oldstat_rgmaline = write2rgma_statline(&ijsp->pub);

		res = processEvent(ijsp, e, seq, be_strict, &errstring);
		if (res == RET_FATAL || res == RET_INTERNAL) { /* !strict */
			edg_wll_FreeStatus(&oldstat);
			return edg_wll_SetError(ctx, EINVAL, errstring);
		}
		edg_wll_StoreIntState(ctx, ijsp, seq);
		edg_wll_UpdateStatistics(ctx,&oldstat,e,&ijsp->pub);

		if (ctx->rgma_export) write2rgma_chgstatus(&ijsp->pub, oldstat_rgmaline);

		if (stat_out) {
			memcpy(stat_out,&ijsp->pub,sizeof *stat_out);
			destroy_intJobStat_extension(ijsp);
		}
		else destroy_intJobStat(ijsp);
		free(ijsp);
		edg_wll_FreeStatus(&oldstat);
	}
	else if (!edg_wll_intJobStatus(ctx, job, flags,&jobstat, js_enable_store)) 
	{
		/* FIXME: we miss state change in the case of seq != 0 
		 * Does anybody care? */

		edg_wll_UpdateStatistics(ctx,NULL,e,&jobstat.pub);

		if (ctx->rgma_export) write2rgma_status(&jobstat.pub);

		if (stat_out) {
			memcpy(stat_out,&jobstat.pub,sizeof *stat_out);
			destroy_intJobStat_extension(&jobstat);
		}
		else destroy_intJobStat(&jobstat);
	}
	return edg_wll_Error(ctx, NULL, NULL);
}

