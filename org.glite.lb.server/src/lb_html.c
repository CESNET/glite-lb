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


#include "lb_html.h"
#include "lb_proto.h"
#include "cond_dump.h"
#include "pretty_print_wrapper.h"
#include "server_stats.h"
#include "authz_policy.h"

#include "glite/lb/context-int.h"
#include "glite/lb/xml_conversions.h"

#include "glite/lbu/log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <errno.h>

#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif

int edg_wll_QueryToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wll_Event *eventsOut UNUSED_VAR, char **message UNUSED_VAR)
{
/* not implemented yet */
	return -1;
}

/* construct Message-Body of Response-Line for edg_wll_UserJobs */
int edg_wll_UserInfoToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wlc_JobId *jobsOut, edg_wll_JobStat *statsOut, char **message)
{
	//TODO remove quadratic complexity one day...

        char *pomA = NULL, *pomB;
        int i = 0, j;

        /* head */
        pomB = strdup("");

        while (jobsOut && jobsOut[i]) {
                char    *chid = edg_wlc_JobIdUnparse(jobsOut[i]);

		if (! statsOut){
			 asprintf(&pomA,"%s\t\t <li><a href=\"%s\">%s</a></li>\r\n",
                                pomB, chid,chid);
                        free(pomB);
                        pomB = pomA;
		}
		else if ((statsOut[i].jobtype != EDG_WLL_STAT_FILE_TRANSFER_COLLECTION 
			&& statsOut[i].jobtype != EDG_WLL_STAT_FILE_TRANSFER 
			&& ! statsOut[i].parent_job )
			|| ((statsOut[i].jobtype == EDG_WLL_STAT_FILE_TRANSFER_COLLECTION 
			|| statsOut[i].jobtype == EDG_WLL_STAT_FILE_TRANSFER) 
			&& ! statsOut[i].ft_compute_job)
			){
			asprintf(&pomA,"%s\t\t <li><a href=\"%s\">%s</a></li>\r\n",
        	        	pomB, chid,chid);
	                free(pomB);
        	        pomB = pomA;
			if (statsOut[i].jobtype == EDG_WLL_STAT_COLLECTION){
				asprintf(&pomA,"%s\t\t <ul>\r\n", pomB);
                                free(pomB);
                                pomB = pomA;
				j = 0;
				while (jobsOut[j]) {
					char *chid_parent = edg_wlc_JobIdUnparse(statsOut[j].parent_job);
					if (chid_parent && (strcmp(chid, chid_parent) == 0)){
						char *chid_children = edg_wlc_JobIdUnparse(jobsOut[j]);
						asprintf(&pomA,"%s\t\t <li><a href=\"%s\">%s</a></li>\r\n",
			                               	pomB, chid_children,chid_children);
						free(chid_children);
			                        free(pomB);
                        			pomB = pomA;
					}
					free(chid_parent);
					j++;
				}
				asprintf(&pomA,"%s\t\t </ul>\r\n", pomB);
                                free(pomB);
                                pomB = pomA;
			}
			free(chid);
		}

                i++;
        }

	char *ret;
	asprintf(&ret, "<html>\r\n\t<body>\r\n");
	pomA = ret;
	if (pomB[0]){
		asprintf(&ret, "%s<h2><B>User jobs</B></h2>\r\n"
			"<ul>%s</ul>",
			pomA, pomB
		);
		free(pomA);
		free(pomB);
	}
	pomA = ret;
	asprintf(&ret, "%sUser subject: %s<p>"
		"\t</body>\r\n</html>",
		pomA, ctx->peerName?ctx->peerName: "&lt;anonymous&gt;"
	);
	free(pomA);

        *message = ret;

        return 0;
}

int edg_wll_UserNotifsToHTML(edg_wll_Context ctx UNUSED_VAR, char **notifids, char **message){
	char *pomA = NULL, *pomB = NULL;
        pomB = strdup("");

	int i = 0;
        while(notifids && notifids[i]){
                asprintf(&pomA, "%s\t\t <li> <a href=\"/NOTIF:%s\">%s</a>\r\n",
                                pomB,
                                notifids[i],
                                notifids[i]
                        );
                free(pomB);
                pomB = pomA;
                i++;
        }

	char *ret;
        asprintf(&ret, "<html>\r\n\t<body>\r\n");
	asprintf(&ret, "<html>\r\n\t<body>\r\n"
			"<h2><B>User notifications</B></h2>\r\n"
                        "<ul>%s</ul>"
			"\t</body>\r\n</html>",
                        pomA
        );
        free(pomA);

	*message = ret;

	return 0;
}

