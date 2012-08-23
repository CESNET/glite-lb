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
#include "index.h"
#include "server_state.h"
#include "lb_authz.h"

#include "glite/lb/context-int.h"
#include "glite/lb/xml_conversions.h"

#include "glite/lbu/log.h"
#include "glite/lbu/db.h"
#include "glite/lbu/escape.h"

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

#define TR(name,type,field,null) \
{ \
	int l; \
	if ((field) != (null)){ \
		l = asprintf(&pomA,"<tr><th align=\"left\">" name ":</th>" \
			"<td>" type "</td></tr>\n", (field)); \
	} \
	else{ \
                l = asprintf(&pomA,"<tr class=\"notused\"><th>" name \
                        "</th></tr>\n"); \
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
                        "<td><a href=\""type"\">" type "</a></td></tr>\n", (field), (field)); \
        } \
        else{ \
                l = asprintf(&pomA,"<tr class=\"notused\"><th>" name \
                        "</th></tr>\n"); \
        } \
        pomB = realloc(pomB, sizeof(*pomB)*(pomL+l+1)); \
        strcpy(pomB+pomL, pomA); \
        pomL += l; \
        free(pomA); \
	pomA=NULL; \
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

void add_row(char **body, char *text_title, char *html_title, char *value, char *link, http_output_type text) {
	char *newbody = NULL, *target;
	int len;

	switch (text) {
		case HTTP_OUTPUT_TYPE_HTML:
			if (value) {
				if (link) len = asprintf(&newbody,"%s<tr><th align=\"left\">%s</th><td><a href=\"%s\">%s</a></td></tr>\n", *body ? *body : "", html_title, link, value);
				else len = asprintf(&newbody,"%s<tr><th align=\"left\">%s</th><td>%s</td></tr>\n", *body ? *body : "", html_title, value);
			}
			else len = asprintf(&newbody,"%s<tr class=\"notused\"><th>%s</th></tr>\n", *body ? *body : "", html_title);
			break;
		case HTTP_OUTPUT_TYPE_TEXT_APPEND:
			if ((target = strrchr(*body ? *body : "", '\n'))) target[0]='\0'; 
		case HTTP_OUTPUT_TYPE_TEXT:
			len = asprintf(&newbody,"%s%s=%s\n", *body ? *body : "", text_title, value ? value : "");
			break;
	}

	free(*body);
        *body = newbody;

}

char *get_html_header(edg_wll_Context ctx, int text) {
	char *header = NULL;
	size_t header_len = 0, rlen = 0;
	FILE *header_file;

	if (text) return NULL;

	if ((header_file = fopen(ctx->html_header_file, "r"))) { 
		rlen = getdelim( &header, &header_len, '\0', header_file);
		fclose (header_file);
	}
	else rlen = -1;
	
	if (rlen == -1 ) header=strdup("<style type=\"text/css\">tr.notused {color: gray; text-align: left;}</style>");

	return header;
}

int edg_wll_QueryToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wll_Event *eventsOut UNUSED_VAR, char **message UNUSED_VAR)
{
/* not implemented yet */
	return -1;
}

int jobstat_cmp (const void *a, const void *b) {
	static int compr;
	// This code should sort IDs by parent ID first, then by JobID.
	compr = strcmp(((JobIdSorter*)a)->key, ((JobIdSorter*)b)->key);
	if (compr) return compr;
	if (!((JobIdSorter*)a)->parent_unparsed) return -1;
	if (!((JobIdSorter*)b)->parent_unparsed) return 1;
	return strcmp(((JobIdSorter*)a)->id_unparsed, ((JobIdSorter*)b)->id_unparsed);
}

