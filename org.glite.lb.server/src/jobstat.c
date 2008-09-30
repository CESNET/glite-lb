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

#include "glite/lbu/trio.h"
#include "glite/lb/events.h"
#include "glite/lb/context-int.h"
#include "glite/lb/intjobstat.h"
#include "glite/lb/process_event.h"

#include "get_events.h"
#include "store.h"
#include "lock.h"
#include "index.h"
#include "jobstat.h"
#include "lb_authz.h"
#include "stats.h"
#include "db_supp.h"
#include "db_calls.h"

#define DAG_ENABLE	1

#define	DONT_LOCK	0
#define	LOCK		1

/* TBD: share in whole logging or workload */
#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif
  

#define mov(a,b) { free(a); a = b; b = NULL; }

static void warn (const char* format, ...) UNUSED_VAR ;
static char *job_owner(edg_wll_Context,char *);
static edg_wll_ErrorCode get_job_parent(edg_wll_Context ctx, glite_jobid_const_t job, glite_jobid_t *parent);


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


int edg_wll_JobStatusServer(
	edg_wll_Context	ctx,
	glite_jobid_const_t		job,
	int		flags,
	edg_wll_JobStat	*stat)
{

/* Local variables */
	char		*string_jobid = NULL;
	char		*md5_jobid = NULL;

	intJobStat	jobstat;
	intJobStat	*ijsp;
	int		whole_cycle;
	edg_wll_Acl	acl = NULL;
#if DAG_ENABLE	
	char		*stmt = NULL;
#endif
	char *s_out;
	intJobStat *js;
	char *out[1], *out_stat[3];
	glite_lbu_Statement sh = NULL;
	int num_sub, num_f, i, ii;


	edg_wll_ResetError(ctx);

	memset(&jobstat, 0, sizeof(jobstat));
	string_jobid = edg_wlc_JobIdUnparse(job);
	if (string_jobid == NULL || stat == NULL)
		return edg_wll_SetError(ctx,EINVAL, NULL);
	md5_jobid = edg_wlc_JobIdGetUnique(job);

	do {
		whole_cycle = 0;

		if (edg_wll_Transaction(ctx)) goto rollback;
		if (edg_wll_LockJobRowInShareMode(ctx, md5_jobid)) goto rollback;


		if (!edg_wll_LoadIntState(ctx, job, DONT_LOCK, -1 /*all events*/, &ijsp)) {
			memcpy(stat, &(ijsp->pub), sizeof(ijsp->pub));
			destroy_intJobStat_extension(ijsp);
			free(ijsp);
		} else {
			if (edg_wll_intJobStatus(ctx, job, flags,&jobstat, js_enable_store, 0)) {
				goto rollback;
			}
			memcpy(stat, &(jobstat.pub), sizeof(jobstat.pub));
		}
		
		if (edg_wll_GetACL(ctx, job, &acl)) goto rollback;

		/* authorization check */
		if ( !(ctx->noAuth) &&
		    (!(ctx->peerName) ||  !edg_wll_gss_equal_subj(ctx->peerName, stat->owner))) {
		      if ((acl == NULL) || edg_wll_CheckACL(ctx, acl, EDG_WLL_PERM_READ)) {
			 if (acl) {
				goto rollback;
			 } else {
				edg_wll_SetError(ctx,EPERM, "not owner, no ACL is set");
				goto rollback;
			 }
		      }
		}

		if (acl) {
			stat->acl = strdup(acl->string);
			edg_wll_FreeAcl(acl);
			acl = NULL;
		}

		if ((flags & EDG_WLL_STAT_CLASSADS) == 0) {
			char *null = NULL;

			mov(stat->jdl, null);
			mov(stat->matched_jdl, null);
			mov(stat->condor_jdl, null);
			mov(stat->rsl, null);
		}

	#if DAG_ENABLE
		if (stat->jobtype == EDG_WLL_STAT_DAG || stat->jobtype == EDG_WLL_STAT_COLLECTION) {

	//	XXX: The users does not want any histogram. What do we do about it? 
	//		if ((!(flags & EDG_WLL_STAT_CHILDHIST_FAST))&&(!(flags & EDG_WLL_STAT_CHILDHIST_THOROUGH))) { /* No Histogram */
	//                        if (stat->children_hist != NULL) {	/* No histogram will be sent even if there was one */
	//
	//				printf("\nNo Histogram required\n\n");
	//
	//                              free(stat->children_hist);
	//			}
	//			
	//		}


			if (flags & EDG_WLL_STAT_CHILDSTAT) {

				trio_asprintf(&stmt, "SELECT version,int_status,jobid FROM states WHERE parent_job='%|Ss'", md5_jobid);
				if (stmt != NULL) {
					num_sub = edg_wll_ExecSQL(ctx, stmt, &sh);
					if (num_sub >=0 ) {
						i = 0;
						stat->children_states = calloc(num_sub+1, sizeof(edg_wll_JobStat));
						if (stat->children_states == NULL) {
							edg_wll_SetError(ctx, ENOMEM, "edg_wll_JobStatusServer() calloc children_states failed!");
							goto rollback;
						}
						while ((num_f = edg_wll_FetchRow(ctx, sh, sizeof(out_stat), NULL, out_stat)) == 3
							&& i < num_sub) {
							if (!strcmp(INTSTAT_VERSION,out_stat[0])) {
								js = dec_intJobStat(out_stat[1], &s_out);
								if (s_out != NULL && js != NULL) {
									stat->children_states[i] = js->pub;
									destroy_intJobStat_extension(js);
									free(js);
									i++; // Careful, this value will also be used further
								}
							}
							else { // recount state
								glite_jobid_t	subjob;
								intJobStat	js_real;
								char		*name;
								int		port;


								js = &js_real;
								glite_jobid_getServerParts(job, &name, &port);
								if (glite_jobid_recreate(name, port, out_stat[2], &subjob)) {
									goto rollback;
								}
								free(name);

								if (edg_wll_intJobStatus(ctx, subjob, flags, js, js_enable_store, 0)) {
									goto rollback;
								}
								glite_jobid_free(subjob);
								stat->children_states[i] = js->pub;
								destroy_intJobStat_extension(js);
								i++; // Careful, this value will also be used further
							}
							free(out_stat[0]); out_stat[0] = NULL;
							free(out_stat[1]); out_stat[1] = NULL;  
							free(out_stat[2]); out_stat[2] = NULL;
						}
						if (num_f < 0) goto rollback;

						glite_lbu_FreeStmt(&sh); sh = NULL;
					}
					else goto rollback;

					free(stmt); stmt = NULL;
				} else {
					edg_wll_SetError(ctx, ENOMEM, "edg_wll_JobStatusServer() trio_asprintf failed!");
					goto rollback;
				}
			}


			if (flags & EDG_WLL_STAT_CHILDHIST_THOROUGH) { /* Full (thorough) Histogram */


				if (stat->children_hist == NULL) {
					stat->children_hist = (int*) calloc(1+EDG_WLL_NUMBER_OF_STATCODES, sizeof(int));
					if (stat->children_hist == NULL) {
						edg_wll_SetError(ctx, ENOMEM, "edg_wll_JobStatusServer() calloc children_hist failed!");
						goto rollback;
					}

					stat->children_hist[0] = EDG_WLL_NUMBER_OF_STATCODES;
				}
				else {
					/* If hist is loaded, it probably contain only incomplete histogram
					 * built in update_parent_status. Count it from scratch...*/
					for (ii=1; ii<=EDG_WLL_NUMBER_OF_STATCODES; ii++)
						stat->children_hist[ii] = 0;
				}

				if (flags & EDG_WLL_STAT_CHILDSTAT) { // Job states have already been loaded
					for ( ii = 0 ; ii < i ; ii++ ) {
						stat->children_hist[(stat->children_states[ii].state)+1]++;
					}
				}
				else {
					// Get child states from the database
					trio_asprintf(&stmt, "SELECT version,status,jobid FROM states WHERE parent_job='%|Ss'", md5_jobid);
					if (stmt != NULL) {
						num_sub = edg_wll_ExecSQL(ctx, stmt, &sh);
						if (num_sub >=0 ) {
							while ((num_f = edg_wll_FetchRow(ctx, sh, sizeof(out_stat)/sizeof(out_stat[0]), NULL, out_stat)) == 3 ) {
								if (!strcmp(INTSTAT_VERSION,out_stat[0])) {
									num_f = atoi(out_stat[1]);
									if (num_f > EDG_WLL_JOB_UNDEF && num_f < EDG_WLL_NUMBER_OF_STATCODES)
										stat->children_hist[num_f+1]++;
								}
								else { // recount state
									glite_jobid_t	subjob;
									intJobStat	js_real;
									char		*name;
									int		port;


									js = &js_real;
									glite_jobid_getServerParts(job, &name, &port);
									if (glite_jobid_recreate(name, port, out_stat[2], &subjob)) {
										goto rollback;
									}
									free(name);

									if (edg_wll_intJobStatus(ctx, subjob, flags, js, js_enable_store, 0)) {
										goto rollback;
									}
									glite_jobid_free(subjob);
									num_f = js->pub.state;
									if (num_f > EDG_WLL_JOB_UNDEF && num_f < EDG_WLL_NUMBER_OF_STATCODES)
										stat->children_hist[num_f+1]++;

									destroy_intJobStat(js);
								}
								free(out_stat[0]); out_stat[0] = NULL;
								free(out_stat[1]); out_stat[1] = NULL;  
								free(out_stat[2]); out_stat[2] = NULL;
							}
							if (num_f < 0) goto rollback;

							glite_lbu_FreeStmt(&sh); sh = NULL;
						}
						else goto rollback;

						free(stmt); stmt = NULL;
					} else {
						edg_wll_SetError(ctx, ENOMEM, "edg_wll_JobStatusServer() trio_asprintf failed!");
						goto rollback;
					}
				}
			}
			else {
				if (flags & EDG_WLL_STAT_CHILDHIST_FAST) { /* Fast Histogram */
					
					if (stat->children_hist == NULL) {
						// If the histogram exists, assume that it was already filled during job state retrieval
						stat->children_hist = (int*) calloc(1+EDG_WLL_NUMBER_OF_STATCODES, sizeof(int));
						if (stat->children_hist == NULL) {
							edg_wll_SetError(ctx, ENOMEM, "edg_wll_JobStatusServer() calloc children_hist failed!");
							goto rollback;
						}

						if (edg_wll_GetSubjobHistogram(ctx, job, stat->children_hist))
							goto rollback;
					}
				}
				else {
					if (stat->children_hist) {
						free (stat->children_hist);
						stat->children_hist = NULL;
					}
				}

			}


			if (flags & EDG_WLL_STAT_CHILDREN) {

				trio_asprintf(&stmt, "SELECT j.dg_jobid FROM states s,jobs j "
						"WHERE s.parent_job='%|Ss' AND s.jobid=j.jobid",
					md5_jobid);
				if (stmt != NULL) {
					num_sub = edg_wll_ExecSQL(ctx, stmt, &sh);
					if (num_sub >=0 ) {
						while ((num_f = edg_wll_FetchRow(ctx, sh, sizeof(out)/sizeof(out[0]), NULL, out)) == 1 ) {
							add_stringlist(&stat->children, out[0]);
							free(out[0]); 
						}
						if (num_f < 0) goto rollback;

						glite_lbu_FreeStmt(&sh); sh = NULL;
					}
					else goto rollback;

					free(stmt); stmt = NULL;
				} else {
					edg_wll_SetError(ctx, ENOMEM, "edg_wll_JobStatusServer() trio_asprintf failed!");
					goto rollback;
				}
			}
		}
#endif

		whole_cycle = 1;
rollback:
		if (!whole_cycle) { 
			edg_wll_FreeStatus(&jobstat.pub);
			memset(stat, 0, sizeof(*stat));
		}
		destroy_intJobStat_extension(&jobstat);
		if (acl) { edg_wll_FreeAcl(acl); acl = NULL; }
		if (stmt) { free(stmt); stmt = NULL; }
		if (sh) { glite_lbu_FreeStmt(&sh); sh = NULL; }

        } while (edg_wll_TransNeedRetry(ctx));

	free(string_jobid);
	free(md5_jobid);

	return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_intJobStatus(
	edg_wll_Context	ctx,
	glite_jobid_const_t	job,
	int			flags,
	intJobStat	*intstat,
	int		update_db,
	int		add_fqans)
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
	if (string_jobid == NULL || intstat == NULL) {
		free(string_jobid);
		return edg_wll_SetError(ctx,EINVAL, NULL);
	}
	free(string_jobid);

	/* can be already filled by public edg_wll_JobStat() */
	if (intstat->pub.owner == NULL) {
		md5_jobid = edg_wlc_JobIdGetUnique(job);
		if ( !(intstat->pub.owner = job_owner(ctx,md5_jobid)) ) {
			free(md5_jobid);
			return edg_wll_Error(ctx,NULL,NULL);
		}
	}
	
	/* re-lock job from InShareMode to ForUpdate
	 * needed by edg_wll_RestoreSubjobState and edg_wll_StoreIntState
	 */
	res = edg_wll_LockJobRowForUpdate(ctx, md5_jobid);
	free(md5_jobid);
	if (res) return edg_wll_Error(ctx, NULL, NULL);

        jqr[0].attr = EDG_WLL_QUERY_ATTR_JOBID;
        jqr[0].op = EDG_WLL_QUERY_OP_EQUAL;
        jqr[0].value.j = (glite_jobid_t)job;
        jqr[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	jqra = (edg_wll_QueryRec **) malloc (2 * sizeof(edg_wll_QueryRec **));
	jqra[0] = jqr;
	jqra[1] = NULL;

	if (edg_wll_QueryEventsServer(ctx,1, (const edg_wll_QueryRec **)jqra, NULL, &events)) {
		if (edg_wll_Error(ctx, NULL, NULL) == ENOENT) {
			if (edg_wll_RestoreSubjobState(ctx, job, intstat)) {
				destroy_intJobStat(intstat);
				free(jqra);
				free(intstat->pub.owner); intstat->pub.owner = NULL;
				return edg_wll_Error(ctx, NULL, NULL);
			}
		}
		else {
			free(jqra);
			free(intstat->pub.owner); intstat->pub.owner = NULL;
                	return edg_wll_Error(ctx, NULL, NULL);
		}
	}
	else {
		free(jqra);

		for (num_events = 0; events[num_events].type != EDG_WLL_EVENT_UNDEF;
			num_events++);

		if (num_events == 0) {
			free(intstat->pub.owner); intstat->pub.owner = NULL;
			return edg_wll_SetError(ctx,ENOENT,NULL);
		}

		for (i = 0; i < num_events; i++) {
			res = processEvent(intstat, &events[i], i, be_strict, &errstring);
			if (res == RET_FATAL || res == RET_INTERNAL) { /* !strict */
				intErr = 1; break;
			}
		}
		if (intstat->pub.state == EDG_WLL_JOB_UNDEF) {
			intstat->pub.state = EDG_WLL_JOB_UNKNOWN;
		}


		for (i=0; i < num_events ; i++) edg_wll_FreeEvent(&events[i]);
		free(events);
	}

	if (intErr) {
		destroy_intJobStat(intstat);
		return edg_wll_SetError(ctx, EDG_WLL_ERROR_SERVER_RESPONSE, NULL);
	} else {
		/* XXX intstat->pub.expectUpdate = eval_expect_update(intstat, &intstat->pub.expectFrom); */
		intErr = edg_wlc_JobIdDup(job, &intstat->pub.jobId);
		if (intErr) return edg_wll_SetError(ctx, intErr, NULL);

		if (update_db) {
			int tsq = num_events - 1;
			if (add_fqans && tsq == 0 && ctx->fqans != NULL) {
				for (i=0; ctx->fqans[i]; i++);
				intstat->pub.user_fqans = malloc(sizeof(*ctx->fqans)*(i+1));
				for (i=0; ctx->fqans[i]; i++) {
					intstat->pub.user_fqans[i] = strdup(ctx->fqans[i]);
				}
				intstat->pub.user_fqans[i] = NULL;
			}

			edg_wll_StoreIntState(ctx, intstat, tsq);
			/* recheck
			 * intJobStat *reread;
			 * edg_wll_LoadIntState(ctx, job, tsq, &reread);
			 * destroy_intJobStat(reread);
			*/
		}
	}

	return edg_wll_Error(ctx, NULL, NULL);
}


/* 
 * Regenarate state of subjob without any event from its parent JobReg Event 
 */
edg_wll_ErrorCode edg_wll_RestoreSubjobState(
	edg_wll_Context		ctx,
	glite_jobid_const_t	job,
	intJobStat		*intstat)
{
	glite_jobid_t		parent_job = NULL;
	edg_wll_QueryRec        jqr_p1[2], jqr_p2[2];
	edg_wll_QueryRec        **ec, **jc;
	edg_wll_Event		*events_p;
	int			err, i;

	
	/* find job parent */
	if (get_job_parent(ctx, job, &parent_job)) goto err;

	/* get registration event(s) of parent*/
	jqr_p1[0].attr = EDG_WLL_QUERY_ATTR_JOBID;
	jqr_p1[0].op = EDG_WLL_QUERY_OP_EQUAL;
	jqr_p1[0].value.j = parent_job;
	jqr_p1[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	jqr_p2[0].attr = EDG_WLL_QUERY_ATTR_EVENT_TYPE;
	jqr_p2[0].op = EDG_WLL_QUERY_OP_EQUAL;
	jqr_p2[0].value.i = EDG_WLL_EVENT_REGJOB;
	jqr_p2[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	jc = (edg_wll_QueryRec **) malloc (2 * sizeof(edg_wll_QueryRec **));
	jc[0] = jqr_p1;
	jc[1] = NULL;

	ec = (edg_wll_QueryRec **) malloc (2 * sizeof(edg_wll_QueryRec **));
	ec[0] = jqr_p2;
	ec[1] = NULL;

	if (edg_wll_QueryEventsServer(ctx,1, (const edg_wll_QueryRec **)jc, 
				(const edg_wll_QueryRec **)ec, &events_p)) {
		free(jc);
		free(ec);
		return edg_wll_Error(ctx, NULL, NULL);
	}
	free(jc);
	free(ec);

	/* recreate job status of subjob */
	err = intJobStat_embryonic(ctx, job, (const edg_wll_RegJobEvent *) &(events_p[0]), intstat);

	for (i=0; events_p[i].type != EDG_WLL_EVENT_UNDEF ; i++) 
		edg_wll_FreeEvent(&events_p[i]);
	free(events_p);

	if (err) goto err;

err:
	return edg_wll_Error(ctx, NULL, NULL);
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
	if (edg_wll_ExecSQL(ctx,stmt,&sh) >= 0) {
		f=edg_wll_FetchRow(ctx,sh,1,NULL,&out);
		if (f == 0) {
			if (out) free(out);
			out = NULL;
			edg_wll_SetError(ctx,ENOENT,md5_jobid);
		}
	}
	glite_lbu_FreeStmt(&sh);
	free(stmt);

	return out;
}


static edg_wll_ErrorCode get_job_parent(edg_wll_Context ctx, glite_jobid_const_t job, glite_jobid_t *parent)
{
	glite_lbu_Statement	sh = NULL;
	char	*stmt = NULL, *out = NULL;
	char 	*md5_jobid = edg_wlc_JobIdGetUnique(job);
	int	ret;

	
	edg_wll_ResetError(ctx);
	trio_asprintf(&stmt,"select parent_job from states "
		"where jobid = '%|Ss'" ,md5_jobid);

	if (stmt==NULL) {
		edg_wll_SetError(ctx,ENOMEM, NULL);
		goto err;
	}

	if (edg_wll_ExecSQL(ctx,stmt,&sh) < 0) goto err;

	if (!edg_wll_FetchRow(ctx,sh,1,NULL,&out)) {
		edg_wll_SetError(ctx,ENOENT,md5_jobid);
		goto err;
	}

	ret = glite_jobid_recreate((const char*) ctx->srvName,
			ctx->srvPort, (const char *) out, parent);

	if (ret) {
		edg_wll_SetError(ctx,ret,"Error creating jobid");
		goto err;
	}

err:
	if (sh) glite_lbu_FreeStmt(&sh);
	free(md5_jobid);
	free(stmt);
	free(out);

	return edg_wll_Error(ctx,NULL,NULL);
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
	jobid_md5 = edg_wlc_JobIdGetUnique(stat->pub.jobId);
	stat_enc = enc_intJobStat(strdup(""), stat);

	tagp = stat->pub.user_tags;
	if (tagp) {
		while ((*tagp).tag != NULL) {
			trio_asprintf(&stmt, "insert into status_tags"
					"(jobid,seq,name,value) values "
					"('%|Ss',%d,'%|Ss','%|Ss')",
					jobid_md5, seq, (*tagp).tag, (*tagp).value);
			if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) {
				if (EEXIST == edg_wll_Error(ctx, NULL, NULL)) {
				/* XXX: this should not happen */
					edg_wll_ResetError(ctx);
					free(stmt); stmt = NULL;
					tagp++;
					continue;
				}
				else
					goto cleanup;
			}
			free(stmt); stmt = NULL;
			tagp++;
		}
	}

	parent_md5 = edg_wlc_JobIdGetUnique(stat->pub.parent_job);
	if (parent_md5 == NULL) parent_md5 = strdup("*no parent job*");


	edg_wll_IColumnsSQLPart(ctx, ctx->job_index_cols, &stat->pub, 0, NULL, &icvalues);

	trio_asprintf(&stmt,
		"update states set "
		"status=%d,seq=%d,int_status='%|Ss',version='%|Ss'"
			",parent_job='%|Ss'%s "
		"where jobid='%|Ss'",
		stat->pub.state, seq, stat_enc, INTSTAT_VERSION,
		parent_md5, icvalues,
		jobid_md5);
	free(icvalues);

	if ((dbret = edg_wll_ExecSQL(ctx,stmt,NULL)) < 0) goto cleanup;
	free(stmt); stmt = NULL;

	if (dbret == 0) {
		edg_wll_IColumnsSQLPart(ctx, ctx->job_index_cols, &stat->pub, 1, &icnames, &icvalues);
		trio_asprintf(&stmt,
			"insert into states"
			"(jobid,status,seq,int_status,version"
				",parent_job%s) "
			"values ('%|Ss',%d,%d,'%|Ss','%|Ss','%|Ss'%s)",
			icnames,
			jobid_md5, stat->pub.state, seq, stat_enc,
			INTSTAT_VERSION, parent_md5, icvalues);
		free(icnames); free(icvalues);

		if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) goto cleanup;
		free(stmt); stmt = NULL;
	}

	if (update) {
		trio_asprintf(&stmt, "delete from states "
			"where jobid ='%|Ss' and ( seq<%d or version !='%|Ss')",
			jobid_md5, seq, INTSTAT_VERSION);
		if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) goto cleanup;
		free(stmt); stmt = NULL;
	}
	if (update) {
		trio_asprintf(&stmt, "delete from status_tags "
			"where jobid ='%|Ss' and seq<%d", jobid_md5, seq);
		if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) goto cleanup;
		free(stmt); stmt = NULL;
	}