#define TR(name,type,field,null) \
{ \
	int l; \
	if ((field) != (null)){ \
		l = asprintf(&pomA,"<tr><th align=\"left\">" name ":</th>" \
			"<td>" type "</td></tr>", (field)); \
	} \
	else{ \
		l = asprintf(&pomA,"<tr><th align=\"left\"><span style=\"color:grey\">" name \
			"</span></th></tr>"); \
	} \
	pomB = realloc(pomB, sizeof(*pomB)*(pomL+l+1)); \
	strcpy(pomB+pomL, pomA); \
	pomL += l; \
	free(pomA); \
	pomA=NULL; \
}

#define TRL(name,type,field,null) \
{ \
        int l; \
        if ((field) != (null)){ \
                l = asprintf(&pomA,"<tr><th align=\"left\">" name ":</th>" \
                        "<td><a href=\""type"\">" type "</a></td></tr>", (field), (field)); \
        } \
        else{ \
                l = asprintf(&pomA,"<tr><th align=\"left\"><span style=\"color:grey\">" name \
                        "</span></th></tr>"); \
        } \
        pomB = realloc(pomB, sizeof(*pomB)*(pomL+l+1)); \
        strcpy(pomB+pomL, pomA); \
        pomL += l; \
        free(pomA); \
	pomA=NULL; \
}

int edg_wll_NotificationToHTML(edg_wll_Context ctx UNUSED_VAR, notifInfo *ni, char **message){
	char *pomA = NULL, *pomB = NULL, *flags = NULL, *cond;
	int pomL = 0;

	flags = edg_wll_stat_flags_to_string(ni->flags);
	printf("flags %d - %s", ni->flags, flags);

	TR("Destination", "%s", ni->destination, NULL);
	TR("Valid until", "%s", ni->valid, NULL);
	TR("Flags", "%s", flags, NULL);
        free(flags);

	if (! edg_wll_Condition_Dump(ni, &cond, 0)){
		asprintf(&pomA, "%s<h3>Conditions</h3>\r\n<pre>%s</pre>\r\n",
        	        pomB, cond);
	        free(pomB);
        	pomB = pomA;
		pomL = strlen(pomB);

	}
	free(cond);

	asprintf(&pomA, "<html>\r\n\t<body>\r\n"
		"<h2>Notification %s</h2>\r\n"
		"<table halign=\"left\">%s</table>"
		"\t</body>\r\n</html>",
		ni->notifid, pomB);
	
	*message = pomA;

	return 0;
}