int edg_wll_ConfigurationToHTML(edg_wll_Context ctx, int admin, char **message, int text){
        char *a = NULL, *b, *header, *out, *out_tmp;
        int i = 0;
        b = strdup("");

        header = get_html_header(ctx, text);
        if (!text) asprintf(&out, "<HTML>\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<TITLE>Server Configuration</TITLE>\n<HEAD>%s\n</HEAD>\n<BODY>\n"
                "<h2>LB Server Configuration</h2>\n"
                "<table halign=\"left\">\n"
                "<tr><td>Option</td><td>Value</td></tr>\n",
                header ? header : "");
        else
                out = strdup("");

        free(header);

	add_row(&out, "server_version", "Server version", VERSION, NULL, text);

	add_row(&out, "server_identity", "Server identity", ctx->serverIdentity, NULL, text);

	for (i = 0; ctx->job_index && ctx->job_index[i]; i++){
		char *idx_title = edg_wll_QueryRecToColumn(ctx->job_index[i]);
		asprintf(&out_tmp, "%s%s%s", a ? a : "", a ? (text ? "," : "<BR>") : "", idx_title);
		free(a);
		a = out_tmp;
	}
	add_row(&out, "server_indices", "Server indices", a, NULL, text);
	free(a);

	a = NULL;	
	for (i = 0; ctx->msg_brokers && ctx->msg_brokers[i]; i++){
		asprintf(&out_tmp, "%s%s%s", a ? a : "", a ? (text ? "," : "<BR>") : "", ctx->msg_brokers[i]);
		free(a);
		a = out_tmp;
	}
	add_row(&out, "msg_brokers", "Messaging brokers", a, NULL, text);
	free(a);

	a = NULL;	
	for (i = 0; ctx->msg_prefixes && ctx->msg_prefixes[i]; i++){
		asprintf(&out_tmp, "%s%s%s", a ? a : "", a ? (text ? "," : "<BR>") : "", ctx->msg_prefixes[i]);
		free(a);
		a = out_tmp;
	}
	add_row(&out, "msg_prefixes", "Messaging prefixes", a, NULL, text);
	free(a);

        /* only for superusers */
        if (admin){
                char *dbname, *dbhost;
                dbname = glite_lbu_DBGetName(ctx->dbctx);
                dbhost = glite_lbu_DBGetHost(ctx->dbctx);
		add_row(&out, "database_name", "Database name", dbname, NULL, text);
		add_row(&out, "database_host", "Database host", dbhost, NULL, text);
                free(dbname);
                free(dbhost);

                char *pf = NULL, *pf_pars = NULL;
		FILE *authz_file;
		size_t rlen, alloc_len;
		if ((authz_file = fopen(ctx->authz_policy_file, "r"))) { 
			rlen = getdelim(&pf, &alloc_len, '\0', authz_file);
			fclose (authz_file);
		}
		else rlen = -1;

		if (text) pf_pars = glite_lbu_EscapeULM(pf);

		add_row(&out, "authz_policy_file", "Authorization<BR>policy file", rlen < 0 ? NULL : (pf_pars ? pf_pars : pf), NULL, text);
		free(pf_pars);
		free(pf);

		a = NULL;
                edg_wll_authz_policy ap = edg_wll_get_server_policy();
                int i, j, k;
                for (i = 0; i < ap->actions_num; i++){
                        if (ap->actions[i].id == ADMIN_ACCESS)
                                for (j = 0; j < ap->actions[i].rules_num; j++)
                                        for (k = 0; k < ap->actions[i].rules[j].attrs_num; k++){
						asprintf(&out_tmp, "%s%s%s", a ? a : "", a ? (text ? "," : "<BR>") : "", ap->actions[i].rules[j].attrs[k].value);
						free(a);
						a = out_tmp;
                                        }
                }
		add_row(&out, "admins", "Super Users", a, NULL, text);
		free(a);

                char *start = NULL, *end = NULL;
                edg_wll_GetServerState(ctx, EDG_WLL_STATE_DUMP_START, &start);
                edg_wll_GetServerState(ctx, EDG_WLL_STATE_DUMP_END, &end);
		add_row(&out, "dump_start", "Last dump start", start, NULL, text);
		add_row(&out, "dump_end", "Last dump end", end, NULL, text);
		add_row(&out, "html_header", "HTML Header file", ctx->html_header_file, NULL, text);
        }

        if (!text) {
                asprintf(&out_tmp, "%s</table>\n</BODY>\n</HTML>", out);
                free(out);
                out = out_tmp;
        }

        *message = out;
        return 0;
}