cleanup:
	free(stmt); 
	free(jobid_md5); free(stat_enc);
	free(parent_md5);
	return edg_wll_Error(ctx,NULL,NULL);
}


edg_wll_ErrorCode edg_wll_StoreIntStateEmbryonic(edg_wll_Context ctx,
        char *icnames, 
	char *values)
{
	char *stmt = NULL;

/* TODO
		edg_wll_UpdateStatistics(ctx, NULL, e, &jobstat.pub);
		if (ctx->rgma_export) write2rgma_status(&jobstat);
*/

	trio_asprintf(&stmt,
		"insert into states"
		"(jobid,status,seq,int_status,version"
			",parent_job%s) "
		"values (%s)",
		icnames, values);

	if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) goto cleanup;

cleanup:
	free(stmt); 

	return edg_wll_Error(ctx,NULL,NULL);
}

/*
 * Retrieve stored job state from states and status_tags DB tables.
 * Should be called with the job locked.
 */

edg_wll_ErrorCode edg_wll_LoadIntState(edg_wll_Context ctx,
				    glite_jobid_const_t jobid,
				    int lock,
				    int seq,
				    intJobStat **stat)
{
	char *jobid_md5;
	char *stmt;
	glite_lbu_Statement sh;
	char *res, *res_rest;
	int nstates;

	edg_wll_ResetError(ctx);
	jobid_md5 = edg_wlc_JobIdGetUnique(jobid);

	if (lock) {
		edg_wll_LockJobRowForUpdate(ctx,jobid_md5);
	}

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

	if ((nstates = edg_wll_ExecSQL(ctx,stmt,&sh)) < 0) goto cleanup;
	if (nstates == 0) {
		edg_wll_SetError(ctx,ENOENT,"no state in DB");
		goto cleanup;
	}
	if (edg_wll_FetchRow(ctx,sh,1,NULL,&res) < 0) goto cleanup;

	*stat = dec_intJobStat(res, &res_rest);
	if (res_rest == NULL) {
		edg_wll_SetError(ctx, EDG_WLL_ERROR_DB_CALL,
				"error decoding DB intJobStatus");
	}

	free(res);
cleanup:
	free(jobid_md5);
	free(stmt); 
	if (sh) glite_lbu_FreeStmt(&sh);
	return edg_wll_Error(ctx,NULL,NULL);
}


