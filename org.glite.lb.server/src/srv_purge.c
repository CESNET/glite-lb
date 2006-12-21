#ident "$Header$"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "glite/lb-utils/cjobid.h"

#include "glite/lb-utils/db.h"
#include "glite/lb-utils/trio.h"
#include "glite/lb/context-int.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/mini_http.h"
#include "glite/lb/ulm_parse.h"

#include "db_supp.h"
#include "lb_html.h"
#include "lb_proto.h"
#include "store.h"
#include "lock.h"
#include "query.h"
#include "get_events.h"
#include "glite/lb/purge.h"
#include "glite/lb/load.h"
#include "glite/lb/dump.h"
#include "purge.h"
#include "lb_xml_parse.h"


#define DUMP_FILE_STORAGE					"/tmp/"

#define sizofa(a) (sizeof(a)/sizeof((a)[0]))

static const char* const resp_headers[] = {
	"Cache-Control: no-cache",
	"Accept: application/x-dglb",
	"User-Agent: edg_wll_Server/" PROTO_VERSION "/" COMP_PROTO,
	"Content-Type: application/x-dglb",
	NULL
};

static int purge_one(edg_wll_Context ctx,const glite_lbu_JobId,int,int);

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

int edg_wll_PurgeServerProxy(edg_wll_Context ctx, glite_lbu_JobId job)
{
	switch ( purge_one(ctx, job, -1, 1) ) {
	case 0:
	case ENOENT:
		edg_wll_ResetError(ctx);
		return 0;

	default:
		return -1;
	}
}

