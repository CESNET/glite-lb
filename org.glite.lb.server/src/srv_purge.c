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


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#include "glite/jobid/cjobid.h"
#include "glite/lbu/trio.h"

#include "glite/lbu/log.h"

#include "glite/lb/context-int.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/mini_http.h"
#include "glite/lb/ulm_parse.h"

#include "lb_html.h"
#include "lb_proto.h"
#include "store.h"
#include "query.h"
#include "get_events.h"
#include "purge.h"
#include "lb_xml_parse.h"
#include "db_calls.h"
#include "db_supp.h"
#include "jobstat.h"
#include "il_notification.h"
#include "authz_policy.h"


#define DUMP_FILE_STORAGE					"/tmp/"

#define sizofa(a) (sizeof(a)/sizeof((a)[0]))

typedef struct {
	int naffected_jobs;
	int parse;
	int dumpfile;

	int jobs_to_exa;
	double purge_end;
	double time_per_job;
	time_t target_runtime;
} purge_ctx_t;

extern volatile sig_atomic_t purge_quit;

static const char* const resp_headers[] = {
	"Cache-Control: no-cache",
	"Server: edg_wll_Server/" PROTO_VERSION "/" COMP_PROTO,
	"Content-Type: application/x-dglb",
	NULL
};

static int purge_check(edg_wll_Context ctx, edg_wll_JobStat *stat, time_t start, time_t *timeout);
static int purge_one_with_subjobs(edg_wll_Context ctx, purge_ctx_t *prg, edg_wll_JobStat *stat, const edg_wll_PurgeRequest *request, edg_wll_PurgeResult *result);
static int purge_one(edg_wll_Context ctx,edg_wll_JobStat *,int,int,int);
int unset_proxy_flag(edg_wll_Context ctx, glite_jobid_const_t job);
static int unset_server_flag(edg_wll_Context ctx, glite_jobid_const_t job);
int job_exists(edg_wll_Context ctx, glite_jobid_const_t job);
static void purge_throttle(purge_ctx_t *prg);


int edg_wll_CreateTmpFileStorage(edg_wll_Context ctx, char *prefix, char **fname)
{
	char			fname_buf[1024];
	struct timeval	tv;
	int				retfd;


	while ( 1 )
	{
		gettimeofday(&tv, NULL);
		snprintf(fname_buf, 1024, "%s/%ld_%ld", prefix, tv.tv_sec, tv.tv_usec);

		if ( (retfd = open(fname_buf, O_WRONLY|O_CREAT|O_EXCL|O_APPEND,0600)) == -1 )
		{
			if ( errno != EEXIST )
			{
				char buff[100];

				sprintf(buff, "couldn't create temporary server file");
				edg_wll_SetError(ctx, errno, buff);
				return -1;
			}
		}
		else
			break;
	}

	*fname = strdup(fname_buf);

	return retfd;
}

int edg_wll_CreateFileStorageFromTmp(edg_wll_Context ctx, char *old_file, char *file_type, char **fname)
{
	char			fname_buf[1024],
					fname_prefix[1024],
					stimebuf[100];
	char		   *stmp;
	struct timeval	tv;
	int				retfd;


	if ( (stmp = strrchr(old_file, '/')) )
	{
		strncpy(fname_prefix, old_file, stmp - old_file);
		fname_prefix[stmp - old_file] = '\0';
	}

	while ( 1 )
	{
		gettimeofday(&tv, NULL);
		edg_wll_ULMTimevalToDate(tv.tv_sec, tv.tv_usec, stimebuf);
		snprintf(fname_buf, 1024, "%s/%s_%s", fname_prefix, file_type, stimebuf);

		if ( !link(old_file, fname_buf) )
			break;

		if ( errno != EEXIST )
		{
			char buff[100];

			sprintf(buff, "couldn't create server %s file", file_type);
			return edg_wll_SetError(ctx, errno, buff);
		}
	}

	*fname = strdup(fname_buf);

	return retfd;
}