/* construct Message-Body of Response-Line for edg_wll_UserJobs */
int edg_wll_UserInfoToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wlc_JobId *jobsOut, edg_wll_JobStat *statsOut, char **message, int text)
{
        char *pomA = NULL, *pomB, *pomC, *header = NULL, *recent_parent = NULL;
	int i, total = 0, bufsize, written = 0, linlen, wassub = 0, issub = 0, lineoverhead;
	JobIdSorter *order;

        while (jobsOut && jobsOut[total]) total++;

	order=(JobIdSorter*)malloc(sizeof(JobIdSorter) * total);

	for (i = 0; i < total; i++) {
		order[i].id_unparsed = edg_wlc_JobIdUnparse(jobsOut[i]);
		order[i].parent_unparsed = statsOut ? edg_wlc_JobIdUnparse(statsOut[i].parent_job) : NULL;
		order[i].key = order[i].parent_unparsed ? order[i].parent_unparsed : order[i].id_unparsed;
		order[i].order = i;
	} 

	if (text) {
		linlen = asprintf(&pomA,"User_jobs=");
		asprintf(&pomC, "User_subject=%s\n", ctx->peerName ? ctx->peerName: "<anonymous>");
		lineoverhead = 2;
	}
	else {		
		qsort(order, total, sizeof(JobIdSorter), jobstat_cmp); 

		header = get_html_header(ctx, text);

		linlen = asprintf(&pomA, "<HTML>\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<TITLE>User Jobs</TITLE>\n<HEAD>\n%s\n</HEAD>\n<BODY>\n"
			"<h2><B>User Jobs</B></h2>\nTotal of %d<P><UL>\n", header ? header : "", total);

		free(header);

		asprintf(&pomC, "</UL>User subject: %s<p>\n"
			"</BODY>\n</HTML>",
			ctx->peerName?ctx->peerName: "&lt;anonymous&gt;" );

		lineoverhead = 35;//Good idea to change this value if the table row format changes, but not 100-% necessary. Loop will realloc if necessary.
	}

	bufsize = (strlen(order[0].id_unparsed)*2+lineoverhead)*total + linlen + strlen(pomC);
	pomB = (char*)malloc(sizeof(char) * bufsize);

	strcpy(pomB + written, pomA);
	written=linlen;
	free(pomA);

        /* head */
	for (i = 0; i < total; i++) {
		if (text) linlen = asprintf(&pomA,"%s%s", order[i].id_unparsed, i + 1 == total ? "\n" : ",");
		else {
			issub = order[i].parent_unparsed && recent_parent && !strcmp(recent_parent, order[i].parent_unparsed) ? 1 : 0;
			linlen = asprintf(&pomA, "%s<li><a href=\"%s\">%s</a></li>\n",
				issub ? (wassub++ ? "" : "<ul>") : (wassub ? "</UL>" : ""),
				order[i].id_unparsed,
				order[i].id_unparsed );
			if (!issub || !order[i].parent_unparsed) { wassub = 0; recent_parent = order[i].id_unparsed; }
		}

		while (written+linlen >= bufsize) {
			bufsize = (bufsize + 5) * 1.2;
			pomB = realloc(pomB, sizeof(char) * bufsize);
		}

		strcpy(pomB + written, pomA);
		written = written + linlen;
		free(pomA);
	}

	if (written+strlen(pomC)+strlen("</UL>") >= bufsize) {
		pomB = realloc(pomB, bufsize + strlen(pomC) + strlen("</UL>"));
	}

	if (wassub) { strcpy(pomB + written, "</UL>"); written = written + strlen("</UL>"); }
	strcpy(pomB + written, pomC);
	free(pomC);

	for (i = 0; i < total; i++) {
		free(order[i].id_unparsed);
		free(order[i].parent_unparsed);
	} 
	free(order);

        *message = pomB;

        return 0;
}

int edg_wll_UserNotifsToHTML(edg_wll_Context ctx UNUSED_VAR, char **notifids, char **message, http_admin_option option, int adm) {
	char *pomA = NULL, *pomB = NULL;
	char *mylink = NULL, *alllink = NULL, *foreignlink = NULL, *heading = NULL, *header = NULL;
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

	if (adm) {
		asprintf(&mylink,"%s just your registrations%s | ",
			option != HTTP_ADMIN_OPTION_MY ? "<A HREF=\"/NOTIF:\">View" : "<B>Viewing",
			option != HTTP_ADMIN_OPTION_MY ? "</A>" : "</B>");
		asprintf(&alllink,"%s all registrations%s | ",
			option != HTTP_ADMIN_OPTION_ALL ? "<A HREF=\"/NOTIF:?all\">View" : "<B>Viewing",
			option != HTTP_ADMIN_OPTION_ALL ? "</A>" : "</B>");
		if (option == HTTP_ADMIN_OPTION_ALL) asprintf(&heading,"All notifications");
		asprintf(&foreignlink,"%s registrations by other users%s<P>",
			option != HTTP_ADMIN_OPTION_FOREIGN ? "<A HREF=\"/NOTIF:?foreign\">View" : "<B>Viewing",
			option != HTTP_ADMIN_OPTION_FOREIGN ? "</A>" : "</B>");
		if (option == HTTP_ADMIN_OPTION_FOREIGN) asprintf(&heading,"Other users' notifications");
	}
	if (!heading) asprintf(&heading,"Your notifications");

	header = get_html_header(ctx, 0);
	char *ret;
	asprintf(&ret, "<HTML>\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<TITLE>Notifications</TITLE>\n<HEAD>\n<HEAD>\n%s\n</HEAD>\n<body>\r\n"
			"<h2><B>%s</B></h2>\r\n"
			"<P>%s%s%s"
			"<P>Total of %d"
                        "<ul>%s</ul>"
			"\t</body>\r\n</HTML>",
			header,
			heading,
			mylink ? mylink : "",
			alllink ? alllink : "",
			foreignlink ? foreignlink : "",
                        i, pomA ? pomA : "No registrations found"
        );
        free(pomA);
	free(mylink);
	free(alllink);
	free(foreignlink);
	free(heading);
	free(header);

	*message = ret;

	return 0;
}


