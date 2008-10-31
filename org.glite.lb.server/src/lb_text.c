#ident "$Header$"

#include "lb_text.h"
#include "lb_proto.h"
#include "cond_dump.h"

#include "glite/lb/context-int.h"
#include "glite/lb/xml_conversions.h"
#include "glite/lbu/trio.h"
#include "glite/lbu/db.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif

static char *escape_text(char *text){
	int len = strlen(text);
	char *ret = malloc((len+1)*sizeof(char));
	int reti = 0;
	int retlen = len;
	int i;
	for (i = 0; i < len; i++){
		// escape '\'
		if (text[i] == '\\'){
			ret[reti] = '\\';
			reti++;
			retlen++;
			ret = realloc(ret, (retlen+1)*sizeof(char));
		}
		// replace newline by '\\n'
		if (text[i] == '\n'){
			ret[reti] = '\\';
			reti++;
			ret[reti] = 'n';
			retlen++;
                        ret = realloc(ret, (retlen+1)*sizeof(char));
		}
		else
			ret[reti] = text[i];
		reti++;
	}
	ret[reti] = 0;
	return ret;
}

int edg_wll_QueryToText(edg_wll_Context ctx UNUSED_VAR, edg_wll_Event *eventsOut UNUSED_VAR, char **message UNUSED_VAR)
{
/* not implemented yet */
	return -1;
}

#define TR(name,type,field) \
        if (field) { \
                asprintf(&a,"%s%s=" type "\n", \
                        b, name, field); \
                free(b); \
                b = a; \
        }


int edg_wll_UserInfoToText(edg_wll_Context ctx, edg_wlc_JobId *jobsOut, char **message)
{
        char *a = NULL, *b;
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

	if (a){
		asprintf(&a, "User_jobs=%s\n", b);
		free(b);
		b = a;
	}
	b = a;

	asprintf(&a, "%sUser_subject=%s\n", b, ctx->peerName ? ctx->peerName: "<anonymous>");

        *message = a;

        return 0;
}

int edg_wll_UserNotifsToText(edg_wll_Context ctx, char **notifids, char **message){
	char *a = NULL, *b;
        int i = 0;
        b = strdup("");

        while(notifids[i]){
                if (i == 0)
                        asprintf(&a, "%s", notifids[i]);
                else
                        asprintf(&a, "%s,%s", b, notifids[i]);
                free(b);
                b = a;
                i++;
        }
	if (b)
		asprintf(&a, "User_notifications=%s\n", b);

	*message = a;

	return 0;
}

int edg_wll_NotificationToText(edg_wll_Context ctx UNUSED_VAR, notifInfo *ni, char **message){
	char *a = NULL, *b, *cond, *flags;
	b = strdup("");

	TR("Notif_id", "%s", ni->notifid);
	TR("Destination", "%s", ni->destination);
	TR("Valid_until", "%s", ni->valid);
	if (! edg_wll_Condition_Dump(ni, &cond, 1))
		TR("Conditions", "%s", cond);
	free(cond);
	flags = edg_wll_stat_flags_to_string(ni->flags);
	TR("Flags", "%s", flags);
	free(flags);

	*message = a;

	return 0;
}

/* construct Message-Body of Response-Line for edg_wll_JobStatus */
int edg_wll_JobStatusToText(edg_wll_Context ctx UNUSED_VAR, edg_wll_JobStat stat, char **message)
{
	char *a, *b;
	char    *chid,*chstat;
        char    *jdl,*rsl;

	jdl = strdup("");
	rsl = strdup("");
	
	b = strdup("");

        chid = edg_wlc_JobIdUnparse(stat.jobId);

	TR("Status","%s",(chstat = edg_wll_StatToString(stat.state)));
	free(chstat);
	TR("owner","%s",stat.owner);
	TR("condorId","%s",stat.condorId);
	TR("globusId","%s",stat.globusId);
	TR("localId","%s",stat.localId);
	TR("reason","%s",stat.reason);
	if ( (stat.stateEnterTime.tv_sec) || (stat.stateEnterTime.tv_usec) ) {
		time_t  time = stat.stateEnterTime.tv_sec;
		//TR("State_entered","%s",ctime(&time));
		asprintf(&a, "%sstateEnterTime=%s", b, ctime(&time));
		free(b); b = a;
	}
        if ( (stat.lastUpdateTime.tv_sec) || (stat.lastUpdateTime.tv_usec) ) {
		time_t  time = stat.lastUpdateTime.tv_sec;
		//TR("Last_update","%s",ctime(&time));
		asprintf(&a, "%slastUpdateTime=%s", b, ctime(&time));
                free(b); b = a;
	}
	TR("expectUpdate","%s",stat.expectUpdate ? "YES" : "NO");
	TR("expectFrom","%s",stat.expectFrom);
	TR("location","%s",stat.location);
	TR("destination","%s",stat.destination);
	TR("cancelling","%s",stat.cancelling>0 ? "YES" : "NO");
	if (stat.cancelReason != NULL) {
		TR("cancelReason","%s",stat.cancelReason);
	}
	TR("cpuTime","%d",stat.cpuTime);

	
	TR("done_code","%d",stat.done_code);
	TR("exit_code","%d",stat.exit_code);

        if (stat.jdl){
		char* my_jdl = escape_text(stat.jdl);
		asprintf(&jdl,"jdl=%s\n", my_jdl);
		free(my_jdl);
	}
	if (stat.rsl) asprintf(&rsl,"rsl=%s\n", stat.rsl);

        asprintf(&a, "Job=%s\n"
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