int edg_wll_CreateFileStorage(edg_wll_Context ctx, char *file_type, char *prefix, char **fname)
{
	char			fname_buf[1024],
					stimebuf[100];
	struct timeval	tv;
	int				retfd;

	if ( *fname )
	{
		snprintf(fname_buf, 1024, "%s/%s", prefix? prefix: "", *fname);
		if ( (retfd = open(*fname, O_WRONLY|O_CREAT|O_EXCL|O_APPEND,0600)) == -1 )
		{
			char buff[100];
			if ( errno == EEXIST )
				sprintf(buff, "Server file %s exist", fname_buf);
			else
				sprintf(buff, "Server couldn't create file %s", fname_buf);
			edg_wll_SetError(ctx, errno, buff);

			return -1;
		}
		if ( prefix )
		{
			free(*fname);
			*fname = strdup(fname_buf);
		}

		return retfd;
	}

	while ( 1 )
	{
		gettimeofday(&tv, NULL);
		edg_wll_ULMTimevalToDate(tv.tv_sec, tv.tv_usec, stimebuf);
		if ( !prefix )
		{
			if ( strcmp(file_type,FILE_TYPE_PURGE) == 0 )
				prefix = ctx->purgeStorage;
			else if ( strcmp(file_type,FILE_TYPE_DUMP) == 0 )
				prefix = ctx->dumpStorage;
			else
				prefix = "";
		}
		snprintf(fname_buf, 1024, "%s/%s_%s", prefix, file_type, stimebuf);

		if ( (retfd = open(fname_buf, O_WRONLY|O_CREAT|O_EXCL|O_APPEND,0600)) == -1 )
		{
			if ( errno != EEXIST )
			{
				char buff[100];

				sprintf(buff, "couldn't create server %s file", file_type);
				edg_wll_SetError(ctx, errno, buff);

				return -1;
			}
		}
		else
			break;
	}

	*fname = strdup(fname_buf);

	return retfd;
}

int edg_wll_PurgeServerProxy(edg_wll_Context ctx, glite_jobid_const_t job)
{
	edg_wll_JobStat stat;

	memset(&stat, 0, sizeof stat);
	stat.jobId = (glite_jobid_t)job;
	switch ( purge_one(ctx, &stat, -1, 1, 1) ) {
		case 0:
		case ENOENT:
			return(edg_wll_ResetError(ctx));
			break;
		default:
			return(edg_wll_Error(ctx,NULL,NULL));
			break;
	}
}