int edg_wll_NotificationToHTML(edg_wll_Context ctx UNUSED_VAR, notifInfo *ni, char **message, int text){
	char *out_tmp = NULL, *flags = NULL, *cond, *header = NULL, *out;
	int alljobs = strcmp(ni->jobid,"all_jobs");

        header = get_html_header(ctx, text);
        if (!text) asprintf(&out, "<HTML>\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<TITLE>Notification Detail</TITLE>\n<HEAD>%s\n</HEAD>\n<BODY>\n"
                "<h2>Notification %s</h2>\n"
                "<table halign=\"left\">\n",
                header ? header : "",
		ni->notifid);
        else
                out = strdup("");

        free(header);

	flags = edg_wll_stat_flags_to_string(ni->flags);

	add_row(&out, "Notif_id", "Notification ID", ni->notifid, NULL, text);
	add_row(&out, "Owner", "Owner", ni->owner, NULL, text);
	add_row(&out, "Destination", "Destination", ni->destination, NULL, text);
	add_row(&out, "Valid_until", "Valid until", ni->valid, NULL, text);
	add_row(&out, "Flags", "Flags", flags, NULL, text);
	add_row(&out, "Job_id", "Job ID", alljobs ? ni->jobid : NULL, alljobs ? ni->jobid : NULL, text);

        free(flags);

	edg_wll_Condition_Dump(ni, &cond, text);
	add_row(&out, "Conditions", "Conditions", cond, NULL, text);
	free(cond);

        if (!text) {
                asprintf(&out_tmp, "%s</table>\n</BODY>\n</HTML>", out);
                free(out);
                out = out_tmp;
        }
	
	*message = out;

	return 0;
}

