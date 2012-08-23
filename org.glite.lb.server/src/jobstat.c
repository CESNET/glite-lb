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
#ident "$Header$"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <regex.h>

#include "glite/lbu/trio.h"
#include "glite/lb/events.h"
#include "glite/lb/events_json.h"
#include "glite/lb/context-int.h"
#include "glite/lb/intjobstat.h"
#include "glite/lb/process_event.h"
#include "glite/lbu/log.h"

#include "get_events.h"
#include "store.h"
#include "index.h"
#include "jobstat.h"
#include "lb_authz.h"
#include "stats.h"
#include "db_supp.h"
#include "db_calls.h"
#include "authz_policy.h"
#include "server_stats.h"

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


edg_wll_Event* fetch_history(edg_wll_Context ctx, edg_wll_JobStat *stat) {
        edg_wll_QueryRec         jc0[2], *jc[2];
        edg_wll_Event *events = NULL;

        jc[0] = jc0;
        jc[1] = NULL;
        jc[0][0].attr = EDG_WLL_QUERY_ATTR_JOBID;
        jc[0][0].op = EDG_WLL_QUERY_OP_EQUAL;
        jc[0][0].value.j = stat->jobId;
        jc[0][1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

        edg_wll_QueryEventsServer(ctx, 1, (const edg_wll_QueryRec **)jc, NULL, &events);

        return events;
}


int collate_history(edg_wll_Context ctx, edg_wll_JobStat *stat, edg_wll_Event* events, int authz_flags) {
        char                    *event_str = NULL, *history = NULL;
        void *tmpptr;
        size_t i;
        size_t  size, len, maxsize = 1024, newsize;
        char *olduser;
        char *oldacluser;

        if (!events || !events[0].type) {
                history = strdup(HISTORY_EMPTY);
        } else {
                history = malloc(maxsize);
                strcpy(history, HISTORY_HEADER);
                size = HISTORY_HEADER_SIZE;

                oldacluser = NULL;
                olduser = NULL;
                for (i = 0; events && events[i].type; i++) {

                        if ((authz_flags & READ_ANONYMIZED) || (authz_flags & STATUS_FOR_MONITORING)) {
                                //XXX Intorduce a better method of keeping track if more members/types are affected
                                olduser=events[i].any.user;
                                events[i].any.user=(authz_flags & STATUS_FOR_MONITORING) ? NULL : anonymize_string(ctx, events[i].any.user);
                                if (events[i].any.type==EDG_WLL_EVENT_CHANGEACL) {
                                        oldacluser=events[i].changeACL.user_id;
                                        events[i].changeACL.user_id=(authz_flags & STATUS_FOR_MONITORING) ? NULL : anonymize_string(ctx, events[i].changeACL.user_id);
                                }
                        }

                        if (edg_wll_UnparseEventJSON(ctx, events + i, &event_str) != 0) goto err;

                        if (olduser) {
                                free(events[i].any.user);
                                events[i].any.user=olduser;
                                olduser = NULL;
                        }
                        if (oldacluser) {
                                free(events[i].changeACL.user_id);
                                events[i].changeACL.user_id=oldacluser;
                                oldacluser = NULL;
                        }
                        len = strlen(event_str);
                        newsize = size + len + HISTORY_SEPARATOR_SIZE + HISTORY_FOOTER_SIZE + 1;
                        if (newsize > maxsize) {
                                maxsize <<= 1;
                                if (newsize > maxsize) maxsize = newsize;
                                if ((tmpptr = realloc(history, maxsize)) == NULL) {
                                        edg_wll_SetError(ctx, ENOMEM, NULL);
                                        goto err;
                                }
                                history = tmpptr;
                        }
                        strncpy(history + size, event_str, len + 1);
                        size += len;
                        if (events[i+1].type) {
                                strcpy(history + size, HISTORY_SEPARATOR);
                                size += HISTORY_SEPARATOR_SIZE;
                        }
                        free(event_str);
                        event_str = NULL;
                }
                strcpy(history + size, HISTORY_FOOTER);
                size += HISTORY_FOOTER_SIZE;
                stat->history = history;
                history = NULL;
        }

        err:
        free(history);
        return edg_wll_Error(ctx, NULL, NULL);
}


int clear_history(edg_wll_Context ctx, edg_wll_Event* events) {
        int i;

        for (i = 0; events && events[i].type; i++)
                edg_wll_FreeEvent(&events[i]);
        free(events);

        return edg_wll_Error(ctx, NULL, NULL);
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
	int authz_flags = 0;
	struct _edg_wll_GssPrincipal_data peer;

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
			memcpy(stat, intJobStat_to_JobStat(ijsp), sizeof(*stat));
			destroy_intJobStat_extension(ijsp);
			free(ijsp);
		} else {
			if (edg_wll_intJobStatus(ctx, job, flags,&jobstat, js_enable_store, 0)) {
				goto rollback;
			}
			memcpy(stat, intJobStat_to_JobStat(&jobstat), sizeof(*stat));
		}
		
		if (edg_wll_GetACL(ctx, job, &acl)) goto rollback;

		
		memset(&peer, 0, sizeof(peer));
		peer.name = ctx->peerName;
		peer.fqans = ctx->fqans;
		if (check_jobstat_authz(ctx, stat, flags, acl, &peer, &authz_flags) == 0) {
			edg_wll_SetError(ctx, EPERM, "not owner");
			goto rollback;
		}
			
		if (acl) {
			edg_wll_acl_print(ctx, acl, &stat->access_rights);
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
			// !! if adding something here, add it also to edg_wll_NotifJobStatus() !!
		}

	#if DAG_ENABLE
		if (stat->jobtype == EDG_WLL_STAT_DAG || 
			stat->jobtype == EDG_WLL_STAT_COLLECTION || 
			stat->jobtype == EDG_WLL_STAT_FILE_TRANSFER_COLLECTION) {

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
					glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
					num_sub = edg_wll_ExecSQL(ctx, stmt, &sh);
					if (num_sub >=0 ) {
						i = 0;
						stat->children_states = calloc(num_sub+1, sizeof(edg_wll_JobStat));
						if (stat->children_states == NULL) {
							edg_wll_SetError(ctx, ENOMEM, "edg_wll_JobStatusServer() calloc children_states failed!");
							goto rollback;
						}
						while ((num_f = edg_wll_FetchRow(ctx, sh, sizeof(out_stat)/sizeof(out_stat[0]), NULL, out_stat)) == 3
							&& i < num_sub) {
							if (!strcmp(INTSTAT_VERSION,out_stat[0])) {
								js = dec_intJobStat(out_stat[1], &s_out);
								if (s_out != NULL && js != NULL) {
									stat->children_states[i] = *intJobStat_to_JobStat(js);
									destroy_intJobStat_extension(js);
									free(js);
									i++; // Careful, this value will also be used further
								}
							}
							else { // recount state
								glite_jobid_t	subjob;
								intJobStat	js_real;
								char		*name;
								unsigned int		port;


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
								stat->children_states[i] = *intJobStat_to_JobStat(js);
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
						glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
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
									unsigned int		port;


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
									num_f = intJobStat_to_JobStat(js)->state;
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
					glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
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
		if (flags & EDG_WLL_NOTIF_HISTORY) {
			edg_wll_Event* events = NULL;
			if (!(events = fetch_history(ctx, stat))) {
				edg_wll_SetError(ctx, ctx->errCode, "Error extracting job history");
			}
			else {
				collate_history(ctx, stat, events, authz_flags);
			}
		}

		whole_cycle = 1;
rollback:
		if (!whole_cycle) { 
			edg_wll_FreeStatus(intJobStat_to_JobStat(&jobstat));
			memset(stat, 0, sizeof(*stat));
		}
		destroy_intJobStat_extension(&jobstat);
		if (acl) { edg_wll_FreeAcl(acl); acl = NULL; }
		if (stmt) { free(stmt); stmt = NULL; }
		if (sh) { glite_lbu_FreeStmt(&sh); sh = NULL; }

        } while (edg_wll_TransNeedRetry(ctx));

	free(string_jobid);
	free(md5_jobid);

	if (authz_flags & STATUS_FOR_MONITORING)
		blacken_fields(stat, authz_flags);

	if (authz_flags & READ_ANONYMIZED)
		anonymize_stat(ctx, stat);

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
	glite_lbu_Statement	sh;
	char			*stmt, *out;
	struct timeval		ts, maxts = {tv_sec:0, tv_usec:0};

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
	if (intJobStat_to_JobStat(intstat)->owner == NULL) {
		md5_jobid = edg_wlc_JobIdGetUnique(job);
		if ( !(intJobStat_to_JobStat(intstat)->owner = job_owner(ctx,md5_jobid)) ) {
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
				if (edg_wll_Error(ctx, NULL, NULL) != ENOENT) {
					destroy_intJobStat(intstat);
					free(jqra);
					free(intJobStat_to_JobStat(intstat)->owner); intJobStat_to_JobStat(intstat)->owner = NULL;
					return edg_wll_Error(ctx, NULL, NULL);
				}
			}
		}
		else {
			free(jqra);
			free(intJobStat_to_JobStat(intstat)->owner); intJobStat_to_JobStat(intstat)->owner = NULL;
                	return edg_wll_Error(ctx, NULL, NULL);
		}
	}
	edg_wll_ResetError(ctx);

	{
		free(jqra);

		for (num_events = 0; events && events[num_events].type != EDG_WLL_EVENT_UNDEF;
			num_events++);

		for (i = 0; i < num_events; i++) {
			res = processEvent(intstat, &events[i], i, be_strict, &errstring);
			if (res == RET_FATAL || res == RET_INTERNAL) { /* !strict */
				intErr = 1; break;
			}
			ts = events[i].any.timestamp;
			if ((!maxts.tv_sec && !maxts.tv_usec)
			    || (ts.tv_sec > maxts.tv_sec)
			    || (ts.tv_sec == maxts.tv_sec && ts.tv_usec > maxts.tv_usec)) maxts = ts;
			// event has been processed by state machine, refresh statistics
			edg_wll_ServerStatisticsIncrement(ctx, SERVER_STATS_JOB_EVENTS);
			// check if this is job registration (counted only here)
			if (events[i].type == EDG_WLL_EVENT_REGJOB){
				switch (intstat->pub.jobtype) {
                                case EDG_WLL_STAT_SIMPLE:
                                case EDG_WLL_STAT_DAG:
                                case EDG_WLL_STAT_COLLECTION:
                                        edg_wll_ServerStatisticsIncrement(ctx, SERVER_STATS_GLITEJOB_REGS);
                                        break;
                                case EDG_WLL_STAT_PBS:
                                        edg_wll_ServerStatisticsIncrement(ctx, SERVER_STATS_PBSJOB_REGS);
                                        break;
                                case EDG_WLL_STAT_CONDOR:
                                        edg_wll_ServerStatisticsIncrement(ctx, SERVER_STATS_CONDOR_REGS);
                                        break;
                                case EDG_WLL_STAT_CREAM:
                                        edg_wll_ServerStatisticsIncrement(ctx, SERVER_STATS_CREAM_REGS);
                                        break;
                                case EDG_WLL_STAT_FILE_TRANSFER:
                                case EDG_WLL_STAT_FILE_TRANSFER_COLLECTION:
                                        edg_wll_ServerStatisticsIncrement(ctx, SERVER_STATS_SANDBOX_REGS);
                                        break;
                                default:
                                        glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_DEBUG, "Unknown job type, registration will not be counted in statistics.");
                                        break;
                                }
			}
		}
		/* no events or status computation error */
		if (intJobStat_to_JobStat(intstat)->state == EDG_WLL_JOB_UNDEF) {
			intJobStat_to_JobStat(intstat)->state = EDG_WLL_JOB_UNKNOWN;
			if (num_events) intJobStat_to_JobStat(intstat)->lastUpdateTime = maxts;
			else intJobStat_to_JobStat(intstat)->lastUpdateTime.tv_sec = 1;
		}


		for (i=0; i < num_events ; i++) edg_wll_FreeEvent(&events[i]);
		free(events);
	}

	if (intErr) {
		destroy_intJobStat(intstat);
		return edg_wll_SetError(ctx, EDG_WLL_ERROR_SERVER_RESPONSE, errstring);
	} else {
		/* XXX intstat->pub.expectUpdate = eval_expect_update(intstat, &intstat->pub.expectFrom); */
		intErr = edg_wlc_JobIdDup(job, &(intJobStat_to_JobStat(intstat)->jobId));
		if (intErr) return edg_wll_SetError(ctx, intErr, NULL);

		/* don't update status of grey jobs */
		md5_jobid = glite_jobid_getUnique(job);
		trio_asprintf(&stmt, "select grey from jobs where jobid='%|Ss'", md5_jobid);
		free(md5_jobid);
		if (edg_wll_ExecSQL(ctx, stmt, &sh) < 0 ||
		    (res = edg_wll_FetchRow(ctx, sh, 1, NULL, &out)) < 0) {
			free(stmt);
			return edg_wll_Error(ctx, NULL, NULL);
		}
		if (!out || strcmp(out, "0") != 0) update_db = 0;
		glite_lbu_FreeStmt(&sh);
		free(stmt);
		free(out);

		if (update_db) {
			int tsq = num_events - 1;
			if (add_fqans && tsq == 0 && ctx->fqans != NULL) {
				for (i=0; ctx->fqans[i]; i++);
				intJobStat_to_JobStat(intstat)->user_fqans = malloc(sizeof(*ctx->fqans)*(i+1));
				for (i=0; ctx->fqans[i]; i++) {
					intJobStat_to_JobStat(intstat)->user_fqans[i] = strdup(ctx->fqans[i]);
				}
				intJobStat_to_JobStat(intstat)->user_fqans[i] = NULL;
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
		glite_jobid_free(parent_job);
		free(jc);
		free(ec);
		return edg_wll_Error(ctx, NULL, NULL);
	}
	glite_jobid_free(parent_job);
	free(jc);
	free(ec);

	/* recreate job status of subjob */
	err = intJobStat_embryonic(ctx, job, (const edg_wll_RegJobEvent *) &(events_p[0]), intstat);
	//err = intJobStat_embryonic(ctx, job,  &(events_p[0].regJob), intstat);

	for (i=0; events_p[i].type != EDG_WLL_EVENT_UNDEF ; i++) 
		edg_wll_FreeEvent(&events_p[i]);
	free(events_p);

	if (err) goto err;

err:
	return edg_wll_Error(ctx, NULL, NULL);
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
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
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
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	if (edg_wll_ExecSQL(ctx,stmt,&sh) < 0) goto err;

	if (!edg_wll_FetchRow(ctx,sh,1,NULL,&out)) {
		edg_wll_SetError(ctx,ENOENT,md5_jobid);
		goto err;
	}

	if (strcmp(out, "*no parent job*") == 0) {
		edg_wll_SetError(ctx,ENOENT,"No parent job found.");
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


	/* check size of intstat version, its only varchar(32) */
	assert(strlen(INTSTAT_VERSION) <= 32);
	
	update = (seq > 0);
	jobid_md5 = edg_wlc_JobIdGetUnique(intJobStat_to_JobStat(stat)->jobId);
	stat_enc = enc_intJobStat(strdup(""), stat);

	tagp = intJobStat_to_JobStat(stat)->user_tags;
	if (tagp) {
		while ((*tagp).tag != NULL) {
			trio_asprintf(&stmt, "insert into status_tags"
					"(jobid,seq,name,value) values "
					"('%|Ss',%d,'%|Ss','%|Ss')",
					jobid_md5, seq, (*tagp).tag, (*tagp).value);
			glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, 
				LOG_PRIORITY_DEBUG, stmt);

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

	parent_md5 = edg_wlc_JobIdGetUnique(intJobStat_to_JobStat(stat)->parent_job);
	if (parent_md5 == NULL) parent_md5 = strdup("*no parent job*");


	edg_wll_IColumnsSQLPart(ctx, ctx->job_index_cols, intJobStat_to_JobStat(stat), 0, NULL, &icvalues);

	trio_asprintf(&stmt,
		"update states set "
		"status=%d,seq=%d,int_status='%|Ss',version='%|Ss'"
			",parent_job='%|Ss'%s "
		"where jobid='%|Ss'",
		intJobStat_to_JobStat(stat)->state, seq, stat_enc, INTSTAT_VERSION,
		parent_md5, icvalues,
		jobid_md5);
	free(icvalues);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	if ((dbret = edg_wll_ExecSQL(ctx,stmt,NULL)) < 0) goto cleanup;
	free(stmt); stmt = NULL;

	if (dbret == 0) {
		edg_wll_IColumnsSQLPart(ctx, ctx->job_index_cols, intJobStat_to_JobStat(stat), 1, &icnames, &icvalues);
		trio_asprintf(&stmt,
			"insert into states"
			"(jobid,status,seq,int_status,version"
				",parent_job%s) "
			"values ('%|Ss',%d,%d,'%|Ss','%|Ss','%|Ss'%s)",
			icnames,
			jobid_md5, intJobStat_to_JobStat(stat)->state, seq, stat_enc,
			INTSTAT_VERSION, parent_md5, icvalues);
		free(icnames); free(icvalues);

		glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, 
			LOG_PRIORITY_DEBUG, stmt);
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
		glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, 
			LOG_PRIORITY_DEBUG, stmt);
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
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
	
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
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
	
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

	if (edg_wll_LoadIntState(ctx, intJobStat_to_JobStat(cis)->parent_job, LOCK, - 1, pis))
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

	if (!edg_wll_SetSequenceCode(ctx,intJobStat_getLastSeqcode(pis),EDG_WLL_SEQ_NORMAL)) {
		ctx->p_source = EDG_WLL_SOURCE_LB_SERVER;
                edg_wll_IncSequenceCode(ctx);
        }
	event->any.seqcode = edg_wll_GetSequenceCode(ctx);
	edg_wlc_JobIdDup(intJobStat_to_JobStat(pis)->jobId, &(event->any.jobId));
	gettimeofday(&event->any.timestamp,0);
	if (ctx->p_host) event->any.host = strdup(ctx->p_host);
	event->any.level = ctx->p_level;
	event->any.source = EDG_WLL_SOURCE_LB_SERVER; 
		
					
	event->collectionState.state = edg_wll_StatToString(state);
	event->collectionState.done_code = done_code;
	event->collectionState.histogram = hist_to_string(intJobStat_to_JobStat(pis)->children_hist);
	edg_wlc_JobIdDup(intJobStat_to_JobStat(cis)->jobId, &(event->collectionState.child));
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
		case EDG_WLL_JOB_CANCELLED:
			return(SUBJOB_CLASS_CLEARED);
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
	if (intJobStat_to_JobStat(pis)->children_hist[class_to_statCode(SUBJOB_CLASS_RUNNING)+1] > 0) {
		return EDG_WLL_JOB_RUNNING;
	}
	else if (intJobStat_to_JobStat(pis)->children_hist[class_to_statCode(SUBJOB_CLASS_CLEARED)+1] 
		== intJobStat_to_JobStat(pis)->children_num) {
		return EDG_WLL_JOB_CLEARED;
	}
	else if (intJobStat_to_JobStat(pis)->children_hist[class_to_statCode(SUBJOB_CLASS_DONE)+1] 
			+ intJobStat_to_JobStat(pis)->children_hist[class_to_statCode(SUBJOB_CLASS_CLEARED)+1] 
			== intJobStat_to_JobStat(pis)->children_num) {
		return EDG_WLL_JOB_DONE;
	}
	else if (intJobStat_to_JobStat(pis)->children_hist[class_to_statCode(SUBJOB_CLASS_ABORTED)+1]
			+ intJobStat_to_JobStat(pis)->children_hist[class_to_statCode(SUBJOB_CLASS_DONE)+1]
			+ intJobStat_to_JobStat(pis)->children_hist[class_to_statCode(SUBJOB_CLASS_CLEARED)+1] 
			== intJobStat_to_JobStat(pis)->children_num) {
		return EDG_WLL_JOB_ABORTED;
	}
	else
		return EDG_WLL_JOB_WAITING;
}

static edg_wll_JobStatCode process_FT_Histogram(intJobStat *pis)
{
	if (intJobStat_to_JobStat(pis)->children_hist[class_to_statCode(SUBJOB_CLASS_RUNNING)+1] > 0) {
                return EDG_WLL_JOB_RUNNING;
        }
	else if (intJobStat_to_JobStat(pis)->children_hist[class_to_statCode(SUBJOB_CLASS_DONE)+1] 
		== intJobStat_to_JobStat(pis)->children_num) {
		return EDG_WLL_JOB_DONE;
	}
	else if (intJobStat_to_JobStat(pis)->children_hist[class_to_statCode(SUBJOB_CLASS_DONE)+1]
		+ intJobStat_to_JobStat(pis)->children_hist[class_to_statCode(SUBJOB_CLASS_ABORTED)+1]
		== intJobStat_to_JobStat(pis)->children_num)
		return EDG_WLL_JOB_ABORTED;
	else
		return EDG_WLL_JOB_WAITING;
}

static edg_wll_ErrorCode update_parent_status(edg_wll_Context ctx, edg_wll_JobStat *subjob_stat_old, intJobStat *cis, edg_wll_Event *ce)
{
	intJobStat		*pis = NULL;
	subjobClassCodes	subjob_class, subjob_class_old;
	edg_wll_JobStatCode	parent_new_state;


	subjob_class = class(intJobStat_to_JobStat(cis));
	subjob_class_old = class(subjob_stat_old);


	if (subjob_class_old != subjob_class) {
		if (load_parent_intJobStat(ctx, cis, &pis)) goto err;

		intJobStat_to_JobStat(pis)->children_hist[class_to_statCode(subjob_class)+1]++;
		intJobStat_to_JobStat(pis)->children_hist[class_to_statCode(subjob_class_old)+1]--;

		edg_wll_StoreSubjobHistogram(ctx, intJobStat_to_JobStat(cis)->parent_job, pis);


		if (intJobStat_to_JobStat(pis)->jobtype == EDG_WLL_STAT_COLLECTION) {
			parent_new_state = process_Histogram(pis);
			if (intJobStat_to_JobStat(pis)->state != parent_new_state) {
				// XXX: we do not need  EDG_WLL_STAT_code any more
				//	doneFailed subjob is stored in REST class and
				//	inducting collection Waiting state
				//	-> in future may be removed from collectionState event
				//	   supposing collection Done state to be always DoneOK
				if (log_collectionState_event(ctx, parent_new_state, EDG_WLL_STAT_OK, cis, pis, ce))
					goto err;
			}
		}
		else if (intJobStat_to_JobStat(pis)->jobtype == EDG_WLL_STAT_FILE_TRANSFER_COLLECTION) {
			parent_new_state = process_FT_Histogram(pis);
			if (intJobStat_to_JobStat(pis)->state != parent_new_state) {
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
					edg_wll_JobStat *oldstat,
					edg_wll_JobStat	*stat_out)
{
	int		res;
	int		be_strict = 0;
	char		*errstring = NULL;
	char 		*oldstat_rgmaline = NULL;


	edg_wll_CpyStatus(intJobStat_to_JobStat(ijsp),oldstat);

	if (ctx->rgma_export) oldstat_rgmaline = write2rgma_statline(ijsp);

	res = processEvent(ijsp, e, seq, be_strict, &errstring);
	if (res == RET_FATAL || res == RET_INTERNAL) { /* !strict */
		edg_wll_FreeStatus(oldstat);
		memset(oldstat,0,sizeof *oldstat);
		return edg_wll_SetError(ctx, EINVAL, errstring);
	}
	// XXX: store it in update_parent status ?? 
	edg_wll_StoreIntState(ctx, ijsp, seq);

	edg_wll_UpdateStatistics(ctx, oldstat, e, intJobStat_to_JobStat(ijsp));

	// event has been processed by state machine, refresh statistics 
        edg_wll_ServerStatisticsIncrement(ctx, SERVER_STATS_JOB_EVENTS);

	if (ctx->rgma_export) write2rgma_chgstatus(ijsp, oldstat_rgmaline);

	if (stat_out) {
		edg_wll_CpyStatus(intJobStat_to_JobStat(ijsp), stat_out);
	}

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
					edg_wll_JobStat	*oldstat,
					edg_wll_JobStat	*stat_out)
{
	intJobStat 	*ijsp;
	int		flags = 0;
	int		res;
	int		be_strict = 0;
	char		*errstring = NULL;
	intJobStat	jobstat;
	char 		*oldstat_rgmaline = NULL;


	if (!edg_wll_LoadIntState(ctx, job, DONT_LOCK, seq - 1, &ijsp)) {
		edg_wll_CpyStatus(intJobStat_to_JobStat(ijsp),oldstat);

		if (ctx->rgma_export) oldstat_rgmaline = write2rgma_statline(ijsp);

		res = processEvent(ijsp, e, seq, be_strict, &errstring);
		if (res == RET_FATAL || res == RET_INTERNAL) { /* !strict */
			edg_wll_FreeStatus(oldstat);
			memset(oldstat,0,sizeof *oldstat);
			return edg_wll_SetError(ctx, EINVAL, errstring);
		}
		edg_wll_StoreIntState(ctx, ijsp, seq);

		edg_wll_UpdateStatistics(ctx, oldstat, e, intJobStat_to_JobStat(ijsp));

		// event has been processed by state machine, refresh statistics 
                edg_wll_ServerStatisticsIncrement(ctx, SERVER_STATS_JOB_EVENTS);

		/* check whether subjob state change does not change parent state */
		if ((intJobStat_to_JobStat(ijsp)->parent_job) 
			&& (oldstat->state != intJobStat_to_JobStat(ijsp)->state)) { 
			if (update_parent_status(ctx, oldstat, ijsp, e)) {
				edg_wll_FreeStatus(oldstat);
				memset(oldstat,0,sizeof *oldstat);
				return edg_wll_SetError(ctx, EINVAL, "update_parent_status()");
			}
		}

		if (ctx->rgma_export) write2rgma_chgstatus(ijsp, oldstat_rgmaline);

		if (stat_out) {
			memcpy(stat_out,intJobStat_to_JobStat(ijsp),sizeof *stat_out);
			destroy_intJobStat_extension(ijsp);
		}
		else destroy_intJobStat(ijsp);
		free(ijsp);
	}
	else if (!edg_wll_intJobStatus(ctx, job, flags,&jobstat, js_enable_store, 1)) 
	{
		/* FIXME: we miss state change in the case of seq != 0 
		 * Does anybody care? */

		/* FIXME: collection parent status is wrong in this case.
		   However, it should not happen (frequently).
		   Right approach is computing parent status from scratch.
		*/

		edg_wll_UpdateStatistics(ctx, NULL, e, intJobStat_to_JobStat(&jobstat));

		if (ctx->rgma_export) write2rgma_status(&jobstat);

		if (stat_out) {
			memcpy(stat_out, intJobStat_to_JobStat(&jobstat), sizeof *stat_out);
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
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

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
			for (i=0;i<=EDG_WLL_NUMBER_OF_STATCODES;i++) hist[i] = intJobStat_to_JobStat(ijs)->children_hist[i];
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
                intJobStat_to_JobStat(ijs)->state, stat_enc, INTSTAT_VERSION, jobid_md5);

	free(jobid_md5);

	if (stmt==NULL) {
		return edg_wll_SetError(ctx,ENOMEM, NULL);
	}

//printf ("\n\n\n Would like to run SQL statament: %s\n\n\n\n", stmt);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

        if ((dbret = edg_wll_ExecSQL(ctx,stmt,NULL)) < 0) goto cleanup;

	assert(dbret);	/* update should come through OK as the record exists */

cleanup:
        free(stmt);
        free(stat_enc);

	return edg_wll_Error(ctx, NULL, NULL);

}