int edg_wll_PurgeServer(edg_wll_Context ctx,const edg_wll_PurgeRequest *request, edg_wll_PurgeResult *result)
{
	int	i;
	glite_jobid_t	job;
	char	*tmpfname = NULL;
	int	ret;
	double		now;
	struct timeval	tp;
	edg_wll_JobStat	stat;
	purge_ctx_t prg;

	memset(&prg, 0, sizeof prg);
	prg.naffected_jobs = 0;
	prg.parse = 0;
	prg.dumpfile = -1;

	if (!ctx->noAuth && !check_authz_policy_ctx(ctx, PURGE)) {
		edg_wll_SetError(ctx,EPERM,"only superusers may purge");
		goto abort;
	}

	edg_wll_ResetError(ctx);
	memset(result, 0, sizeof(*result));

	if ( (request->flags & EDG_WLL_PURGE_SERVER_DUMP) && 
		 ((prg.dumpfile = edg_wll_CreateTmpPurgeFile(ctx, &tmpfname)) == -1 ) )
		goto abort;

	/* 
	should be changed so that only purged events are sent to whole-server dumps
	(with this commented out, severely delayed events (>purge interval) can miss
	whole-server dumps, but it is more acceptable than invoking whole-server dump
	on each purge request (whole-server dumps are used rarely if at all)
	if (request->flags&EDG_WLL_PURGE_REALLY_PURGE) {
		edg_wll_DumpRequest	req = {
			EDG_WLL_DUMP_LAST_END, EDG_WLL_DUMP_NOW
		};
		edg_wll_DumpResult	res;

		if (edg_wll_DumpEventsServer(ctx,&req,&res)) 
		{
			if ( request->flags & EDG_WLL_PURGE_SERVER_DUMP )
				unlink(tmpfname);
			goto abort;
		}
	}
	*/

	/* throttle parameters */
	gettimeofday(&tp, NULL);
	now = tp.tv_sec + (double)tp.tv_usec / 1000000.0;
	prg.purge_end = now + request->target_runtime;
	prg.target_runtime = request->target_runtime;
	prg.time_per_job = -1.0;

	if (request->jobs) {

	for (prg.jobs_to_exa=0; request->jobs[prg.jobs_to_exa]; prg.jobs_to_exa++);
	for (i=0; request->jobs[i] && !purge_quit; i++) {
		if (edg_wlc_JobIdParse(request->jobs[i],&job)) {
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "%s: parse error", request->jobs[i]);
			prg.parse = 1;
			prg.jobs_to_exa--;
		}
		else {
			if (check_strict_jobid(ctx,job)) {
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "%s: not my job", request->jobs[i]);
				prg.parse = 1;
				prg.jobs_to_exa--;
			}
			else {
				if (purge_quit) break;

				memset(&stat,0,sizeof stat);
				if (edg_wll_JobStatusServer(ctx,job,EDG_WLL_STAT_CHILDSTAT | EDG_WLL_STAT_CHILDREN,&stat)) {
					if (edg_wll_Error(ctx, NULL, NULL) == ENOENT) {
						/* job purged meanwhile, ignore */
						edg_wll_ResetError(ctx);
						glite_jobid_free(job);
						purge_throttle(&prg);
						continue;
					}
					edg_wll_FreeStatus(&stat);
					goto abort; 
				}

				switch (purge_one_with_subjobs(ctx, &prg, &stat, request, result)) {
					case 0: 
						break;
					case ENOENT: /* job does not exist, consider purged and ignore */
						edg_wll_ResetError(ctx);
						break;
					default: goto abort;
				}

				edg_wll_FreeStatus(&stat);
			}
			glite_jobid_free(job);
		}
	}
	}
	else {
		glite_lbu_Statement	s;
		char		*job_s;
		time_t		timeout[EDG_WLL_NUMBER_OF_STATCODES],
				start = time(NULL);

		for (i=0; i<EDG_WLL_NUMBER_OF_STATCODES; i++)
			if (request->timeout[i] < 0) { // no specific timeout
				if (request->timeout[i] == -2) //no purge
					timeout[i] = request->timeout[i];
				else {// use server default
					timeout[i] = ctx->purge_timeout[i];
				}
			}
			else timeout[i] = request->timeout[i]; //specific given

		glite_common_log(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG,
			(ctx->isProxy) 
			? "select dg_jobid from jobs where proxy='1'" 
			: "select dg_jobid from jobs where server='1'");

		if ((prg.jobs_to_exa = edg_wll_ExecSQL(ctx, (ctx->isProxy) ? "select dg_jobid from jobs where proxy='1'" :
			"select dg_jobid from jobs where server='1'", &s)) < 0) goto abort;

		while (edg_wll_FetchRow(ctx,s,1,NULL,&job_s) > 0 && !purge_quit) {
			if (edg_wlc_JobIdParse(job_s,&job)) {
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "%s: parse error (internal inconsistency !)", job_s);
				prg.parse = 1;
				prg.jobs_to_exa--;
			}
			else {
				if (check_strict_jobid(ctx,job)) {
					edg_wlc_JobIdFree(job);
					free(job_s);
					prg.parse = 1;
					prg.jobs_to_exa--;
					continue;
				}

				if (purge_quit) break;

				memset(&stat,0,sizeof stat);
				if (edg_wll_JobStatusServer(ctx,job,EDG_WLL_STAT_CHILDSTAT | EDG_WLL_STAT_CHILDREN,&stat)) {
					if (edg_wll_Error(ctx, NULL, NULL) == ENOENT) {
						/* job purged meanwhile, ignore */
						edg_wll_ResetError(ctx);
						glite_jobid_free(job);
						free(job_s);
						purge_throttle(&prg);
						continue;
					}
					edg_wll_FreeStatus(&stat);
					goto abort; 
				}

				if (purge_check(ctx, &stat, start, timeout)) {
					if (purge_one_with_subjobs(ctx, &prg, &stat, request, result)) {
						edg_wll_FreeStatus(&stat);
						if (edg_wll_Error(ctx, NULL, NULL) == ENOENT) {
							/* job purged meanwhile, ignore */
							edg_wll_ResetError(ctx);
							glite_jobid_free(job);
							free(job_s);
							continue;
						}
						goto abort;
					}
				}

				glite_jobid_free(job);
				edg_wll_FreeStatus(&stat);
			}
			free(job_s);
		}
		glite_lbu_FreeStmt(&s);
	}