/* construct Message-Body of Response-Line for edg_wll_JobStatus */
int edg_wll_GeneralJobStatusToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wll_JobStat stat, char **message)
{
        char *pomA = NULL, *pomB = NULL;
	int pomL = 0;
	char	*chid,*chstat,*chis = NULL, *chos = NULL, *chpa = NULL;
	char	*jdl,*rsl,*children;
	int	i;

	jdl = strdup("");
	rsl = strdup("");
	children = strdup("");
	
        chid = edg_wlc_JobIdUnparse(stat.jobId);
	if (stat.isb_transfer) chis = edg_wlc_JobIdUnparse(stat.isb_transfer);
	if (stat.osb_transfer) chos = edg_wlc_JobIdUnparse(stat.osb_transfer);
	if (stat.parent_job) chpa = edg_wlc_JobIdUnparse(stat.parent_job);

	TRL("Parent job", "%s", chpa, NULL);
	TR("Status","%s",(chstat = edg_wll_StatToString(stat.state)), NULL);
	free(chstat);
	TR("Owner","%s",stat.owner, NULL);
	TR("Payload Owner","%s",stat.payload_owner, NULL);
	TR("Condor Id","%s",stat.condorId, NULL);
	TR("Globus Id","%s",stat.globusId, NULL);
	TR("Local Id","%s",stat.localId, NULL);
	TR("Reason","%s",stat.reason, NULL);
	if ( (stat.stateEnterTime.tv_sec) || (stat.stateEnterTime.tv_usec) ) {
		time_t  time = stat.stateEnterTime.tv_sec;
		TR("State entered","%s",ctime(&time), NULL);
	}
	else
		TR("State entered", "%s", (char*)NULL, NULL);
        if ( (stat.lastUpdateTime.tv_sec) || (stat.lastUpdateTime.tv_usec) ) {
		time_t  time = stat.lastUpdateTime.tv_sec;
		TR("Last update","%s",ctime(&time), NULL);
	}
	else
		TR("Last update", "%s", (char*)NULL, NULL);
	TR("Expect update","%s",stat.expectUpdate ? "YES" : "NO", NULL);
	TR("Expect update from","%s",stat.expectFrom, NULL);
	TR("Location","%s",stat.location, NULL);
	TR("Destination","%s",stat.destination, NULL);
	TR("Cancelling","%s",stat.cancelling>0 ? "YES" : "NO", NULL);
	TR("Cancel reason","%s",stat.cancelReason, NULL);
	TR("CPU time","%d",stat.cpuTime, 0);

	
	TR("Done code","%d",stat.done_code, -1);
	TR("Exit code","%d",stat.exit_code, -1);

	TRL("Input sandbox", "%s", chis, NULL);
	TRL("Output sandbox", "%s", chos, NULL);

	if (stat.jdl){
		char *jdl_unp;
		if (pretty_print(stat.jdl, &jdl_unp) == 0)
			asprintf(&jdl,"<h3>Job description</h3>\r\n"
                                "<pre>%s</pre>\r\n",jdl_unp);
		else
			asprintf(&jdl,"<h3>Job description (not a ClassAd)"
				"</h3>\r\n<pre>%s</pre>\r\n",stat.jdl);
	}

	if (stat.rsl) asprintf(&rsl,"<h3>RSL</h3>\r\n"
		"<pre>%s</pre>\r\n",stat.rsl);

	if ((stat.jobtype == EDG_WLL_STAT_COLLECTION) && (stat.children_num > 0)){
		asprintf(&children, "<h3>Children</h3>\r\n");
		for (i = 0; i < stat.children_num; i++){
			asprintf(&pomA,"%s\t\t <li/> <a href=\"%s\">%s</a>\r\n",
                        children, stat.children[i], stat.children[i]);
			children = pomA;
		}
	}

        asprintf(&pomA, "<html>\r\n\t<body>\r\n"
			"<h2>%s</h2>\r\n"
			"<table halign=\"left\">%s</table>"
			"%s%s%s"
			"\t</body>\r\n</html>",
                	chid,pomB,jdl,rsl, children);
        free(pomB);

        *message = pomA;

	free(chid);
	if (chis) free(chis);
	if (chos) free(chos);
	if (chpa) free(chpa);
	free(jdl);
	free(rsl);
	free(children);
        return 0;
}

int edg_wll_CreamJobStatusToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wll_JobStat stat, char **message)
{
	char *chid, *pomA = NULL, *pomB = NULL, *jdl;
	char *lbstat, *creamstat;
	int pomL = 0;

	jdl = strdup("");

	chid = edg_wlc_JobIdUnparse(stat.jobId);

	TR("CREAM ID", "%s", stat.cream_id, NULL);
	TR("Status", "%s", (lbstat = edg_wll_StatToString(stat.state)), NULL);
	free(lbstat);
	TR("CREAM Status", "%s", (creamstat = edg_wll_CreamStatToString(stat.cream_state)), NULL);
	free(creamstat);
	TR("Owner", "%s", stat.cream_owner, NULL);
	TR("Endpoint", "%s", stat.cream_endpoint, NULL);
	TR("Worker node", "%s", stat.cream_node, NULL);
	TR("Reason", "%s", stat.cream_reason, NULL);
	TR("Failure reason", "%s", stat.cream_failure_reason, NULL);
	
	if ( (stat.stateEnterTime.tv_sec) || (stat.stateEnterTime.tv_usec) ) {
                time_t  time = stat.stateEnterTime.tv_sec;
                TR("State entered","%s",ctime(&time), NULL);
        }
        else
                TR("State entered", "%s", (char*)NULL, NULL);
        if ( (stat.lastUpdateTime.tv_sec) || (stat.lastUpdateTime.tv_usec) ) {
                time_t  time = stat.lastUpdateTime.tv_sec;
                TR("Last update","%s",ctime(&time), NULL);
        }
        else
                TR("Last update", "%s", (char*)NULL, NULL);


	TR("LRMS id", "%s", stat.cream_lrms_id, NULL);
	TR("Node", "%s", stat.cream_node, NULL);
	TR("Cancelling", "%s", stat.cream_cancelling > 0 ? "YES" : "NO", NULL);
	TR("CPU time", "%d", stat.cream_cpu_time, 0);
	TR("Done code", "%d", stat.cream_done_code, -1);
        TR("Exit code", "%d", stat.cream_exit_code, -1);

	/*
                cream_jw_status
        */
	
	if (stat.cream_jdl){
                char *jdl_unp;
                if (pretty_print(stat.cream_jdl, &jdl_unp) == 0)
                        asprintf(&jdl,"<h3>Job description</h3>\r\n"
                                "<pre>%s</pre>\r\n",jdl_unp);
                else
                        asprintf(&jdl,"<h3>Job description (not a ClassAd)"
                                "</h3>\r\n<pre>%s</pre>\r\n",stat.cream_jdl);
        }
	 asprintf(&pomA, "<html>\r\n\t<body>\r\n"
                        "<h2>%s</h2>\r\n"
                        "<table halign=\"left\">%s</table>"
                        "%s"
                        "\t</body>\r\n</html>",
                        chid,pomB,jdl);
        free(pomB); free(jdl);

        *message = pomA;

	return 0;
}