int edg_wll_PurgeServer(edg_wll_Context ctx,const edg_wll_PurgeRequest *request)
{
	int	i,parse = 0,dumpfile = -1;
	glite_lbu_JobId	job;
	char	*message = NULL, *response = NULL;
	char	*tmpfname = NULL;
	int	naffected_jobs = 0;
	edg_wll_PurgeResult result;
	int		ret = HTTP_OK;


	if (!ctx->noAuth) {
		edg_wll_SetError(ctx,EPERM,"only superusers may purge");
		goto abort;
	}

	edg_wll_ResetError(ctx);
	memset(&result, 0, sizeof(edg_wll_PurgeResult));


	if ( (request->flags & EDG_WLL_PURGE_SERVER_DUMP) && 
		 ((dumpfile = edg_wll_CreateTmpPurgeFile(ctx, &tmpfname)) == -1 ) )
		return edg_wll_Error(ctx, NULL, NULL);

	if (request->flags&EDG_WLL_PURGE_REALLY_PURGE) {
		edg_wll_DumpRequest	req = {
			EDG_WLL_DUMP_LAST_END, EDG_WLL_DUMP_NOW
		};
		edg_wll_DumpResult	res;

		if (edg_wll_DumpEvents(ctx,&req,&res)) 
		{
			if ( request->flags & EDG_WLL_PURGE_SERVER_DUMP )
				unlink(tmpfname);
			return edg_wll_Error(ctx, NULL, NULL);
		}
	}

	if (request->jobs) for (i=0; request->jobs[i]; i++) {
		if (glite_lbu_JobIdParse(request->jobs[i],&job)) {
			fprintf(stderr,"%s: parse error\n",request->jobs[i]);
			parse = 1;
		}
		else {
			if (check_strict_jobid(ctx,job)) {
				fprintf(stderr,"%s: not my job\n",request->jobs[i]);
				parse = 1;
			}
			else {
				switch (purge_one(ctx,job,dumpfile,request->flags&EDG_WLL_PURGE_REALLY_PURGE)) {
					case 0: if (request->flags & EDG_WLL_PURGE_LIST_JOBS) {
							result.jobs = realloc(result.jobs,(naffected_jobs+2) * sizeof(*result.jobs));
							result.jobs[naffected_jobs] = strdup(request->jobs[i]);
							result.jobs[naffected_jobs+1] = NULL;
						}
						naffected_jobs++;
						break;
					case ENOENT: parse = 1;
						     edg_wll_ResetError(ctx);
						     break;
					default: goto abort;
				}

			}
			glite_lbu_JobIdFree(job);
		}
	}
	else {
		glite_lbu_Statement	s;
		char		*job_s;
		int		res;
		time_t		timeout[EDG_WLL_NUMBER_OF_STATCODES],
				now = time(NULL);

		for (i=0; i<EDG_WLL_NUMBER_OF_STATCODES; i++)
			timeout[i] = request->timeout[i] < 0 ? ctx->purge_timeout[i] : request->timeout[i];

		if (glite_lbu_ExecSQL(ctx->dbctx,"select dg_jobid from jobs",&s) < 0) {
			edg_wll_SetErrorDB(ctx);
			goto abort;
		}
		while ((res = glite_lbu_FetchRow(s,1,NULL,&job_s)) > 0) {
			if (glite_lbu_JobIdParse(job_s,&job)) {
				fprintf(stderr,"%s: parse error (internal inconsistency !)\n",job_s);
				parse = 1;
			}
			else {
				edg_wll_JobStat	stat;

				if (check_strict_jobid(ctx,job)) {
					glite_lbu_JobIdFree(job);
					free(job_s);
					parse = 1;
					continue;
				}

				memset(&stat,0,sizeof stat);
				if (edg_wll_JobStatus(ctx,job,0,&stat)) goto abort; /* XXX: memory leak */

				switch (stat.state) {
					case EDG_WLL_JOB_CLEARED:
					case EDG_WLL_JOB_ABORTED:
					case EDG_WLL_JOB_CANCELLED:
						i = stat.state;
						break;
					default:
						i = EDG_WLL_PURGE_JOBSTAT_OTHER;
				}

				if (now-stat.lastUpdateTime.tv_sec > timeout[i] && !check_strict_jobid(ctx,job))
				{
					if (purge_one(ctx,job,dumpfile,request->flags&EDG_WLL_PURGE_REALLY_PURGE))
						goto abort;

				/* XXX: change with the streaming interface */
					if (request->flags & EDG_WLL_PURGE_LIST_JOBS) {
						result.jobs = realloc(result.jobs,(naffected_jobs+2) * sizeof(*result.jobs));
						result.jobs[naffected_jobs] = job_s;
						result.jobs[naffected_jobs+1] = NULL;
						job_s = NULL;
					}
					naffected_jobs++;
				}

				glite_lbu_JobIdFree(job);
				edg_wll_FreeStatus(&stat);
				free(job_s);
			}
		}
abort:
		glite_lbu_FreeStmt(&s);
	}

	if (parse && !edg_wll_Error(ctx,NULL,NULL))
	{
		if ( naffected_jobs )
			edg_wll_SetError(ctx,EINVAL,"Invalid JobId(s) but other jobs purged");
		else
			edg_wll_SetError(ctx,EINVAL,"Invalid JobId(s)");
	}

	switch ( edg_wll_Error(ctx,NULL,NULL) )
	{
		case 0:
			ret = HTTP_OK;
			break;
		case EINVAL:
			ret = HTTP_INVALID;
			break;
		case EPERM:
			ret = HTTP_UNAUTH;
			break;
		
		/* fatal errors */
		case ENOMEM:
			/* fall through */
		default:
			ret = HTTP_INTERNAL;
			break;
	}
	
	if (ret != HTTP_INTERNAL) {
		if ( request->flags & EDG_WLL_PURGE_SERVER_DUMP && tmpfname )
		{
			edg_wll_CreatePurgeFileFromTmp(ctx, tmpfname, &(result.server_file));
			unlink(tmpfname);
		}

		if ( edg_wll_PurgeResultToXML(ctx, &result, &message) )
			ret = HTTP_INTERNAL;
		else 
			printf("%s", message);
	}
	
	if ( result.server_file )
		free(result.server_file);
	if ( result.jobs )
	{
		for ( i = 0; result.jobs[i]; i++ )
			free(result.jobs[i]);
		free(result.jobs);
	}

	asprintf(&response, "HTTP/1.1 %d %s", ret, edg_wll_HTTPErrorMessage(ret));

	edg_wll_http_send(ctx, response, resp_headers, message,ctx->connections->serverConnection);

	return edg_wll_Error(ctx,NULL,NULL);
}

static void unlock_and_check(edg_wll_Context ctx,glite_lbu_JobId job)
{
	char	*job_s,*et,*ed;

	if (edg_wll_UnlockJob(ctx,job)) {
		job_s = glite_lbu_JobIdUnparse(job);

		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"%s: edg_wll_UnlockJob(): %s (%s) -- expect bogus things\n",
				job_s,et,ed);
		syslog(LOG_CRIT,"%s: edg_wll_UnlockJob(): %s (%s) -- expect bogus things",
				job_s,et,ed);
		free(et); free(ed); free(job_s);
	}
}