abort:
	if (prg.parse && !edg_wll_Error(ctx,NULL,NULL))
	{
		if ( prg.naffected_jobs ) {
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "[%d] Found only jobs not matching server address/port; these were not purged.", getpid());
		}
		else {
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_INFO, "[%d] Found only jobs not matching server address/port; these were not purged.", getpid());
		}
	}

	ret = edg_wll_Error(ctx,NULL,NULL);
	if (ret == 0 || ret == ENOENT || ret == EPERM || ret == EINVAL) {
		if ( request->flags & EDG_WLL_PURGE_SERVER_DUMP && tmpfname )
		{
			edg_wll_CreatePurgeFileFromTmp(ctx, tmpfname, &(result->server_file));
			unlink(tmpfname);
		}
	}
	
	return edg_wll_Error(ctx,NULL,NULL);
}

/* FIXME: defined but not used */
#if 0
static void unlock_and_check(edg_wll_Context ctx,edg_wlc_JobId job)
{
	char	*job_s,*et,*ed;

	if (edg_wll_UnlockJob(ctx,job)) {
		job_s = edg_wlc_JobIdUnparse(job);

		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"%s: edg_wll_UnlockJob(): %s (%s) -- expect bogus things\n",
				job_s,et,ed);
		syslog(LOG_CRIT,"%s: edg_wll_UnlockJob(): %s (%s) -- expect bogus things",
				job_s,et,ed);
		free(et); free(ed); free(job_s);
	}
}
#endif

static int dump_events(edg_wll_Context ctx, glite_jobid_const_t job, int dump, char **res)
{
	edg_wll_Event	e;
	int		event;


	event = atoi(res[0]);
	free(res[0]); res[0] = NULL;

	res[0] = edg_wlc_JobIdUnparse(job);
	if (convert_event_head(ctx,res,&e) || edg_wll_get_event_flesh(ctx,event,&e))
	{
		char	*et,*ed, *dbjob;


	/* Most likely sort of internal inconsistency. 
	 * Must not be fatal -- just complain
	 */
		edg_wll_Error(ctx,&et,&ed);
		dbjob = edg_wlc_JobIdGetUnique(job);
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, 
			"%s event %d: %s (%s)", dbjob, event, et, ed);
		free(et); free(ed); free(dbjob);
		edg_wll_ResetError(ctx);
	}
	else {
		char	*event_s = edg_wll_UnparseEvent(ctx,&e);
		char    arr_s[100];
		int     len, written, total;

		strcpy(arr_s, "DG.ARRIVED=");
		edg_wll_ULMTimevalToDate(e.any.arrived.tv_sec,
						e.any.arrived.tv_usec,
						arr_s+strlen("DG.ARRIVED="));

		len = strlen(arr_s);
		total = 0;
		while (total != len) {
			written = write(dump,arr_s+total,len-total);
			if (written < 0 && errno != EAGAIN) {
				edg_wll_SetError(ctx,errno,"writing dump file");
				free(event_s);
				return edg_wll_Error(ctx,NULL,NULL);
			}
			total += written;
		}
		write(dump, " ", 1);
		
		len = strlen(event_s);
		total = 0;
		while (total != len) {
			written = write(dump,event_s+total,len-total);
			if (written < 0 && errno != EAGAIN) {
				glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "dump to file");
				dump = -1; /* XXX: likely to be a permanent error
					    * give up writing but do purge */
				break;
			}
			total += written;
		}
		/* write(dump,"\n",1); edg_wll_UnparseEvent does so */
		free(event_s);
	}
	edg_wll_FreeEvent(&e);


	return edg_wll_Error(ctx,NULL,NULL);
}

static enum edg_wll_StatJobtype get_job_type(edg_wll_Context ctx, glite_jobid_const_t job)
{
	edg_wll_JobStat			stat;
	enum edg_wll_StatJobtype	type;

	memset(&stat, 0, sizeof(stat));
	if (edg_wll_JobStatusServer(ctx, job, 0 /*no flags*/, &stat)) {
		edg_wll_FreeStatus(&stat);
                return(EDG_WLL_NUMBER_OF_JOBTYPES);
        }

	type = stat.jobtype;
	edg_wll_FreeStatus(&stat);

	return(type);	
}

