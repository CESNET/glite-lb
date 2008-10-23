#ident "$Header$"

#include "lb_html.h"
#include "lb_proto.h"

#include "glite/lb/context-int.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif

static char *xmlToHTML(char *xml) {
	char *html = strdup("");
	int i = 0;
	int j = 0;
	while (xml[i]){
		if (xml[i] == '<'){
			html = realloc(html, (j+strlen("&lt;")+1)*sizeof(*html) );
			strcpy(html+j, "&lt;");
			j += strlen("&lt;");
		}
		else if (xml[i] == '>'){
			html = realloc(html, (j+strlen("&gt;")+1)*sizeof(*html) );
			strcpy(html+j, "&gt;");
			j += strlen("&gt;");
		}
		else{
			html = realloc(html, (j+2)*sizeof(*html));
			html[j] = xml[i];
			j++;
		}
		i++;
	}
	html[j] = 0;

	return html;
}

int edg_wll_QueryToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wll_Event *eventsOut UNUSED_VAR, char **message UNUSED_VAR)
{
/* not implemented yet */
	return -1;
}

/* construct Message-Body of Response-Line for edg_wll_UserJobs */
int edg_wll_UserInfoToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wlc_JobId *jobsOut, char **message)
{
        char *pomA = NULL, *pomB;
        int i = 0;

        /* head */
        pomB = strdup("");

        while (jobsOut && jobsOut[i]) {
                char    *chid = edg_wlc_JobIdUnparse(jobsOut[i]);

                asprintf(&pomA,"%s\t\t <li> <a href=\"%s\">%s</a>\r\n",
                        pomB, chid,chid);

                free(chid);
                free(pomB);
                pomB = pomA;
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
	char *pomA = NULL, *pomB;
        pomB = strdup("");

	int i = 0;
        while(notifids && notifids[i]){
                asprintf(&pomA, "%s\t\t <li> <a href=\"/notif/%s\">%s</a>\r\n",
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

#define TR(name,type,field)             \
        if (field) {            \
                asprintf(&pomA,"%s<tr><th align=\"left\">" name ":</th>"        \
                        "<td>" type "</td></tr>",pomB,(field)); \
                free(pomB);                                     \
                pomB = pomA;                                    \
        }

#define GS(string){	\
	asprintf(&pomA, "%s %s", pomB, (string)); \
	free(pomB); \
	pomB = pomA; \
}

int edg_wll_NotificationToHTML(edg_wll_Context ctx UNUSED_VAR, notifInfo *ni, char **message){
	char *pomA = NULL, *pomB, *flags;


	pomB = strdup("");
	flags = edg_wll_stat_flags_to_string(ni->flags);
printf("flags %d - %s", ni->flags, flags);

	TR("Destination", "%s", ni->destination);
	TR("Valid until", "%s", ni->valid);

/*	// Fake the ni->conditions content
	ni->conditions = malloc(3*sizeof(*(ni->conditions)));
	ni->conditions[0] = malloc(2*sizeof(*(ni->conditions[0])));
	ni->conditions[0][0].attr = EDG_WLL_QUERY_ATTR_OWNER;
	ni->conditions[0][0].op = EDG_WLL_QUERY_OP_EQUAL;
	ni->conditions[0][0].value.c = strdup("fila");
	ni->conditions[0][1].attr = EDG_WLL_QUERY_ATTR_UNDEF;
	ni->conditions[1] = malloc(3*sizeof(*(ni->conditions[1])));
	ni->conditions[1][0].attr = EDG_WLL_QUERY_ATTR_OWNER;
        ni->conditions[1][0].op = EDG_WLL_QUERY_OP_EQUAL;
        ni->conditions[1][0].value.c = strdup("fila");
	ni->conditions[1][1].attr = EDG_WLL_QUERY_ATTR_OWNER;
        ni->conditions[1][1].op = EDG_WLL_QUERY_OP_EQUAL;
        ni->conditions[1][1].value.c = strdup("sauron");
        ni->conditions[1][2].attr = EDG_WLL_QUERY_ATTR_UNDEF;
	ni->conditions[2] = NULL;*/

/*	edg_wll_QueryRec **l1;
	edg_wll_QueryRec *l2;
	for (l1 = ni->conditions; *l1; l1++){
		if (l1 != ni->conditions)
			GS ("and");
		l2 = *l1;
		switch (l2->attr){
			case EDG_WLL_QUERY_ATTR_JOBID: GS("jobId");
				break;
			case EDG_WLL_QUERY_ATTR_OWNER: GS("owner");
				break;
			case EDG_WLL_QUERY_ATTR_STATUS: GS("status");
				break;
			case EDG_WLL_QUERY_ATTR_LOCATION: GS("location");
				break;
                        case EDG_WLL_QUERY_ATTR_DESTINATION: GS("destination");
				break;
			case EDG_WLL_QUERY_ATTR_DONECODE: GS("donecode");
				break;
			case EDG_WLL_QUERY_ATTR_USERTAG: GS("usertag");
				break;
			case EDG_WLL_QUERY_ATTR_TIME: GS("time");
				break;
			case EDG_WLL_QUERY_ATTR_LEVEL: GS("level");
				break;
			case EDG_WLL_QUERY_ATTR_HOST: GS("host");
				break;
                        case EDG_WLL_QUERY_ATTR_SOURCE: GS("source");
				break;
                        case EDG_WLL_QUERY_ATTR_INSTANCE: GS("instance");
				break;
                        case EDG_WLL_QUERY_ATTR_EVENT_TYPE: GS("eventtype");
				break;
			case EDG_WLL_QUERY_ATTR_CHKPT_TAG: GS("chkpttag");
				break;
			case EDG_WLL_QUERY_ATTR_RESUBMITTED: GS("resubmitted");
				break;
			case EDG_WLL_QUERY_ATTR_PARENT: GS("parent_job");
				break;
			case EDG_WLL_QUERY_ATTR_EXITCODE: GS("exitcode");
				break;
			case EDG_WLL_QUERY_ATTR_JDL_ATTR: GS("jdl");
				break;
			case EDG_WLL_QUERY_ATTR_STATEENTERTIME: GS("stateentertime");
				break;
			case EDG_WLL_QUERY_ATTR_LASTUPDATETIME: GS("lastupdatetime");
				break;
			case EDG_WLL_QUERY_ATTR_NETWORK_SERVER: GS("networkserver");
				break;
		}
		for (l2 = *l1; l2->attr; l2++){
			if (l2 != *l1) GS ("or");
			switch(l2->op){
				case EDG_WLL_QUERY_OP_EQUAL: GS("=");
					break;
				case EDG_WLL_QUERY_OP_LESS: GS ("<");
					break;
				case EDG_WLL_QUERY_OP_GREATER: GS ("<");
					break;
				case EDG_WLL_QUERY_OP_WITHIN: GS ("within");
					break;
				case EDG_WLL_QUERY_OP_UNEQUAL: GS ("!=");
			}
			char *buf;
			switch (l2->attr){
		                case EDG_WLL_QUERY_ATTR_JOBID:
					GS(edg_wlc_JobIdUnparse(l2->value.j));
        	                        break;
				case EDG_WLL_QUERY_ATTR_DESTINATION:
				case EDG_WLL_QUERY_ATTR_LOCATION:
                	        case EDG_WLL_QUERY_ATTR_OWNER:
					GS(l2->value.c);
                        	        break;
	                        case EDG_WLL_QUERY_ATTR_STATUS:
					//XXX: need interpretation!
					asprintf(&buf, "%i", l2->value.i);
					GS(buf);
					free(buf);
        	                        break;
                	        case EDG_WLL_QUERY_ATTR_DONECODE:
					//XXX: need interpretation!
                                        asprintf(&buf, "%i", l2->value.i);
                                        GS(buf);
                                        free(buf);
                        	        break;
	                        case EDG_WLL_QUERY_ATTR_USERTAG:
					GS(l2->attr_id.tag);
        	                        break;
                	        case EDG_WLL_QUERY_ATTR_TIME:
					buf = ctime(&(l2->value.t.tv_sec));
					if (l2->op == EDG_WLL_QUERY_OP_WITHIN){
						buf[strlen(buf)-1] = 0; // cut out '\n'
						asprintf(&buf, " and %s", ctime(&(l2->value2.t.tv_sec)));
						GS(buf);
					}
					else
						GS(buf);
					free(buf);
                        	        break;
	                        case EDG_WLL_QUERY_ATTR_LEVEL:
        	                        break;
                	        case EDG_WLL_QUERY_ATTR_HOST:
                        	        break;
	                        case EDG_WLL_QUERY_ATTR_SOURCE:
        	                        break;
                	        case EDG_WLL_QUERY_ATTR_INSTANCE:
                        	        break;
	                        case EDG_WLL_QUERY_ATTR_EVENT_TYPE:
        	                        break;
                	        case EDG_WLL_QUERY_ATTR_CHKPT_TAG:
                        	        break;
	                        case EDG_WLL_QUERY_ATTR_RESUBMITTED:
        	                        break;
                	        case EDG_WLL_QUERY_ATTR_PARENT:
                        	        break;
	                        case EDG_WLL_QUERY_ATTR_EXITCODE:
        	                        break;
                	        case EDG_WLL_QUERY_ATTR_JDL_ATTR:
                        	        break;
	                        case EDG_WLL_QUERY_ATTR_STATEENTERTIME:
        	                        break;
                	        case EDG_WLL_QUERY_ATTR_LASTUPDATETIME:
                        	        break;
		                case EDG_WLL_QUERY_ATTR_NETWORK_SERVER:
                	                break;
	                }
			GS("\n");
		}
	}*/

	char *cond = xmlToHTML(ni->conditions_text);
	asprintf(&pomA, "%s<h3>Conditions</h3>\r\n<pre>%s</pre>\r\n",
		pomB, cond);
	free(cond);
	free(pomB);
	pomB = pomA;
	TR("Flags", "%s", flags);
	free(flags);

	asprintf(&pomA, "<html>\r\n\t<body>\r\n"
		"<h2>Notification %s</h2>\r\n"
		"<table halign=\"left\">%s</table>"
		"\t</body>\r\n</html>",
		ni->notifid, pomB);
	
	*message = pomA;

	return 0;
}

/* construct Message-Body of Response-Line for edg_wll_JobStatus */
int edg_wll_JobStatusToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wll_JobStat stat, char **message)
{
        char *pomA, *pomB;
	char	*chid,*chstat;
	char	*jdl,*rsl;

	jdl = strdup("");
	rsl = strdup("");
	
	pomB = strdup("");

        chid = edg_wlc_JobIdUnparse(stat.jobId);

	TR("Status","%s",(chstat = edg_wll_StatToString(stat.state)));
	free(chstat);
	TR("owner","%s",stat.owner);
	TR("Condor Id","%s",stat.condorId);
	TR("Globus Id","%s",stat.globusId);
	TR("Local Id","%s",stat.localId);
	TR("Reason","%s",stat.reason);
	if ( (stat.stateEnterTime.tv_sec) || (stat.stateEnterTime.tv_usec) ) {
		time_t  time = stat.stateEnterTime.tv_sec;
		TR("State entered","%s",ctime(&time));
	}
        if ( (stat.lastUpdateTime.tv_sec) || (stat.lastUpdateTime.tv_usec) ) {
		time_t  time = stat.lastUpdateTime.tv_sec;
		TR("Last update","%s",ctime(&time));
	}
	TR("Expect update","%s",stat.expectUpdate ? "YES" : "NO");
	TR("Expect update from","%s",stat.expectFrom);
	TR("Location","%s",stat.location);
	TR("Destination","%s",stat.destination);
	TR("Cancelling","%s",stat.cancelling>0 ? "YES" : "NO");
	if (stat.cancelReason != NULL) {
		TR("Cancel reason","%s",stat.cancelReason);
	}
	TR("CPU time","%d",stat.cpuTime);

	
	TR("Done code","%d",stat.done_code);
	TR("Exit code","%d",stat.exit_code);

        if (stat.jdl) asprintf(&jdl,"<h3>Job description</h3>\r\n"
		"<pre>%s</pre>\r\n",stat.jdl);

	if (stat.rsl) asprintf(&rsl,"<h3>RSL</h3>\r\n"
		"<pre>%s</pre>\r\n",stat.rsl);


        asprintf(&pomA, "<html>\r\n\t<body>\r\n"
			"<h2>%s</h2>\r\n"
			"<table halign=\"left\">%s</table>"
			"%s%s"
			"\t</body>\r\n</html>",
                	chid,pomB,jdl,rsl);
        free(pomB);

        *message = pomA;

	free(chid);
	free(jdl);
	free(rsl);
        return 0;
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
