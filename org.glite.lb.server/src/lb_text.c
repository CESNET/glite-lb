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


#include "lb_text.h"
#include "lb_proto.h"
#include "cond_dump.h"
#include "server_state.h"
#include "authz_policy.h"

#include "glite/lb/context-int.h"
#include "glite/lb/xml_conversions.h"
#include "glite/lbu/trio.h"
#include "glite/lbu/db.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
{ \
	int l; \
        if (field) \
		l = asprintf(&a,"%s=" type "\n", \
			name, field); \
	else \
		l = asprintf(&a,"%s=\n", name); \
	b = realloc(b, sizeof(*b)*(pomL+l+1)); \
	strcpy(b+pomL, a); \
	pomL += l; \
	free(a); a=NULL; \
}

#define TRS(name,type,field) \
{ \
        int l; \
        if (field) \
                l = asprintf(&a,"%s=" type "", \
                        name, field); \
        else \
                l = asprintf(&a,"%s=\n", name); \
        b = realloc(b, sizeof(*b)*(pomL+l+1)); \
        strcpy(b+pomL, a); \
        pomL += l; \
        free(a); a=NULL; \
}

#define TRA(type,field) \
{ \
        int l; \
        if (field) \
                l = asprintf(&a,"," type "", \
                        field); \
        else \
                l = asprintf(&a,"\n"); \
        b = realloc(b, sizeof(*b)*(pomL+l+1)); \
        strcpy(b+pomL, a); \
        pomL += l; \
        free(a); a=NULL; \
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
	char *a = NULL, *b = NULL, *cond, *flags;
	int pomL = 0;

	TR("Notif_id", "%s", ni->notifid);
	TR("Owner", "%s", ni->owner);
	TR("Destination", "%s", ni->destination);
	TR("Valid_until", "%s", ni->valid);
	flags = edg_wll_stat_flags_to_string(ni->flags);
	TR("Flags", "%s", flags);
        free(flags);
	if (! edg_wll_Condition_Dump(ni, &cond, 1)){
		TR("Conditions", "%s", cond);
	}
	free(cond);

	*message = b;

	return 0;
}

/* construct Message-Body of Response-Line for edg_wll_JobStatus */
int edg_wll_JobStatusToText(edg_wll_Context ctx UNUSED_VAR, edg_wll_JobStat stat, char **message)
{
	char *a = NULL, *b = NULL;
	char    *chid,*chstat;
        char    *jdl,*rsl;

	jdl = strdup("");
	rsl = strdup("");
	
	int pomL = 0;

        chid = edg_wlc_JobIdUnparse(stat.jobId);

	TR("Status","%s",(chstat = edg_wll_StatToString(stat.state)));
	free(chstat);
	TR("owner","%s",stat.owner);
	TR("payload_owner","%s",stat.payload_owner);
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
	else{
		asprintf(&a, "%sstateEnterTime=", b);
                free(b); b = a;
	}	
        if ( (stat.lastUpdateTime.tv_sec) || (stat.lastUpdateTime.tv_usec) ) {
		time_t  time = stat.lastUpdateTime.tv_sec;
		//TR("Last_update","%s",ctime(&time));
		asprintf(&a, "%slastUpdateTime=%s", b, ctime(&time));
                free(b); b = a;
	}
	else{
		asprintf(&a, "%slastUpdateTime=", b);
                free(b); b = a;
	}
	TR("expectUpdate","%s",stat.expectUpdate ? "YES" : "NO");
	TR("expectFrom","%s",stat.expectFrom);
	TR("location","%s",stat.location);
	TR("destination","%s",stat.destination);
	TR("cancelling","%s",stat.cancelling>0 ? "YES" : "NO");
	TR("cancelReason","%s",stat.cancelReason);
	TR("cpuTime","%d",stat.cpuTime);
	
	TR("done_code","%d",stat.done_code);
	TR("exit_code","%d",stat.exit_code);

        if (stat.jdl){
		char* my_jdl = escape_text(stat.jdl);
		asprintf(&jdl,"jdl=%s\n", my_jdl);
		free(my_jdl);
	}
	else
		asprintf(&jdl,"jdl=\n");
	if (stat.rsl) 
		asprintf(&rsl,"rsl=%s\n", stat.rsl);
	else
		asprintf(&rsl,"rsl=\n");

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

int edg_wll_ConfigurationToText(edg_wll_Context ctx, int admin, char **message){
	char *a = NULL, *b;
	int pomL = 0;
	int i;
	b = strdup("");

	TRS("server_version", "%s\n", VERSION);

	TRS("server_identity", "%s\n", ctx->serverIdentity);

	if (ctx->job_index)
		for (i = 0; ctx->job_index[i]; i++){
			char *ch = edg_wll_QueryRecToColumn(ctx->job_index[i]);
			if (i == 0) TRS("server_indices", "%s", ch)
			else TRA("%s", ch);
			free(ch);
		}
	if (i > 0)
                TRA("%s", NULL);

	if (ctx->msg_brokers)
		for (i = 0; ctx->msg_brokers[i]; i++){
			if (i == 0) TRS("msg_brokers", "%s", ctx->msg_brokers[i])
			else TRA("%s", ctx->msg_brokers[i]);
		}
	if (i > 0)
		TRA("%s", NULL);

	if (ctx->msg_prefixes)
		for (i = 0; ctx->msg_prefixes[i]; i++)
			if (i == 0) TRS("msg_prefixes", "%s", ctx->msg_prefixes[i])
			else TRA("%s", ctx->msg_prefixes[i]);
	if (i > 0)
                TRA("%s", NULL);

	/* only for superusers */
	if (admin){
		char *dbname, *dbhost;
		dbname = glite_lbu_DBGetName(ctx->dbctx);
		dbhost = glite_lbu_DBGetHost(ctx->dbctx);
		TRS("database_name", "%s\n", dbname);
		TRS("database_host", "%s\n", dbhost);

		free(dbname);
		free(dbhost);

		char *pf = NULL;
		int fd;
		if (ctx->authz_policy_file && (fd = open(ctx->authz_policy_file, O_RDONLY)) >= 0){
			off_t size = lseek(fd, 0, SEEK_END) - lseek(fd, 0, SEEK_SET);
			char *pft = (char*)malloc(size);
			read(fd, pft, size);
			close(fd);
			pf = escape_text(pft);
			
		}
		TRS("authz_policy_file", "%s\n", pf);
		if (pf) free(pf);

		edg_wll_authz_policy ap = edg_wll_get_server_policy();
		int i, j, k, l = 0;
		for (i = 0; i < ap->actions_num; i++){
			if (ap->actions[i].id == ADMIN_ACCESS)
				for (j = 0; j < ap->actions[i].rules_num; j++)
					for (k = 0; k < ap->actions[i].rules[j].attrs_num; k++){
						if (l == 0)
							TRS("admins", "\"%s\"", ap->actions[i].rules[j].attrs[k].value)
						else
							TRA("\"%s\"", ap->actions[i].rules[j].attrs[k].value);
						l++;
					}
		}
		if (l) TRA("%s", NULL);

		char *start = NULL, *end = NULL;
		edg_wll_GetServerState(ctx, EDG_WLL_STATE_DUMP_START, &start);
		edg_wll_GetServerState(ctx, EDG_WLL_STATE_DUMP_END, &end);
		TRS("dump_start", "%s\n", start);
		TRS("dump_end", "%s\n", end);
	}

	*message = b;

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

