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
	char	*from_s, *to_s, *stmt, *time_s, *ptr;
	char	*tmpfname;
	time_t	start,end;
	glite_lbu_Statement	q = NULL;
	char		*res[11];
	int	event;
	edg_wll_Event	e;
	int	ret,dump = 2;	/* TODO: manage dump file */
	time_t	from = req->from,to = req->to;

	from_s = to_s = stmt = NULL;
	memset(res,0,sizeof res);
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
			"order by arrived",
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
			char	*event_s = edg_wll_UnparseEvent(ctx,&e);
			char	arr_s[100];
			int		len, written, total;

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
					edg_wll_SetError(ctx,errno,"writing dump file");
					free(event_s);
					goto clean;
				}
				total += written;
			}
			write(dump,"\n",1);
			free(event_s);
		}
		edg_wll_FreeEvent(&e); memset(&e,0,sizeof e);
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

	free(stmt);
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