static int get_jobid_suffix(edg_wll_Context ctx, glite_jobid_const_t job, enum edg_wll_StatJobtype jobtype, char **unique, char **suffix)
{
	char 		*ptr = NULL, *dbjob = NULL;
	

	dbjob = glite_jobid_getUnique(job);

	switch (jobtype) {
	        case EDG_WLL_STAT_SIMPLE:
	        case EDG_WLL_STAT_DAG:
        	case EDG_WLL_STAT__PARTITIONABLE_UNUSED:
	        case EDG_WLL_STAT__PARTITIONED_UNUSED:
        	case EDG_WLL_STAT_COLLECTION:
		case EDG_WLL_STAT_CREAM:
                case EDG_WLL_STAT_FILE_TRANSFER:
                case EDG_WLL_STAT_FILE_TRANSFER_COLLECTION:
			// glite jobs, no suffix
			*suffix = strdup("");
			*unique = strdup(dbjob);
			break;

	        case EDG_WLL_STAT_PBS:
			// PBS jobs; suffix is everything starting from first '.'
			ptr = strchr(dbjob,'.');
			if (ptr) {
				*suffix = strdup(ptr);
				ptr[0] = '\0';
				*unique = strdup(dbjob);
				ptr[0] = '.';
			}
			else {
				edg_wll_SetError(ctx,EINVAL,"Uknown PBS job format");
				goto err;
			}
			break;

        	case EDG_WLL_STAT_CONDOR:
			// condor jobs
			assert(0); // XXX: todo
			break;
	        default:
			edg_wll_SetError(ctx,EINVAL,"Uknown job type");
			goto err;
			break;
	}

err:	
	free(dbjob);

	return edg_wll_Error(ctx, NULL, NULL);
}

static int get_jobid_prefix(edg_wll_Context ctx, glite_jobid_const_t job, enum edg_wll_StatJobtype jobtype, char **prefix)
{
	char	*ser = NULL;
	

	switch (jobtype) {
	        case EDG_WLL_STAT_SIMPLE:
	        case EDG_WLL_STAT_DAG:
        	case EDG_WLL_STAT__PARTITIONABLE_UNUSED:
	        case EDG_WLL_STAT__PARTITIONED_UNUSED:
        	case EDG_WLL_STAT_COLLECTION:
		case EDG_WLL_STAT_CREAM:
		case EDG_WLL_STAT_FILE_TRANSFER:
		case EDG_WLL_STAT_FILE_TRANSFER_COLLECTION:
			// glite job prefix
			ser = glite_jobid_getServer(job);
			asprintf(prefix,"%s/",ser);
			free(ser);
			break;

	        case EDG_WLL_STAT_PBS:
			// PBS jobs; prefix same as glite job prefix
			ser = glite_jobid_getServer(job);
			asprintf(prefix,"%s/",ser);
			free(ser);
			break;

        	case EDG_WLL_STAT_CONDOR:
			// condor jobs
			assert(0); // XXX: todo
			break;
	        default:
			edg_wll_SetError(ctx,EINVAL,"Uknown job type");
			goto err;
			break;
	}

err:	
	return edg_wll_Error(ctx, NULL, NULL);
}


static int purge_check(edg_wll_Context ctx, edg_wll_JobStat *stat, time_t start, time_t *timeout) {
	int to_purge = 0;
	int i;
	edg_wll_JobStat	*substat;

	if (timeout[stat->state] >=0 && stat->lastUpdateTime.tv_sec && start-stat->lastUpdateTime.tv_sec > timeout[stat->state]
	    // job from this server
	    && !check_strict_jobid(ctx, stat->jobId)
	    // if subjob purge only when orphan
	    && (!stat->parent_job || job_exists(ctx, stat->parent_job) == 0)) {
		to_purge = 1;
	}
	// check the whole collection yet
	if (to_purge && stat->children_num && stat->children_states) {
		for (i = 0; i < stat->children_num && stat->children_states[i].state != EDG_WLL_JOB_UNDEF; i++) {
			substat = stat->children_states + i;
			if (timeout[substat->state] >=0 && substat->lastUpdateTime.tv_sec && start-substat->lastUpdateTime.tv_sec > timeout[substat->state]) ;
			else {
				to_purge = 0;
				break;
			}
		}
	}

	return to_purge;
}


#define GRAN 32