/* construct Message-Body of Response-Line for edg_wll_JobStatus */
int edg_wll_GeneralJobStatusToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wll_JobStat stat, char **message, int text)
{
        char *out_tmp = NULL, *header = NULL, *out, *history = NULL, *jdl = NULL, *rsl = NULL, *children = NULL, *chtemp;
	time_t time;

        char *pomA = NULL;
	int	i;

        chtemp = edg_wlc_JobIdUnparse(stat.jobId);

        header = get_html_header(ctx, text);
        if (!text) asprintf(&out, "<HTML>\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<TITLE>Job Detail</TITLE>\n<HEAD>%s\n</HEAD>\n<BODY>\n"
                "<h2>%s</h2>\n"
                "<table halign=\"left\">\n",
                header ? header : "",
                chtemp);
        else
                out = strdup("");

        free(header);

	if (stat.history) {
		if (!text) asprintf(&history,"<h3>Job History</h3>\r\n<pre>%s</pre>\r\n", stat.history);
		else {
			out_tmp = glite_lbu_EscapeULM(stat.history);
			asprintf(&history,"history=%s\n", out_tmp);
			free(out_tmp); }
	}
	else {
		if (!text) asprintf(&history,"<H3><A HREF=\"%s?flags=history\">Display full history of this job</A></H3>\n", chtemp);
		else asprintf(&history,"history=\n");
	}

	add_row(&out, "Job", "Job ID", chtemp, chtemp, text);
	free(chtemp);
	chtemp = stat.parent_job ? edg_wlc_JobIdUnparse(stat.parent_job) : NULL;
	add_row(&out, "parent_job", "Parent job", chtemp, chtemp, text);
	free(chtemp);
	add_row(&out, "Status", "Status", (chtemp = edg_wll_StatToString(stat.state)), NULL, text);
	free(chtemp);
	add_row(&out, "job_type", "Type", edg_wll_StatusJobtypeNames[stat.jobtype], NULL, text);

	switch (stat.jobtype) {
		case EDG_WLL_STAT_CREAM:
		        chtemp = edg_wll_CreamStatToString(stat.cream_state);
        		add_row(&out, "CREAM_Status", "CREAM Status", chtemp, NULL, text);
	        	free(chtemp);
			break;
		case EDG_WLL_STAT_VIRTUAL_MACHINE:
		        chtemp = edg_wll_VMStatToString(stat.vm_state);
        		add_row(&out, "VM_Status", "VM Status", chtemp, NULL, text);
	        	free(chtemp);
			break;
		case EDG_WLL_STAT_PBS:
			break;
//		case EDG_WLL_STAT_FILE_TRANSFER:
//		case EDG_WLL_STAT_FILE_TRANSFER_COLLECTION:
		default:
			// No handling for other job types
			break;
	}

	add_row(&out, "owner", "Owner", stat.owner, NULL, text);
	add_row(&out, "payload_owner", "Payload Owner", stat.payload_owner, NULL, text);
	add_row(&out, "condorId", "Condor Id", stat.condorId, NULL, text);
	add_row(&out, "globusId", "Globus Id", stat.globusId, NULL, text);
	add_row(&out, "localId", "Local Id", stat.localId, NULL, text);
	add_row(&out, "reason", "Reason", stat.reason, NULL, text);
	time = stat.stateEnterTime.tv_sec;
	asprintf(&chtemp, "%s", ctime(&time));
	chtemp[strlen(chtemp)-1]='\0';
	add_row(&out, "stateEnterTime", "State entered", time > 0 ? chtemp : NULL, NULL, text);
	free(chtemp);
	time = stat.lastUpdateTime.tv_sec;
	asprintf(&chtemp, "%s", ctime(&time));
	chtemp[strlen(chtemp)-1]='\0';
	add_row(&out, "lastUpdateTime", "Last update", time > 0 ? chtemp : NULL, NULL, text);
	free(chtemp);
	add_row(&out, "expectUpdate", "Expect update", stat.expectUpdate ? "YES" : "NO", NULL, text);
	add_row(&out, "expectFrom", "Expect update from", stat.expectFrom, NULL, text);
	add_row(&out, "location", "Location", stat.location, NULL, text);
	add_row(&out, "destination", "Destination", stat.destination, NULL, text);
	add_row(&out, "cancelling", "Cancelling", stat.cancelling>0 ? "YES" : "NO", NULL, text);
	add_row(&out, "cancelReason", "Cancel reason", stat.cancelReason, NULL, text);
	asprintf(&chtemp, "%d", stat.cpuTime);
	add_row(&out, "cpuTime", "CPU time", chtemp, NULL, text);
	free(chtemp);
	if (stat.done_code != -1) asprintf(&chtemp, "%d", stat.done_code); 
	else chtemp = NULL;
	add_row(&out, "done_code", "Done code", chtemp, NULL, text);
	free(chtemp);
	if (stat.exit_code != -1) asprintf(&chtemp, "%d", stat.exit_code); 
	else chtemp = NULL;
	add_row(&out, "exit_code", "Exit code", chtemp, NULL, text);
	free(chtemp);
	chtemp = stat.isb_transfer ? edg_wlc_JobIdUnparse(stat.isb_transfer) : NULL;
	add_row(&out, "input_sandbox", "input sandbox", chtemp, chtemp, text);
	free(chtemp);
	chtemp = stat.osb_transfer ? edg_wlc_JobIdUnparse(stat.osb_transfer) : NULL;
	add_row(&out, "output_sandbox", "Output sandbox", chtemp, chtemp, text);
	free(chtemp);
	add_row(&out, "acl", "ACL", stat.acl, NULL, text);
        if (stat.user_tags) {
		chtemp = NULL;
                for (i=0; stat.user_tags[i].tag; i++) {
			asprintf(&out_tmp, "%s%s%s%s=%s%s",
				chtemp ? chtemp : "",
				i > 0 ? (text ? "," : "<BR>") : "",
				text ? "\"" : "",
				stat.user_tags[i].tag,
				stat.user_tags[i].value,
				text ? "\"" : "");
			free(chtemp);
			chtemp = out_tmp;
		}
		add_row(&out, "user_tags", "User Tags", chtemp, NULL, text);
		free(chtemp);
	}


        if (!text) {
                asprintf(&out_tmp, "%s</table>\n</BODY>\n</HTML>", out);
                free(out);
                out = out_tmp;
        }
	
	if (stat.jdl){
		if (text) {
		        char* my_jdl = glite_lbu_EscapeULM(stat.jdl);
	                asprintf(&jdl,"jdl=%s\n", my_jdl);
        	        free(my_jdl);
		}
		else {
			char *jdl_unp;
			if (pretty_print(stat.jdl, &jdl_unp) == 0)
				asprintf(&jdl,"<h3>Job description</h3>\r\n"
                        	        "<pre>%s</pre>\r\n",jdl_unp);
			else
				asprintf(&jdl,"<h3>Job description (not a ClassAd)"
					"</h3>\r\n<pre>%s</pre>\r\n",stat.jdl);
		}
	}
	else if (text) asprintf(&jdl,"jdl=\n");

	if (stat.rsl) {
		if (!text) asprintf(&rsl,"<h3>RSL</h3>\r\n<pre>%s</pre>\r\n",stat.rsl);
		else asprintf(&rsl,"rsl=%s\n", stat.rsl);
	}
	else if (text) asprintf(&rsl,"rsl=\n");

	if ((!text) && (stat.jobtype == EDG_WLL_STAT_COLLECTION) && (stat.children_num > 0)){
		asprintf(&children, "<h3>Children</h3>\r\n");
		for (i = 0; i < stat.children_num; i++){
			asprintf(&pomA,"%s\t\t <li/> <a href=\"%s\">%s</a>\r\n",
                        children, stat.children[i], stat.children[i]);
			children = pomA;
		}
	}

	asprintf(&pomA, "%s%s%s%s%s%s", 
                	out,
			history ? history : "",
			jdl ? jdl : "",
			rsl ? rsl : "",
			children ? children : "",
			text ? "" : "\n</body></html>");

        *message = pomA;

	free(out);
	free(jdl);
	free(rsl);
	free(children);
        return 0;
}