int purge_one(edg_wll_Context ctx,const glite_lbu_JobId job,int dump, int purge)
{
	char	*dbjob;
	char	*stmt = NULL;
	glite_lbu_Statement	q;
	int		ret,dumped = 0;

	edg_wll_ResetError(ctx);
	if ( !purge && dump < 0 ) return 0;

	dbjob = glite_lbu_JobIdGetUnique(job);	/* XXX: strict jobid already checked */
	if (edg_wll_LockJob(ctx,job)) goto clean;

	if ( purge )
	{
		trio_asprintf(&stmt,"delete from jobs where jobid = '%|Ss'",dbjob);
		ret = glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL);
		if (ret <= 0) {
			unlock_and_check(ctx,job);
			if (ret == 0) {
				fprintf(stderr,"%s: no such job\n",dbjob);
				edg_wll_SetError(ctx,ENOENT,dbjob);
			}
			goto clean;
		}
		free(stmt); stmt = NULL;

		trio_asprintf(&stmt,"delete from states where jobid = '%|Ss'",dbjob);
		if (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL) < 0) {
			unlock_and_check(ctx,job);
			goto clean;
		}
		free(stmt); stmt = NULL;

/* Why on earth ?
		trio_asprintf(&stmt,"delete from states where jobid = '%|Ss'",dbjob);
		if (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL) < 0) {
			unlock_and_check(ctx,job);
			goto clean;
		}
		free(stmt); stmt = NULL;
*/

	}

	if (!ctx->strict_locking) unlock_and_check(ctx,job);

	if ( purge )
	{
		trio_asprintf(&stmt,"delete from status_tags where jobid = '%|Ss'",dbjob);
		if (glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL) < 0) goto unlock;
		free(stmt); stmt = NULL;
	}

	if (dump >= 0) 
		trio_asprintf(&stmt,
			"select event,code,prog,host,u.cert_subj,time_stamp,usec,level,arrived "
			"from events e,users u "
			"where e.jobid='%|Ss' "
			"and u.userid=e.userid "
			"order by event", dbjob);
	else
		trio_asprintf(&stmt,"select event from events "
			"where jobid='%|Ss' "
			"order by event", dbjob);

/* check for events repeatedly -- new one may have arrived in the meantime */
	while ((ret = glite_lbu_ExecSQL(ctx->dbctx,stmt,&q)) > 0) {
		char	*res[9];

		dumped = 1;
		while ((ret = glite_lbu_FetchRow(q,dump >= 0 ? 9 : 1,NULL,res)) > 0) {
			int	event;

			event = atoi(res[0]);
			free(res[0]); res[0] = NULL;

			if (dump >= 0) {
				edg_wll_Event	e;

				assert(ret == 9);
				res[0] = glite_lbu_JobIdUnparse(job);
				if (convert_event_head(ctx,res,&e) || edg_wll_get_event_flesh(ctx,event,&e))
				{
					char	*et,*ed;
					int	i;

				/* Most likely sort of internal inconsistency. 
				 * Must not be fatal -- just complain
				 */
					edg_wll_Error(ctx,&et,&ed);
					fprintf(stderr,"%s event %d: %s (%s)\n",dbjob,event,et,ed);
					syslog(LOG_WARNING,"%s event %d: %s (%s)",dbjob,event,et,ed);
					free(et); free(ed);
					for (i=0; i<sizofa(res); i++) free(res[i]);
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
							glite_lbu_FreeStmt(&q);
							free(event_s);
							goto clean;
						}
						total += written;
					}
					write(dump, " ", 1);
					
					len = strlen(event_s);
					total = 0;
					while (total != len) {
						written = write(dump,event_s+total,len-total);
						if (written < 0 && errno != EAGAIN) {
							perror("dump to file");
							syslog(LOG_ERR,"dump to file: %m");
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
			}

			if ( purge ) {
				if (edg_wll_delete_event(ctx,dbjob,event)) {
					char	*et,*ed;

				/* XXX: just complain and carry on. Is it OK? */
					edg_wll_Error(ctx,&et,&ed);
					fprintf(stderr,"%s event %d: %s (%s)\n",dbjob,event,et,ed);
					syslog(LOG_WARNING,"%s event %d: %s (%s)",dbjob,event,et,ed);
					free(et); free(ed);
					edg_wll_ResetError(ctx);
				}
			}
		}
		glite_lbu_FreeStmt(&q);
		if (ret < 0 || !purge) {
			edg_wll_SetErrorDB(ctx);
			break;
		}
	}

	if (ret == 0 && dumped == 0) {
		if (ctx->strict_locking) unlock_and_check(ctx,job);
		fprintf(stderr,"%s: no events, i.e. no such job or internal inconsistency\n",dbjob);
		edg_wll_SetError(ctx,ENOENT,dbjob);
		goto clean;
	}
	if (ret < 0) edg_wll_SetErrorDB(ctx);

unlock:
	if (ctx->strict_locking) unlock_and_check(ctx,job);

clean:
	free(dbjob);
	free(stmt);
	return edg_wll_Error(ctx,NULL,NULL);
}
