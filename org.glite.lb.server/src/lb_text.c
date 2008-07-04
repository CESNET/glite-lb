#ident "$Header$"

#include "lb_text.h"
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

int edg_wll_QueryToText(edg_wll_Context ctx UNUSED_VAR, edg_wll_Event *eventsOut UNUSED_VAR, char **message UNUSED_VAR)
{
/* not implemented yet */
	return -1;
}

/* construct Message-Body of Response-Line for edg_wll_UserJobs */
int edg_wll_UserJobsToText(edg_wll_Context ctx, edg_wlc_JobId *jobsOut, char **message)
{
	char *a, *b;
	int i = 0;
	b = strdup("");

	while (jobsOut[i]){
		char *chid = edg_wlc_JobIdUnparse(jobsOut[i]);

		if (i == 0)
			asprintf(&a, "%s%s", b, chid);
		else
			asprintf(&a, "%s,%s", b, chid);

		free(chid);
		free(b);
		b = a;
		i++;
	}

	asprintf(&a, "User_jobs=%s\n"
		     "User_subject=%s\n",
		b, ctx->peerName ? ctx->peerName: "<anonymous>");

        *message = a;

        return 0;
}


/* construct Message-Body of Response-Line for edg_wll_JobStatus */
int edg_wll_JobStatusToText(edg_wll_Context ctx UNUSED_VAR, edg_wll_JobStat stat, char **message)
{
	char *a, *b;
	char    *chid,*chstat;
        char    *jdl,*rsl;

	*chid = edg_wlc_JobIdUnparse(stat.jobId);

	#define TR(name,type,field) \
        if (field) { \
                asprintf(&a,"%s%s=" type "\n", \
                        b, name, field); \
                free(b); \
                b = a; \
        }

	jdl = strdup("");
	rsl = strdup("");
	
	b = strdup("");

        chid = edg_wlc_JobIdUnparse(stat.jobId);

	TR("Status","%s",(chstat = edg_wll_StatToString(stat.state)));
	free(chstat);
	TR("owner","%s",stat.owner);
	TR("Condor_Id","%s",stat.condorId);
	TR("Globus_Id","%s",stat.globusId);
	TR("Local_Id","%s",stat.localId);
	TR("Reason","%s",stat.reason);
	if ( (stat.stateEnterTime.tv_sec) || (stat.stateEnterTime.tv_usec) ) {
		time_t  time = stat.stateEnterTime.tv_sec;
		//TR("State_entered","%s",ctime(&time));
		asprintf(&a, "%sState_entered=%s", b, ctime(&time));
		free(b); b = a;
	}
        if ( (stat.lastUpdateTime.tv_sec) || (stat.lastUpdateTime.tv_usec) ) {
		time_t  time = stat.lastUpdateTime.tv_sec;
		//TR("Last_update","%s",ctime(&time));
		asprintf(&a, "%sLast_update=%s", b, ctime(&time));
                free(b); b = a;
	}
	TR("Expect_update","%s",stat.expectUpdate ? "YES" : "NO");
	TR("Expect_update_from","%s",stat.expectFrom);
	TR("Location","%s",stat.location);
	TR("Destination","%s",stat.destination);
	TR("Cancelling","%s",stat.cancelling>0 ? "YES" : "NO");
	if (stat.cancelReason != NULL) {
		TR("Cancel_reason","%s",stat.cancelReason);
	}
	TR("CPU_time","%d",stat.cpuTime);

	
	TR("Done_code","%d",stat.done_code);
	TR("Exit_code","%d",stat.exit_code);

        if (stat.jdl) asprintf(&jdl,">Job_description = %s\n", stat.jdl);

	if (stat.rsl) asprintf(&rsl,"RSL = %s\n", stat.rsl);

        asprintf(&a, "Job = %s\n"
			"%s"
			"%s"
			"%s",
                	chid,b,jdl,rsl);
        free(b);

        *message = a;

	free(chid);
	free(jdl);
	free(rsl);
        return 0;
}

char *edg_wll_ErrorToText(edg_wll_Context ctx,int code)
{
	char	*out,*et,*ed;
	char	*msg = edg_wll_HTTPErrorMessage(code);
	edg_wll_ErrorCode	e;

	e = edg_wll_Error(ctx,&et,&ed);
	asprintf(&out,"Error=%s\n"
		"Code=%d\n"
		"Description=%s (%s)\n"
		,msg,e,et,ed);

	free(et); free(ed);
	return out;
}
