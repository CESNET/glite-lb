#ident "$Header$"

#include "lb_html.h"
#include "lb_proto.h"
#include "cond_dump.h"
#include "pretty_print_wrapper.h"

#include "glite/lb/context-int.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

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

#define TR(name,type,field) \
{ \
	int l; \
	if (field){ \
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
	free(pomA); pomA=NULL; \ 
}

int edg_wll_NotificationToHTML(edg_wll_Context ctx UNUSED_VAR, notifInfo *ni, char **message){
	char *pomA = NULL, *pomB = NULL, *flags, *cond;
	int pomL = 0;

	flags = edg_wll_stat_flags_to_string(ni->flags);
	printf("flags %d - %s", ni->flags, flags);

	TR("Destination", "%s", ni->destination);
	TR("Valid until", "%s", ni->valid);
	TR("Flags", "%s", flags);
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
	char	*chid,*chstat;
	char	*jdl,*rsl;

	jdl = strdup("");
	rsl = strdup("");
	
        chid = edg_wlc_JobIdUnparse(stat.jobId);

	TR("Status","%s",(chstat = edg_wll_StatToString(stat.state)));
	free(chstat);
	TR("Owner","%s",stat.owner);
	TR("Condor Id","%s",stat.condorId);
	TR("Globus Id","%s",stat.globusId);
	TR("Local Id","%s",stat.localId);
	TR("Reason","%s",stat.reason);
	if ( (stat.stateEnterTime.tv_sec) || (stat.stateEnterTime.tv_usec) ) {
		time_t  time = stat.stateEnterTime.tv_sec;
		TR("State entered","%s",ctime(&time));
	}
	else
		TR("State entered", "%s", NULL);
        if ( (stat.lastUpdateTime.tv_sec) || (stat.lastUpdateTime.tv_usec) ) {
		time_t  time = stat.lastUpdateTime.tv_sec;
		TR("Last update","%s",ctime(&time));
	}
	else
		TR("Last update", "%s", NULL);
	TR("Expect update","%s",stat.expectUpdate ? "YES" : "NO");
	TR("Expect update from","%s",stat.expectFrom);
	TR("Location","%s",stat.location);
	TR("Destination","%s",stat.destination);
	TR("Cancelling","%s",stat.cancelling>0 ? "YES" : "NO");
	TR("Cancel reason","%s",stat.cancelReason);
	TR("CPU time","%d",stat.cpuTime);

	
	TR("Done code","%d",stat.done_code);
	TR("Exit code","%d",stat.exit_code);

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

int edg_wll_CreamJobStatusToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wll_JobStat stat, char **message)
{
	char *chid, *pomA = NULL, *pomB = NULL, *jdl;
	int pomL = 0;

	jdl = strdup("");

	chid = edg_wlc_JobIdUnparse(stat.jobId);

	TR("Status", "%s", edg_wll_CreamStatToString(stat.cream_state));
	TR("Owner", "%s", stat.cream_owner);
	TR("Endpoint", "%s", stat.cream_endpoint);
	TR("Reason", "%s", stat.cream_reason);
	TR("LRMS id", "%s", stat.cream_lrms_id);
	TR("Node", "%s", stat.cream_node);
	TR("Cancelling", "%s", stat.cream_cancelling > 0 ? "YES" : "NO");
	TR("CPU time", "%d", stat.cream_cpu_time);
	TR("Done code", "%d", stat.cream_done_code);
        TR("Exit code", "%d", stat.cream_exit_code);

	/*
                cream_jw_status
        */
	
	if (stat.jdl){
                char *jdl_unp;
                if (pretty_print(stat.jdl, &jdl_unp) == 0)
                        asprintf(&jdl,"<h3>Job description</h3>\r\n"
                                "<pre>%s</pre>\r\n",jdl_unp);
                else
                        asprintf(&jdl,"<h3>Job description (not a ClassAd)"
                                "</h3>\r\n<pre>%s</pre>\r\n",stat.jdl);
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

