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


#include <time.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>

#include "glite/lbu/trio.h"
#include "glite/jobid/cjobid.h"
#include  "glite/lbu/log.h"

#include "glite/lb/context-int.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/ulm_parse.h"
#include "glite/lb/intjobstat.h"
#include "glite/lb/intjobstat_supp.h"

#include "query.h"
#include "get_events.h"
#include "server_state.h"
#include "purge.h"
#include "db_supp.h"
#include "lb_proto.h"

static char *time_to_string(time_t t, char **ptr);
static int handle_specials(edg_wll_Context,time_t *);

#define sizofa(a) (sizeof(a)/sizeof((a)[0]))

int edg_wll_DumpEventsServer(edg_wll_Context ctx,const edg_wll_DumpRequest *req,edg_wll_DumpResult *result)
{
	char	*from_s, *to_s, *stmt, *stmt2, *time_s, *ptr;
	char	*tmpfname;
	time_t	start,end;
	glite_lbu_Statement	q = NULL;
	glite_lbu_Statement	q2 = NULL;
	char		*res[11];
	char		*res2[6];
	int	event;
	edg_wll_Event	e;
	edg_wll_Event	*f;
	int	ret,dump = 2;	/* TODO: manage dump file */
	time_t	from = req->from,to = req->to;
	char *event_s, *dumpline;
	int		len, written, total;
	intJobStat *stat;

	from_s = to_s = stmt = stmt2 = NULL;
	memset(res,0,sizeof res);
	memset(res2,0,sizeof res2);
	memset(&e,0,sizeof e);

	time(&start);
	edg_wll_ResetError(ctx);

	if ( (dump = edg_wll_CreateTmpDumpFile(ctx, &tmpfname)) == -1 )
		return edg_wll_Error(ctx, NULL, NULL);

	if (handle_specials(ctx,&from) || handle_specials(ctx,&to))
	{
		unlink(tmpfname);
		return edg_wll_Error(ctx,NULL,NULL);
	}

	glite_lbu_TimeToStr(from, &from_s);
	glite_lbu_TimeToStr(to, &to_s);

	trio_asprintf(&stmt,
			"select event,dg_jobid,code,prog,host,u.cert_subj,time_stamp,usec,level,arrived,seqcode "
			"from events e,users u,jobs j "
			"where u.userid=e.userid "
			"and j.jobid = e.jobid "
			"and j.dg_jobid like 'https://%|Ss:%d/%%' "
			"and arrived > %s and arrived <= %s "
			"order by arrived,event",
			ctx->srvName,ctx->srvPort,
			from_s,to_s);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
	
	if (edg_wll_ExecSQL(ctx,stmt,&q) < 0) goto clean;

	while ((ret = edg_wll_FetchRow(ctx,q,sizeof(res)/sizeof(res[0]),NULL,res)) > 0) {
		assert(ret == sizofa(res));
		event = atoi(res[0]); free(res[0]); res[0] = NULL;

		if (convert_event_head(ctx,res+1,&e)
			|| edg_wll_get_event_flesh(ctx,event,&e))
		{
			char	*et,*ed;
			int	i;

		/* Most likely sort of internal inconsistency. 
		 * Must not be fatal -- just complain
		 */
			edg_wll_Error(ctx,&et,&ed);
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "%s event %d: %s (%s)", res[1], event, et, ed);
			free(et); free(ed);
			for (i=0; i<sizofa(res); i++) free(res[i]);
			edg_wll_ResetError(ctx);
		}
		else {
			event_s = edg_wll_UnparseEvent(ctx,&e);
			char	arr_s[100];

		        edg_wll_ULMTimevalToDate(e.any.arrived.tv_sec, e.any.arrived.tv_usec, arr_s);
			asprintf(&dumpline, "DG.ARRIVED=%s %s\n", arr_s, event_s);
			len = strlen(dumpline); 
			total = 0;
			while (total != len) {
				written = write(dump,dumpline+total,len-total);
				if (written < 0 && errno != EAGAIN) {
					edg_wll_SetError(ctx,errno,"writing dump file");
					break;
				}
				total += written;
			}
			free(event_s);
		}
		edg_wll_FreeEvent(&e); memset(&e,0,sizeof e);
	}

	// Take care of implicit subjob registration events
        trio_asprintf(&stmt2,
                        "select s.jobid,s.parent_job,ef.ulm,j.dg_jobid,s.int_status,e.arrived from states s, events_flesh ef, events e,jobs j "
                        "where s.parent_job<>'*no parent job*' AND "
			"ef.jobid=s.parent_job AND (e.code=%d OR e.code=%d) AND "
			"e.jobid=ef.jobid AND e.event=ef.event AND "
			"e.arrived > %s AND e.arrived <= %s AND "
			"j.jobid=s.jobid",
			EDG_WLL_EVENT_REGJOB, EDG_WLL_EVENT_FILETRANSFERREGISTER,
			from_s,to_s);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt2);
	if (edg_wll_ExecSQL(ctx,stmt2,&q2) < 0) goto clean;

	while ((ret = edg_wll_FetchRow(ctx,q2,sizeof(res2)/sizeof(res2[0]),NULL,res2)) > 0) {
		glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG, "Dumping subjob %s, parent %s", res2[0], res2[1]);

		edg_wll_ParseEvent(ctx,res2[2],&f);

		f->regJob.nsubjobs = 0;
		f->regJob.parent = f->any.jobId;

		f->any.jobId=NULL;
		edg_wlc_JobIdParse(res2[3], &f->any.jobId);

	        f->any.arrived.tv_sec = glite_lbu_StrToTime(res2[5]);
	        f->any.arrived.tv_usec = 0;

		char *rest;
		stat = dec_intJobStat(res2[4], &rest);
		//nasty but not the only similar solution in code
		switch (stat->pub.jobtype) {
			case EDG_WLL_STAT_SIMPLE:
				f->regJob.jobtype = EDG_WLL_REGJOB_SIMPLE; break;
			case EDG_WLL_STAT_FILE_TRANSFER:
				f->regJob.jobtype = EDG_WLL_REGJOB_FILE_TRANSFER; break;
			default:
				f->regJob.jobtype = EDG_WLL_REGJOB_JOBTYPE_UNDEFINED;
				glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_WARN, "Job %s has type %d but it also lists a parent job %s", res2[2], stat->pub.jobtype, res2[1]);
		}

		char	arr_s[100];
		event_s = edg_wll_UnparseEvent(ctx,f);
		edg_wll_ULMTimevalToDate(f->any.arrived.tv_sec, f->any.arrived.tv_usec, arr_s);
		asprintf(&dumpline, "DG.ARRIVED=%s %s\n", arr_s, event_s);

		len = strlen(dumpline); 
		total = 0;
		while (total != len) {
			written = write(dump,dumpline+total,len-total);
			if (written < 0 && errno != EAGAIN) {
				edg_wll_SetError(ctx,errno,"writing dump file");
				break;
			}
			total += written;
		}
		edg_wll_FreeStatus(intJobStat_to_JobStat(stat));
		free(event_s);
		free(dumpline);
		edg_wll_FreeEvent(f);
		if (total != len) goto clean; 
	}

	time(&end);
	time_s = time_to_string(start, &ptr);
	edg_wll_SetServerState(ctx,EDG_WLL_STATE_DUMP_START,time_s);
	free(ptr);
	
	time_s = time_to_string(end, &ptr);
	edg_wll_SetServerState(ctx,EDG_WLL_STATE_DUMP_END,time_s);
	free(ptr);

	result->from = from;
	result->to = to;

	edg_wll_CreateDumpFileFromTmp(ctx, tmpfname, &(result->server_file));
	unlink(tmpfname);