static char* hist_to_string(int * hist)
{
	int i;
	char *s, *s1;


	assert(hist[0] == EDG_WLL_NUMBER_OF_STATCODES);
	asprintf(&s, "%s=%d", edg_wll_StatToString(1), hist[2]);

	for (i=2; i<hist[0] ; i++) {
		asprintf(&s1, "%s, %s=%d", s, edg_wll_StatToString(i), hist[i+1]);
		free(s); s=s1; s1=NULL;
	}

	return s;
}

#if 0	// should not be needed anymore

/* checks whether parent jobid would generate the same sem.num. */
/* as children jobid 						*/
static int dependent_parent_lock(edg_wll_Context ctx, glite_jobid_const_t p,glite_jobid_const_t c)
{
	int     p_id, c_id;

	if ((p_id=edg_wll_JobSemaphore(ctx, p)) == -1) return -1;
	if ((c_id=edg_wll_JobSemaphore(ctx, c)) == -1) return -1;

	if (p_id == c_id) return 1;
	else return 0;
}
#endif


static edg_wll_ErrorCode load_parent_intJobStat(edg_wll_Context ctx, intJobStat *cis, intJobStat **pis)
{
	if (*pis) return edg_wll_Error(ctx, NULL, NULL); // already loaded and locked

	if (edg_wll_LoadIntState(ctx, cis->pub.parent_job, LOCK, - 1, pis))
		goto err;

	assert(*pis);	// deadlock would happen with next call of this function

err:
	return edg_wll_Error(ctx, NULL, NULL);

}