static int purge_one_with_subjobs(edg_wll_Context ctx, purge_ctx_t *prg, edg_wll_JobStat *stat, const edg_wll_PurgeRequest *request, edg_wll_PurgeResult *result) {
	char *job_s;
	int i;

	if (purge_one(ctx,stat,prg->dumpfile,request->flags&EDG_WLL_PURGE_REALLY_PURGE,ctx->isProxy)) return edg_wll_Error(ctx, NULL, NULL);
	purge_throttle(prg);

/* XXX: change with the streaming interface */
	if (request->flags & EDG_WLL_PURGE_LIST_JOBS) {
		job_s = glite_jobid_unparse(stat->jobId);
		if (prg->naffected_jobs % GRAN == 0 || !result->jobs)
			result->jobs = realloc(result->jobs,(prg->naffected_jobs+GRAN+1) * sizeof(*(result->jobs)));
		result->jobs[prg->naffected_jobs] = job_s;
		result->jobs[prg->naffected_jobs+1] = NULL;
	}
	prg->naffected_jobs++;

	/* purge the subjobs */
	if (stat->children_num && stat->children) {
		for (i = 0; i < stat->children_num && stat->children[i]; i++) {
			if (purge_one(ctx, stat->children_states + i, prg->dumpfile, request->flags&EDG_WLL_PURGE_REALLY_PURGE,ctx->isProxy)) {
				return edg_wll_Error(ctx, NULL, NULL);
			}
			purge_throttle(prg);

			if (request->flags & EDG_WLL_PURGE_LIST_JOBS) {
				if (prg->naffected_jobs % GRAN == 0 || !result->jobs)
					result->jobs = realloc(result->jobs,(prg->naffected_jobs+GRAN+1) * sizeof(*(result->jobs)));
				result->jobs[prg->naffected_jobs] = strdup(stat->children[i]);
				result->jobs[prg->naffected_jobs+1] = NULL;
			}
			prg->naffected_jobs++;
		}
	}

	return edg_wll_Error(ctx, NULL, NULL);
}