int edg_wll_FileTransferStatusToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wll_JobStat stat, char **message)
{
        char *pomA = NULL, *pomB = NULL, *lbstat, *children;
        int pomL = 0, i;
        char    *chid,*chcj,*chpar,*chsbt=NULL;

	children = strdup("");

        chid = edg_wlc_JobIdUnparse(stat.jobId);

	TR("Status", "%s", (lbstat = edg_wll_StatToString(stat.state)), NULL);
	free(lbstat);
        TR("Owner","%s",stat.owner, NULL);
	chcj = edg_wlc_JobIdUnparse(stat.ft_compute_job);
	TRL("Compute job", "%s", chcj, NULL);
	free(chcj);

	if (stat.jobtype == EDG_WLL_STAT_FILE_TRANSFER){
		chpar = edg_wlc_JobIdUnparse(stat.parent_job);
	        TRL("Parent job", "%s", chpar, NULL);
        	free(chpar);
		switch(stat.ft_sandbox_type){
			case EDG_WLL_STAT_INPUT: chsbt = strdup("INPUT");
				break;
			case EDG_WLL_STAT_OUTPUT: chsbt = strdup("OUTPUT");
				break;
			default:
				break;
		}
		TR("Sandbox type", "%s", chsbt, NULL);
		TR("File transfer source", "%s", stat.ft_src, NULL);
		TR("File transfer destination", "%s", stat.ft_dest, NULL);
	}
	else if ((stat.jobtype == EDG_WLL_STAT_FILE_TRANSFER_COLLECTION) && (stat.children_num > 0)){
		asprintf(&children, "<h3>Children</h3>\r\n");
                for (i = 0; i < stat.children_num; i++){
                        asprintf(&pomA,"%s\t\t <li/> <a href=\"%s\">%s</a>\r\n",
                        children, stat.children[i], stat.children[i]);
                        children = pomA;
                }
	}

        asprintf(&pomA, "<html>\r\n\t<body>\r\n"
                        "<h2>%s</h2>\r\n"
                        "<table halign=\"left\">%s</table>\r\n"
			"%s\n"
                        "\t</body>\r\n</html>",
                        chid, pomB, children);
        free(pomB);

        *message = pomA;

	if (chsbt) free(chsbt);
        free(chid);
	free(children);
        return 0;
}

// replace s1 to s2 in text
static int replace_substr(char **text, char *s1, char *s2){
	int l1, l2;
	char *out;

	char *p = strstr(*text, s1);
	if (! p) return -1; // exit if s1 not found

	l1 = strlen(s1);
	l2 = strlen(s2);
	out = (char*)malloc((strlen(*text)+l2-l1+1)*sizeof(char));
	
	strncpy(out, *text, p-*text);
	out[p-*text] = 0;
	sprintf(out+(p-*text), "%s%s", s2, p+l1);

	free(*text);
	*text = out;
}

int edg_wll_WSDLOutput(edg_wll_Context ctx UNUSED_VAR, char **message, char *filename){
	FILE *f;
	char *wsdl, *p, *start;
	int i, b;

	if ((f = fopen(filename, "r")) == NULL){
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, "%s not found!\n", filename);
		edg_wll_SetError(ctx, ENOENT, "WSDL File not found!");
		return -1;
	}

	// load wsdl
	wsdl = (char*)malloc(4096*sizeof(char));
	i = 1;
	while ((b = fread(wsdl+(i-1)*4096, sizeof(char), 4096, f)) == 4096){
		i++;
		wsdl = (char*)realloc(wsdl, 4096*i*sizeof(char));
	}
	wsdl[(i-1)*4096+b] = 0;

	// fix imports
	char *addr;
	asprintf(&addr, "https://%s:%i/?types", ctx->srvName, ctx->srvPort);
	replace_substr(&wsdl, "LBTypes.wsdl", addr);
	free(addr);
	asprintf(&addr, "https://%s:%i/?agu", ctx->srvName, ctx->srvPort);
	replace_substr(&wsdl, "lb4agu.wsdl", addr);
	free(addr);

	*message = wsdl;
	
	return 0;
}