static int log_collectionState_event(edg_wll_Context ctx, edg_wll_JobStatCode state, enum edg_wll_StatDone_code done_code, intJobStat *cis, intJobStat *pis, edg_wll_Event *ce) 
{
	int	ret = 0;

	edg_wll_Event  *event = 
		edg_wll_InitEvent(EDG_WLL_EVENT_COLLECTIONSTATE);

	event->any.priority = EDG_WLL_LOGFLAG_INTERNAL;

	if (ctx->serverIdentity) 
		event->any.user = strdup(ctx->serverIdentity);	
	else
		event->any.user = strdup("LBProxy");

	if (!edg_wll_SetSequenceCode(ctx,pis->last_seqcode,EDG_WLL_SEQ_NORMAL)) {
		ctx->p_source = EDG_WLL_SOURCE_LB_SERVER;
                edg_wll_IncSequenceCode(ctx);
        }
	event->any.seqcode = edg_wll_GetSequenceCode(ctx);
	edg_wlc_JobIdDup(pis->pub.jobId, &(event->any.jobId));
	gettimeofday(&event->any.timestamp,0);
	if (ctx->p_host) event->any.host = strdup(ctx->p_host);
	event->any.level = ctx->p_level;
	event->any.source = EDG_WLL_SOURCE_LB_SERVER; 
		
					
	event->collectionState.state = edg_wll_StatToString(state);
	event->collectionState.done_code = done_code;
	event->collectionState.histogram = hist_to_string(pis->pub.children_hist);
	edg_wlc_JobIdDup(cis->pub.jobId, &(event->collectionState.child));
	event->collectionState.child_event = edg_wll_EventToString(ce->any.type);

	ret = db_parent_store(ctx, event, pis);

	edg_wll_FreeEvent(event);
	free(event);

	return ret;	
}