int purge_one(edg_wll_Context ctx,edg_wll_JobStat *stat,int dump, int purge, int purge_from_proxy_only)
{
	char	*dbjob = NULL;
	char	*stmt = NULL;
	glite_lbu_Statement	q = NULL;
	int		ret,dumped = 0;
	char	*res[10];
	char	*prefix = NULL, *suffix = NULL, *root = NULL;
	char	*prefix_id = NULL, *suffix_id = NULL;
	int	sql_retval;
	glite_jobid_const_t job = stat->jobId;
	edg_wll_JobStat new_stat;

	edg_wll_ResetError(ctx);
	if ( !purge && dump < 0 ) return 0;

	do {

        	if (edg_wll_Transaction(ctx)) goto err;

		switch (edg_wll_jobMembership(ctx, job)) {
			case DB_PROXY_JOB:
				if (!ctx->isProxy) {
					/* should not happen */
					goto commit;
				}
				/* continue */
				break;
			case DB_SERVER_JOB:
				if (purge_from_proxy_only) {
					/* no action needed */
					goto commit;
				}
				if (ctx->isProxy) {
					/* should not happen */
					goto commit;
				}
				/* continue */
				break;
			case DB_PROXY_JOB+DB_SERVER_JOB:
				if (ctx->isProxy) {
					purge = 0;
					if (unset_proxy_flag(ctx, job) < 0) {
						goto rollback;
					}
				}
				else {
					purge = 0;
					/* if server&proxy DB is shared ... */
					if (is_job_local(ctx,job) && purge_from_proxy_only) {
						if (unset_proxy_flag(ctx, job) < 0) {
							goto rollback;
						}
					}
					else {
						if (unset_server_flag(ctx, job) < 0) {
							goto rollback;
						}
					}
				}
				break;
			case 0:
				// Zombie job (server=0, proxy=0)? should not happen;
				// clear it to keep DB healthy
				break;
			default:
				goto rollback;
				break;
		}

		dbjob = glite_jobid_getUnique(job);	/* XXX: strict jobid already checked */


		if ( purge )
		{
			enum edg_wll_StatJobtype jobtype = get_job_type(ctx, job);

			// get job prefix/suffix before its state is deleted
			if ( jobtype == EDG_WLL_NUMBER_OF_JOBTYPES) goto rollback;
			if (get_jobid_suffix(ctx, job, jobtype, &root, &suffix)
			 || get_jobid_prefix(ctx, job, jobtype, &prefix)) {
				glite_common_log(LOG_CATEGORY_CONTROL, 
					LOG_PRIORITY_WARN, 
					"[%d] unknown job type %d of job %s", 
					getpid(), jobtype, dbjob);
				edg_wll_ResetError(ctx);
			}

			// notifications
			memcpy(&new_stat, stat, sizeof new_stat);
			new_stat.state = EDG_WLL_JOB_PURGED;
			edg_wll_NotifMatch(ctx, stat, &new_stat);
		}

		if ( purge )
		{
			trio_asprintf(&stmt,"delete from jobs where jobid = '%|Ss'",dbjob);
			glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
			if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) goto rollback;
			free(stmt); stmt = NULL;

			trio_asprintf(&stmt,"delete from states where jobid = '%|Ss'",dbjob);
			glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
			if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) goto rollback; 
			free(stmt); stmt = NULL;

			trio_asprintf(&stmt,"delete from status_tags where jobid = '%|Ss'",dbjob);
			glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
			if (edg_wll_ExecSQL(ctx,stmt,NULL) < 0) goto rollback;
			free(stmt); stmt = NULL;
		}

		if ( purge && prefix && suffix )
		{
			/* Store zombie prefix */
		
			// See if that prefix is already stored in the database	
			trio_asprintf(&stmt,"select prefix_id from zombie_prefixes where prefix = '%|Ss'", prefix);
			glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

			sql_retval = edg_wll_ExecSQL(ctx,stmt,&q);
			free(stmt); stmt = NULL;

			if (sql_retval < 0) goto rollback;

			if (sql_retval == 0) { //prefix does not exist yet
				glite_lbu_FreeStmt(&q);

				trio_asprintf(&stmt,"insert into zombie_prefixes (prefix) VALUES ('%|Ss')", prefix);
				glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

				if (edg_wll_ExecSQL(ctx,stmt,&q) <= 0) goto rollback;

				free(stmt); stmt = NULL;
				glite_lbu_FreeStmt(&q);

				// The record should exist now, however we need to look up the prefix_id 
				trio_asprintf(&stmt,"select prefix_id from zombie_prefixes where prefix = '%|Ss'", prefix);
				glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

				if (edg_wll_ExecSQL(ctx,stmt,&q) <= 0) goto rollback;
				free(stmt); stmt = NULL;
			}
			ret = edg_wll_FetchRow(ctx,q, 1, NULL, &prefix_id);
			glite_lbu_FreeStmt(&q);


			/* Store zombie suffix */

			// See if that suffix is already stored in the database	
			trio_asprintf(&stmt,"select suffix_id from zombie_suffixes where suffix = '%|Ss'", suffix);
			glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

			sql_retval = edg_wll_ExecSQL(ctx,stmt,&q);
			free(stmt); stmt = NULL;

			if (sql_retval < 0) goto rollback;

			if (sql_retval == 0) { //suffix does not exist yet
				glite_lbu_FreeStmt(&q);

				trio_asprintf(&stmt,"insert into zombie_suffixes (suffix) VALUES ('%|Ss')", suffix);
				glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

				if (edg_wll_ExecSQL(ctx,stmt,&q) <= 0) goto rollback;

				free(stmt); stmt = NULL;
				glite_lbu_FreeStmt(&q);

				// The record should exist now, however we need to look up the suffix_id 
				trio_asprintf(&stmt,"select suffix_id from zombie_suffixes where suffix = '%|Ss'", suffix);
				glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

				if (edg_wll_ExecSQL(ctx,stmt,&q) <= 0) goto rollback;
				free(stmt); stmt = NULL;
			}
			ret = edg_wll_FetchRow(ctx,q, 1, NULL, &suffix_id);
			glite_lbu_FreeStmt(&q);


			/* Store zombie job */

			trio_asprintf(&stmt,"insert into zombie_jobs (jobid, prefix_id, suffix_id)"
					" VALUES ('%|Ss', '%|Ss', '%|Ss')", root, prefix_id, suffix_id);
			glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

			if (edg_wll_ExecSQL(ctx,stmt,&q) < 0) {
				if (edg_wll_Error(ctx, NULL, NULL) == EEXIST) {
					/* job already among zombies */
					/* print warning but continue */
					/* erasing other jobs */
					char *et, *ed, *msg, *job_s;

					edg_wll_Error(ctx, &et, &ed);
					job_s = glite_jobid_unparse(job);
					
					asprintf(&msg,"Warning: erasing job %s that already existed in this LB "
						"(reused jobid or corruped DB) (%s: %s)",job_s,et,ed);
					glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "[%d] %s", getpid(), msg);
					free(et); free(ed); free(msg); free(job_s);
					edg_wll_ResetError(ctx);
				}
				else goto rollback;
			}
			glite_lbu_FreeStmt(&q);
			free(stmt); stmt = NULL;
		}

		if (dump >= 0) 
			trio_asprintf(&stmt,
				"select event,code,prog,host,u.cert_subj,time_stamp,usec,level,arrived,seqcode "
				"from events e,users u "
				"where e.jobid='%|Ss' "
				"and u.userid=e.userid "
				"order by event", dbjob);
		else
			trio_asprintf(&stmt,"select event from events "
				"where jobid='%|Ss' "
				"order by event", dbjob);

		glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

		if (edg_wll_ExecSQL(ctx,stmt,&q) < 0) goto rollback;
		free(stmt); stmt = NULL;

		dumped = 1;
		while ((ret = edg_wll_FetchRow(ctx,q,sizofa(res),NULL,res)) > 0) {
			int	event, ret_dump = 0, i;

			event = atoi(res[0]);

			if (dump >= 0) {
				assert(ret == 10);
				ret_dump = dump_events( ctx, job, dump, (char **) &res);
			}

			for (i=0; i<sizofa(res); i++) free(res[i]);

			if (dump >= 0 && ret_dump) goto rollback;

			if ( purge ) 
				if (edg_wll_delete_event(ctx,dbjob,event)) goto rollback;
		}
		glite_lbu_FreeStmt(&q);
		if (ret < 0) goto rollback;