int edg_wll_StatisticsToHTML(edg_wll_Context ctx, char **message) {
        char *out;

        struct _edg_wll_GssPrincipal_data princ;
        memset(&princ, 0, sizeof princ);
        princ.name = ctx->peerName;
        princ.fqans = ctx->fqans;
        if (ctx->count_server_stats == 2 && !ctx->noAuth && !check_authz_policy(&ctx->authz_policy, &princ, ADMIN_ACCESS))  
        {
                asprintf(&out,"<h2>LB Server Usage Statistics</h2>\n"
                        "Only superusers can view server usage statistics on this particular server.\n");
        }
        else
        {
                char* times[SERVER_STATISTICS_COUNT];
                int i;
                for (i = 0; i < SERVER_STATISTICS_COUNT; i++)
                        if (edg_wll_ServerStatisticsGetStart(ctx, i))
                                times[i] = strdup((const char*)ctime(edg_wll_ServerStatisticsGetStart(ctx, i)));
                        else
                                times[i] = 0;

                asprintf(&out,
                        "<h2>LB Server Usage Statistics</h2>\n"
                        "<table halign=\"left\">\n"
                        "<tr><td>Variable</td><td>Value</td><td>Measured from</td></tr>\n"
                        "<tr><td>gLite job regs</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>PBS job regs</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>Condor job regs</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>CREAM job regs</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>Sandbox regs</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>Notification regs (legacy interface)</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>Notification regs (msg interface)</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>Job events</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>Notifications sent (legacy)</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>Notifications sent (msg)</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>L&B protocol accesses</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>WS queries</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>HTML accesses</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>Plain text accesses</td><td>%i</td><td>%s</td></tr>\n"
                        "<tr><td>RSS accesses</td><td>%i</td><td>%s</td></tr>\n"
                        "</table>\n",
			edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_GLITEJOB_REGS),
                        times[SERVER_STATS_GLITEJOB_REGS],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_PBSJOB_REGS),
                        times[SERVER_STATS_PBSJOB_REGS],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_CONDOR_REGS),
                        times[SERVER_STATS_CONDOR_REGS],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_CREAM_REGS),
                        times[SERVER_STATS_CREAM_REGS],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_SANDBOX_REGS),
                        times[SERVER_STATS_SANDBOX_REGS],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_NOTIF_LEGACY_REGS),
                        times[SERVER_STATS_NOTIF_LEGACY_REGS],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_NOTIF_MSG_REGS),
                        times[SERVER_STATS_NOTIF_MSG_REGS],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_JOB_EVENTS),
                        times[SERVER_STATS_JOB_EVENTS],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_NOTIF_LEGACY_SENT),
                        times[SERVER_STATS_NOTIF_LEGACY_SENT],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_NOTIF_MSG_SENT),
                        times[SERVER_STATS_NOTIF_MSG_SENT],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_LBPROTO),
                        times[SERVER_STATS_LBPROTO],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_WS_QUERIES),
                        times[SERVER_STATS_WS_QUERIES],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_HTML_VIEWS),
                        times[SERVER_STATS_HTML_VIEWS],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_TEXT_VIEWS),
                        times[SERVER_STATS_TEXT_VIEWS],
                        edg_wll_ServerStatisticsGetValue(ctx, SERVER_STATS_RSS_VIEWS),
                        times[SERVER_STATS_RSS_VIEWS]
                );

                for (i = 0; i < SERVER_STATISTICS_COUNT; i++)
                        free(times[i]);
        }

        *message = out;
}

char *edg_wll_ErrorToHTML(edg_wll_Context ctx,int code)
{
	char	*out,*et,*ed;
	char	*msg = edg_wll_HTTPErrorMessage(code);
	edg_wll_ErrorCode	e;

	e = edg_wll_Error(ctx,&et,&ed);
	asprintf(&out,"<html><head><title>Error</title></head>\n"
		"<body><h1>%s</h1>\n"
		"%d: %s (%s)</body></html>",msg,e,et,ed);

	free(et); free(ed);
	return out;
}