/* returns state class of subjob of job collection	*/
static subjobClassCodes class(edg_wll_JobStat *stat)
{
	switch (stat->state) {
		case EDG_WLL_JOB_RUNNING:
			return(SUBJOB_CLASS_RUNNING);
			break;
		case EDG_WLL_JOB_DONE:
			if (stat->done_code == EDG_WLL_STAT_OK)
				return(SUBJOB_CLASS_DONE);
			else
				// failed & cancelled
				return(SUBJOB_CLASS_REST);
			break;
		case EDG_WLL_JOB_ABORTED:
			return(SUBJOB_CLASS_ABORTED);
			break;
		case EDG_WLL_JOB_CLEARED:
			return(SUBJOB_CLASS_CLEARED);
			break;
		default:
			return(SUBJOB_CLASS_REST);
			break;
	}
}

/* Mapping of subjob class to some field in childen_hist */
static edg_wll_JobStatCode class_to_statCode(subjobClassCodes code)
{
	switch (code) {
		case SUBJOB_CLASS_RUNNING:	return(EDG_WLL_JOB_RUNNING); break;
		case SUBJOB_CLASS_DONE:		return(EDG_WLL_JOB_DONE); break;
		case SUBJOB_CLASS_ABORTED:	return(EDG_WLL_JOB_ABORTED); break;
		case SUBJOB_CLASS_CLEARED:	return(EDG_WLL_JOB_CLEARED); break;
		case SUBJOB_CLASS_REST:		return(EDG_WLL_JOB_UNKNOWN); break;
		default:			assert(0); break;
	}
}