int edg_wll_CreamJobStatusToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wll_JobStat stat, char **message)
{
	char *chid, *pomA = NULL, *pomB = NULL, *jdl, *header = NULL;
	char *lbstat, *creamstat;
	int pomL = 0;

	jdl = strdup("");

	chid = edg_wlc_JobIdUnparse(stat.jobId);

	TR("CREAM ID", "%s", stat.cream_id, NULL);
	TR("Status", "%s", (lbstat = edg_wll_StatToString(stat.state)), NULL);
	free(lbstat);
	TR("CREAM Status", "%s", (creamstat = edg_wll_CreamStatToString(stat.cream_state)), NULL);
	free(creamstat);
	TR("Type","%s",edg_wll_StatusJobtypeNames[stat.jobtype], NULL);
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
	header = get_html_header(ctx, 0);
	asprintf(&pomA, "<HTML>\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<TITLE>Job Detail</TITLE>\n<HEAD>\n<HEAD>\n%s\n</HEAD>\n<body>\r\n"
                        "<h2>%s</h2>\r\n"
                        "<table halign=\"left\">%s</table>"
                        "%s"
                        "\t</body>\r\n</html>",
			header,
                        chid,pomB,jdl);
        free(pomB); free(jdl);

        *message = pomA;

	return 0;
}

int edg_wll_FileTransferStatusToHTML(edg_wll_Context ctx UNUSED_VAR, edg_wll_JobStat stat, char **message)
{
        char *pomA = NULL, *pomB = NULL, *lbstat, *children, *header = NULL;
        int pomL = 0, i;
        char    *chid,*chcj,*chpar,*chsbt=NULL;

	children = strdup("");

        chid = edg_wlc_JobIdUnparse(stat.jobId);

	TR("Status", "%s", (lbstat = edg_wll_StatToString(stat.state)), NULL);
	free(lbstat);
	TR("Type","%s",edg_wll_StatusJobtypeNames[stat.jobtype], NULL);
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
	header = get_html_header(ctx, 0);
	asprintf(&pomA, "<HTML>\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<TITLE>Job Detail</TITLE>\n<HEAD>\n<HEAD>\n%s\n</HEAD>\n<body>\r\n"
                        "<h2>%s</h2>\r\n"
                        "<table halign=\"left\">%s</table>\r\n"
			"%s\n"
                        "\t</body>\r\n</html>",
			header,
                        chid, pomB, children);
        free(pomB);
	free(header);

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

	return 0;
}