commit:
rollback:;
	} while (edg_wll_TransNeedRetry(ctx));


err:
	free(root);
	free(suffix);
	free(prefix);
	free(prefix_id);
	free(suffix_id);
	free(dbjob);
	free(stmt);
	glite_lbu_FreeStmt(&q);

	return edg_wll_Error(ctx,NULL,NULL);
}


int unset_proxy_flag(edg_wll_Context ctx, glite_jobid_const_t job)
{
	char	*stmt = NULL;
	char            *dbjob;

	edg_wll_ResetError(ctx);

	dbjob = glite_jobid_getUnique(job);
	trio_asprintf(&stmt,"update jobs set proxy='0' where jobid='%|Ss'", dbjob);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
	free(dbjob);

	return(edg_wll_ExecSQL(ctx,stmt,NULL));
}


int unset_server_flag(edg_wll_Context ctx, glite_jobid_const_t job)
{
	char	*stmt = NULL;
	char            *dbjob;

	edg_wll_ResetError(ctx);

	dbjob = glite_jobid_getUnique(job);
	trio_asprintf(&stmt,"update jobs set server='0' where jobid='%|Ss'", dbjob);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
	free(dbjob);

	return(edg_wll_ExecSQL(ctx,stmt,NULL));
}


int job_exists(edg_wll_Context ctx, glite_jobid_const_t job) {
	char *stmt, *dbjob;
	int retval;

	edg_wll_ResetError(ctx);

	if (check_strict_jobid(ctx, job)) return 0;

	dbjob = glite_jobid_getUnique(job);
	trio_asprintf(&stmt, "SELECT jobid FROM jobs WHERE jobid='%|Ss'", dbjob);
	retval = edg_wll_ExecSQL(ctx, stmt, NULL);
	free(dbjob);
	free(stmt);

	return retval;
}


/* throttle purging according to the required target_runtime */
static void purge_throttle(purge_ctx_t *prg) {
	struct timeval	tp;
	double target_this, now;

	if (prg->time_per_job < 0.0) {
		prg->time_per_job = prg->jobs_to_exa ? (double)(prg->target_runtime) / prg->jobs_to_exa : 0.0;
		//glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_DEBUG, "purge, target runtime: %ld, end: %lf, jobs: %d, s/job: %lf\n",
		//    prg->target_runtime, prg->purge_end, prg->jobs_to_exa, prg->time_per_job);
	}

	if (--prg->jobs_to_exa < 0) prg->jobs_to_exa = 0;

	if (prg->target_runtime) {
		target_this = prg->purge_end - prg->time_per_job * prg->jobs_to_exa;
		gettimeofday(&tp, NULL);
		now = tp.tv_sec + (double)tp.tv_usec / 1000000.0;
		if (target_this > now) {  /* enough time */
			//glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_DEBUG, "purge, sleeping for %lf seconds...\n", target_this - now);
			usleep(1e6*(target_this - now));
		}
		if (target_this < now) {   /* speed up */
			prg->time_per_job = (prg->purge_end-now)/prg->jobs_to_exa;
			/* limit not catched, maximal speed */
			if (prg->time_per_job <= 0) {
				prg->time_per_job = 0.0;
				prg->target_runtime = 0;
			}
		}
		//glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_DEBUG, "purge, s/job: %lf\n", prg->time_per_job);
	}
}