/* count parent state from subjob histogram */
static edg_wll_JobStatCode process_Histogram(intJobStat *pis)
{
	if (pis->pub.children_hist[class_to_statCode(SUBJOB_CLASS_RUNNING)+1] > 0) {
		return EDG_WLL_JOB_RUNNING;
	}
	else if (pis->pub.children_hist[class_to_statCode(SUBJOB_CLASS_CLEARED)+1] == pis->pub.children_num) {
		return EDG_WLL_JOB_CLEARED;
	}
	else if (pis->pub.children_hist[class_to_statCode(SUBJOB_CLASS_DONE)+1] 
			+ pis->pub.children_hist[class_to_statCode(SUBJOB_CLASS_CLEARED)+1] == pis->pub.children_num) {
		return EDG_WLL_JOB_DONE;
	}
	else if (pis->pub.children_hist[class_to_statCode(SUBJOB_CLASS_ABORTED)+1]
			+ pis->pub.children_hist[class_to_statCode(SUBJOB_CLASS_DONE)+1]
			+ pis->pub.children_hist[class_to_statCode(SUBJOB_CLASS_CLEARED)+1] == pis->pub.children_num) {
		return EDG_WLL_JOB_ABORTED;
	}
	else
		return EDG_WLL_JOB_WAITING;
}

