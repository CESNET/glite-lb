#ident "$Header$"

#include "lb_html.h"
#include "lb_proto.h"

#include "glite/lb/context-int.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
int edg_wll_UserJobsToHTML(edg_wll_Context ctx, edg_wlc_JobId *jobsOut, char **message)
{
        char *pomA, *pomB;
        int i = 0;

	/* head */
	pomB = strdup("");	

        while (jobsOut[i]) {
		char	*chid = edg_wlc_JobIdUnparse(jobsOut[i]);

                asprintf(&pomA,"%s\t\t <li> <a href=\"%s\">%s</a>\r\n",
                        pomB, chid,chid);

		free(chid);
                free(pomB);
                pomB = pomA;
                i++;
        }

        asprintf(&pomA, "<html>\r\n\t<body>\r\n"
			"<h2><B>User jobs</B></h2>\r\n"
			"User subject: %s<p>"
			"<ul>%s</ul>"
			"\t</body>\r\n</html>",ctx->peerName,pomB);
        free(pomB);

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

#define TR(name,type,field) 		\
	if (field) {		\
		asprintf(&pomA,"%s<tr><th align=\"left\">" name ":</th>"	\
			"<td>" type "</td></tr>",pomB,(field));	\
		free(pomB);					\
		pomB = pomA;					\
	}

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