clean:
	edg_wll_FreeEvent(&e);
	glite_lbu_FreeStmt(&q);
	glite_lbu_FreeStmt(&q2);

	free(stmt);
	free(stmt2);
	free(from_s);
	free(to_s);
	return edg_wll_Error(ctx,NULL,NULL);
}

static int handle_specials(edg_wll_Context ctx,time_t *t)
{
	char	*time_s;
	int	ret;

	edg_wll_ResetError(ctx);
	switch (*t) {
		case EDG_WLL_DUMP_NOW:
			time(t);
			return 0;
		case EDG_WLL_DUMP_LAST_START:
		case EDG_WLL_DUMP_LAST_END:
			switch (ret = edg_wll_GetServerState(ctx,
					*t == EDG_WLL_DUMP_LAST_START ? 
						EDG_WLL_STATE_DUMP_START:
						EDG_WLL_STATE_DUMP_END,
					&time_s))
			{
				case ENOENT: *t = 0; 
					     edg_wll_ResetError(ctx);
					     break;
				case 0: *t = glite_lbu_StrToTime(time_s); 
					assert(*t >= 0);
					break;
				default: break;
			}
			break;
		default: if (*t < 0) return edg_wll_SetError(ctx,EINVAL,"special time limit unrecognized");
	}
	return edg_wll_Error(ctx,NULL,NULL);
}


static char *time_to_string(time_t t, char **ptr) {
	char *s;

	glite_lbu_TimeToStr(t, &s);
	s[strlen(s) - 1] = '\0';
	*ptr = s;

	return s + 1;
}