static edg_wll_ErrorCode update_parent_status(edg_wll_Context ctx, edg_wll_JobStat *subjob_stat_old, intJobStat *cis, edg_wll_Event *ce)
{
	intJobStat		*pis = NULL;
	subjobClassCodes	subjob_class, subjob_class_old;
	edg_wll_JobStatCode	parent_new_state;


	subjob_class = class(&cis->pub);
	subjob_class_old = class(subjob_stat_old);


	if (subjob_class_old != subjob_class) {
		if (load_parent_intJobStat(ctx, cis, &pis)) goto err;

		pis->pub.children_hist[class_to_statCode(subjob_class)+1]++;
		pis->pub.children_hist[class_to_statCode(subjob_class_old)+1]--;

		edg_wll_StoreSubjobHistogram(ctx, cis->pub.parent_job, pis);


		if (pis->pub.jobtype == EDG_WLL_STAT_COLLECTION) {
			parent_new_state = process_Histogram(pis);
			if (pis->pub.state != parent_new_state) {
				// XXX: we do not need  EDG_WLL_STAT_code any more
				//	doneFailed subjob is stored in REST class and
				//	inducting collection Waiting state
				//	-> in future may be removed from collectionState event
				//	   supposing collection Done state to be always DoneOK
				if (log_collectionState_event(ctx, parent_new_state, EDG_WLL_STAT_OK, cis, pis, ce))
					goto err;
			}
		}
	}

err:
	if (pis)
		destroy_intJobStat(pis);


	return edg_wll_Error(ctx,NULL,NULL);
}



/*
 * update stored state according to the new event
 * (must be called with the job locked)
 */

edg_wll_ErrorCode edg_wll_StepIntStateParent(edg_wll_Context ctx,
					glite_jobid_const_t job,
					edg_wll_Event *e,
					int seq,
					intJobStat *ijsp,
					edg_wll_JobStat	*stat_out)
{
	int		res;
	int		be_strict = 0;
	char		*errstring = NULL;
	edg_wll_JobStat	oldstat;
	char 		*oldstat_rgmaline = NULL;


	memset(&oldstat,0,sizeof oldstat);

	edg_wll_CpyStatus(&ijsp->pub,&oldstat);

	if (ctx->rgma_export) oldstat_rgmaline = write2rgma_statline(ijsp);

	res = processEvent(ijsp, e, seq, be_strict, &errstring);
	if (res == RET_FATAL || res == RET_INTERNAL) { /* !strict */
		edg_wll_FreeStatus(&oldstat);
		return edg_wll_SetError(ctx, EINVAL, errstring);
	}
	// XXX: store it in update_parent status ?? 
	edg_wll_StoreIntState(ctx, ijsp, seq);

	edg_wll_UpdateStatistics(ctx,&oldstat,e,&ijsp->pub);

	if (ctx->rgma_export) write2rgma_chgstatus(ijsp, oldstat_rgmaline);

	if (stat_out) {
		edg_wll_CpyStatus(&ijsp->pub, stat_out);
	}
	edg_wll_FreeStatus(&oldstat);

	return edg_wll_Error(ctx, NULL, NULL);
}



/*
 * update stored state according to the new event
 * (must be called with the job locked)

 * XXX: job is locked on entry, unlocked in this function
 */