int edg_wll_WSDLOutput(edg_wll_Context ctx UNUSED_VAR, char **message, char *filename){
	FILE *f;
	char *wsdl/*, *p, *start*/; // unused vars commented out
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

int edg_wll_StatisticsToHTML(edg_wll_Context ctx, char **message, int text) {
        char *out, *header = NULL;
	char* times[SERVER_STATISTICS_COUNT];
	int i;
	char *out_tmp;
	time_t *starttime;

	for (i = 0; i < SERVER_STATISTICS_COUNT; i++)
		if ((starttime = edg_wll_ServerStatisticsGetStart(ctx, i))) 
			if (text) asprintf(&(times[i]), "%ld", *starttime);
			else times[i] = strdup((const char*)ctime(starttime));
		else
			times[i] = NULL;

	header = get_html_header(ctx, text);
	if (!text) asprintf(&out, "<HTML>\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<TITLE>Server Usage Statistics</TITLE>\n<HEAD>%s\n</HEAD>\n<BODY>\n"
		"<h2>LB Server Usage Statistics</h2>\n"
		"%s"
		"<table halign=\"left\">\n"
		"<tr><td>Variable</td><td>Value</td><td>Measured from</td></tr>\n",
		header ? header : "",
		edg_wll_ServerStatisticsInTmp() ? "<b>WARNING: L&B statistics are stored in /tmp, please, configure L&B server to make them really persistent!</b><br/><br/>\n" : "");
	else
                out = strdup("");

	free(header);

	for (i = 0; i < SERVER_STATISTICS_COUNT; i++) {
		asprintf(&out_tmp, text ? "%s%s=%i,%s\n" : "%s<tr><td>%s</td><td>%i</td><td>%s</td></tr>\n", out,
			text ? edg_wll_server_statistics_type_key[i] : edg_wll_server_statistics_type_title[i],
			edg_wll_ServerStatisticsGetValue(ctx, i),
			times[i]);
		free(out);
		out = out_tmp;
	}

	if (!text) {
		asprintf(&out_tmp, "%s</table>\n</BODY>\n</HTML>", out);
		free(out);
		out = out_tmp;
	}

	for (i = 0; i < SERVER_STATISTICS_COUNT; i++)
		free(times[i]);

        *message = out;
	return 0;
}

int edg_wll_VMHostToHTML(edg_wll_Context ctx UNUSED_VAR, char *hostname, edg_wll_JobStat *states, char **message){
        char *pomA = NULL, *pomB = NULL;
	int i;

        if (states) for (i = 0; states[i].state != EDG_WLL_JOB_UNDEF; i++){
		char *status = edg_wll_VMStatToString(states[i].vm_state);
		char *chid = edg_wlc_JobIdUnparse(states[i].jobId);
		if (pomB)
			asprintf(&pomA, "%s\t\t <li> <a href=\"%s\">%s</a> (%s)\r\n",
				pomB, chid, chid, status);
		else
			asprintf(&pomA, "<li> <a href=\"%s\">%s</a> (%s)\r\n",
                                chid, chid, status);
		free(pomB);
		pomB = pomA;
                free(chid);
		free(status);
        }

	asprintf(&pomA, "<html>\r\n\t<body>\r\n"
                        "<h2>%s</h2>\r\n"
                        "<table halign=\"left\">%s</table>"
                        "\t</body>\r\n</html>",
                        hostname,pomB);
        free(pomB);

        *message = pomA;

        return 0;
}

char *edg_wll_ErrorToHTML(edg_wll_Context ctx,int code)
{
	char	*out,*et,*ed,*header = NULL;
	char	*msg = edg_wll_HTTPErrorMessage(code);
	edg_wll_ErrorCode	e;

	e = edg_wll_Error(ctx,&et,&ed);
	header = get_html_header(ctx, 0);
	asprintf(&out, "<HTML>\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<HEAD>\n<TITLE>Error</TITLE>\n%s\n</HEAD>\n"
		"<body><h1>%s</h1>\n"
		"%d: %s (%s)</body></html>",header,msg,e,et,ed);

	free(et); free(ed);
	return out;
}

