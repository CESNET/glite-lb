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
int edg_wll_UserInfoToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wlc_JobId *jobsOut, char **notifids, char **message)
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

	char *pomC = NULL, *pomD;
	pomD = strdup("");
	i = 0;

	while(notifids && notifids[i]){
		asprintf(&pomC, "%s\t\t <li> <a href=\"/notif/%s\">%s</a>\r\n",
				pomD,
				notifids[i],
				notifids[i]
			);
		free(pomD);
		pomD = pomC;
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
	if (pomC){
		asprintf(&ret, "%s<h2><B>User notifications</B></h2>\r\n"
                        "<ul>%s</ul>",
                        pomA, pomD
                );
		free(pomA);
		free(pomD);
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

#define TR(name,type,field)             \
        if (field) {            \
                asprintf(&pomA,"%s<tr><th align=\"left\">" name ":</th>"        \
                        "<td>" type "</td></tr>",pomB,(field)); \
                free(pomB);                                     \
                pomB = pomA;                                    \
        }

int edg_wll_NotificationToHTML(edg_wll_Context ctx UNUSED_VAR, notifInfo *ni, char **message){
	char *pomA, *pomB, *flags;


	pomB = strdup("");
	flags = edg_wll_stat_flags_to_string(ni->flags);
printf("flags %d - %s", ni->flags, flags);

	TR("Destination", "%s", ni->destination);
	TR("Valid until", "%s", ni->valid);
	char *cond = xmlToHTML(ni->conditions);
	asprintf(&pomA, "%s<h3>Conditions</h3>\r\n<pre>%s</pre>\r\n",
		pomB, cond);
	free(cond);
	free(pomB);
	pomB = pomA;
	TR("Flags", "%s", flags);
	free(flags);
	if (ni->JDL_VirtualOrganisation && ni->JDL_VirtualOrganisation[0])
		TR("JDL VirtualOrganisation", "%s", ni->JDL_VirtualOrganisation);
	if (ni->STD_owner && ni->STD_owner[0])
		TR("STD owner", "%s", ni->STD_owner);
	if (ni->STD_network_server && ni->STD_network_server[0])
		TR("STD network server", "%s", ni->STD_network_server);

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