edg_wll_ErrorCode edg_wll_StepIntState(edg_wll_Context ctx,
					glite_jobid_const_t job,
					edg_wll_Event *e,
					int seq,
					edg_wll_JobStat	*stat_out)
{
	intJobStat 	*ijsp;
	int		flags = 0;
	int		res;
	int		be_strict = 0;
	char		*errstring = NULL;
	intJobStat	jobstat;
	edg_wll_JobStat	oldstat;
	char 		*oldstat_rgmaline = NULL;


	memset(&oldstat,0,sizeof oldstat);

	if (!edg_wll_LoadIntState(ctx, job, DONT_LOCK, seq - 1, &ijsp)) {
		edg_wll_CpyStatus(&ijsp->pub,&oldstat);

		if (ctx->rgma_export) oldstat_rgmaline = write2rgma_statline(ijsp);

		res = processEvent(ijsp, e, seq, be_strict, &errstring);
		if (res == RET_FATAL || res == RET_INTERNAL) { /* !strict */
			edg_wll_FreeStatus(&oldstat);
			return edg_wll_SetError(ctx, EINVAL, errstring);
		}
		edg_wll_StoreIntState(ctx, ijsp, seq);

		edg_wll_UpdateStatistics(ctx,&oldstat,e,&ijsp->pub);

		/* check whether subjob state change does not change parent state */
		if ((ijsp->pub.parent_job) && (oldstat.state != ijsp->pub.state)) { 
			if (update_parent_status(ctx, &oldstat, ijsp, e))
				return edg_wll_SetError(ctx, EINVAL, "update_parent_status()");
		}

		if (ctx->rgma_export) write2rgma_chgstatus(ijsp, oldstat_rgmaline);

		if (stat_out) {
			memcpy(stat_out,&ijsp->pub,sizeof *stat_out);
			destroy_intJobStat_extension(ijsp);
		}
		else destroy_intJobStat(ijsp);
		free(ijsp);
		edg_wll_FreeStatus(&oldstat);
	}
	else if (!edg_wll_intJobStatus(ctx, job, flags,&jobstat, js_enable_store, 1)) 
	{
		/* FIXME: we miss state change in the case of seq != 0 
		 * Does anybody care? */

		/* FIXME: collection parent status is wrong in this case.
		   However, it should not happen (frequently).
		   Right approach is computing parent status from scratch.
		*/

		edg_wll_UpdateStatistics(ctx,NULL,e,&jobstat.pub);

		if (ctx->rgma_export) write2rgma_status(&jobstat);

		if (stat_out) {
			memcpy(stat_out,&jobstat.pub,sizeof *stat_out);
			destroy_intJobStat_extension(&jobstat);
		}
		else destroy_intJobStat(&jobstat);
	}

	return edg_wll_Error(ctx, NULL, NULL);
}


edg_wll_ErrorCode edg_wll_GetSubjobHistogram(edg_wll_Context ctx, glite_jobid_const_t parent_jobid, int *hist)
{

        char    *stmt = NULL,*out = NULL, *rest = NULL;
        glite_lbu_Statement    sh;
        int     f = -1, i;
	char *jobid_md5;
	intJobStat *ijs = NULL;

	jobid_md5 = edg_wlc_JobIdGetUnique(parent_jobid);

        edg_wll_ResetError(ctx);
        trio_asprintf(&stmt,"select int_status from states where (jobid='%|Ss') AND (version='%|Ss')", jobid_md5, INTSTAT_VERSION);

	free(jobid_md5);

        if (stmt==NULL) {
                return edg_wll_SetError(ctx,ENOMEM, NULL);
        }

        if (edg_wll_ExecSQL(ctx,stmt,&sh) >= 0) {
                f=edg_wll_FetchRow(ctx,sh,1,NULL,&out);
                if (f == 0) {
                        if (out) free(out);
                        out = NULL;
                        edg_wll_SetError(ctx, ENOENT, NULL);
                }
		else {
			// Ready to read the histogram from the record returned
			rest = (char *)calloc(1,strlen(out));	
			ijs = dec_intJobStat(out, &rest);
			for (i=0;i<=EDG_WLL_NUMBER_OF_STATCODES;i++) hist[i] = ijs->pub.children_hist[i];
		}
        }
        glite_lbu_FreeStmt(&sh);
        free(stmt);
	if (rest==NULL) free(rest);
	if (ijs==NULL) free(rest);



	return edg_wll_Error(ctx, NULL, NULL);
}

/* Make a histogram of all subjobs belonging to the parent job */

edg_wll_ErrorCode edg_wll_StoreSubjobHistogram(edg_wll_Context ctx, glite_jobid_const_t parent_jobid, intJobStat *ijs)
{
        char *stat_enc = NULL;
        char *stmt;
        int dbret;
	char *jobid_md5;

        stat_enc = enc_intJobStat(strdup(""), ijs);

	jobid_md5 = edg_wlc_JobIdGetUnique(parent_jobid);

        trio_asprintf(&stmt,
                "update states set "
                "status=%d,int_status='%|Ss',version='%|Ss'"
                "where jobid='%|Ss'",
                ijs->pub.state, stat_enc, INTSTAT_VERSION, jobid_md5);

	free(jobid_md5);

	if (stmt==NULL) {
		return edg_wll_SetError(ctx,ENOMEM, NULL);
	}

//printf ("\n\n\n Would like to run SQL statament: %s\n\n\n\n", stmt);

        if ((dbret = edg_wll_ExecSQL(ctx,stmt,NULL)) < 0) goto cleanup;

	assert(dbret);	/* update should come through OK as the record exists */

cleanup:
        free(stmt);
        free(stat_enc);

	return edg_wll_Error(ctx, NULL, NULL);

}


