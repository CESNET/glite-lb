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


#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "glite/lb/context-int.h"
#include "glite/lbu/trio.h"
#include "glite/lbu/log.h"

#include "lb_authz.h"
#include "lb_xml_parse.h"
#include "query.h"
#include "il_notification.h"
#include "db_supp.h"
#include "index.h"
#include "authz_policy.h"

static int notif_match_conditions(edg_wll_Context,const edg_wll_JobStat *,const edg_wll_JobStat *,const char *, int flags);

int edg_wll_NotifExpired(edg_wll_Context,const char *);

int edg_wll_NotifMatch(edg_wll_Context ctx, const edg_wll_JobStat *oldstat, const edg_wll_JobStat *stat)
{
	edg_wll_NotifId		nid = NULL;
	char	*jobq,*ju = NULL,*jobc[6];
	glite_lbu_Statement	jobs = NULL;
	int	ret,authz_flags = 0;
	size_t i;
	time_t	expires,now = time(NULL);
	
	char *cond_where = NULL;
	char *cond_and_where = NULL;

	edg_wll_ResetError(ctx);

	if (ctx->notif_index) {
		edg_wll_IColumnRec *notif_index_cols = ctx->notif_index_cols;

		for (i=0; notif_index_cols[i].qrec.attr; i++) {
			char	*val = NULL;

			if (notif_index_cols[i].qrec.attr != EDG_WLL_QUERY_ATTR_JDL_ATTR) {
				val = edg_wll_StatToSQL(stat,notif_index_cols[i].qrec.attr);
				assert(val != (char *) -1);
			}
			else { // Special Treatment for JDL attributes
				val = edg_wll_JDLStatToSQL(stat,notif_index_cols[i].qrec);
			} 

			if (val) {
				char	*aux;
				if (!cond_where) cond_where = strdup("");
				trio_asprintf(&aux, "%s or %s = %s",cond_where,
						notif_index_cols[i].colname,val);
				free(cond_where);
				cond_where = aux;
				free(val);
			}
			else if (notif_index_cols[i].qrec.attr == EDG_WLL_QUERY_ATTR_JDL_ATTR) {
				char	*aux;
				if (!cond_and_where) cond_and_where = strdup("");
				trio_asprintf(&aux, "%s AND %s is NULL",cond_and_where,
						notif_index_cols[i].colname);
				free(cond_and_where);
				cond_and_where = aux;
				free(val);
			}
		}
	}

	if ( (ret = edg_wll_NotifIdCreate(ctx->srvName, ctx->srvPort, &nid)) )
	{
		edg_wll_SetError(ctx, ret, "edg_wll_NotifMatch()");
		goto err;
	}

	trio_asprintf(&jobq,
		"select distinct n.notifid,n.destination,n.valid,u.cert_subj,n.conditions,n.flags "
		"from notif_jobs j,users u,notif_registrations n "
		"where j.notifid=n.notifid and n.userid=u.userid "
		"   and (j.jobid = '%|Ss' or j.jobid = '%|Ss' %s) %s",
		ju = edg_wlc_JobIdGetUnique(stat->jobId),NOTIF_ALL_JOBS,cond_where ? cond_where : "",cond_and_where ? cond_and_where : "");

	free(ju); ju = NULL;
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, jobq);

	if (edg_wll_ExecSQL(ctx,jobq,&jobs) < 0) goto err;

	while ((ret = edg_wll_FetchRow(ctx,jobs,sizeof(jobc)/sizeof(jobc[0]),NULL,jobc)) > 0) {
		if (now > (expires = glite_lbu_StrToTime(jobc[2]))) {
			edg_wll_NotifExpired(ctx,jobc[0]);
			glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG, "[%d] NOTIFY:%s expired at %s UTC", 
				getpid(),jobc[0],asctime(gmtime(&expires)));
		}
		else if (notif_match_conditions(ctx,oldstat,stat,jobc[4],atoi(jobc[5])) &&
				edg_wll_NotifCheckACL(ctx,stat,jobc[3], &authz_flags))
		{
			char			   *dest;

			glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG, "NOTIFY: %s, job %s", jobc[0], ju = edg_wlc_JobIdGetUnique(stat->jobId));
			free(ju); ju = NULL;

			dest = jobc[1];
			
			if (   edg_wll_NotifIdSetUnique(&nid, jobc[0]) )
			{
				free(dest);
				goto err;
			}
			/* XXX: only temporary hack!!!
			 */
			ctx->p_instance = strdup("");
			if ( edg_wll_NotifJobStatus(ctx, nid, dest, jobc[3], atoi(jobc[5]), authz_flags, expires, *stat) )
			{
				for (i=0; i<sizeof(jobc)/sizeof(jobc[0]); i++) free(jobc[i]);
				goto err;
			}
		}
		
		for (i=0; i<sizeof(jobc)/sizeof(jobc[0]); i++) free(jobc[i]);
	}
	if (ret < 0) goto err;
	
err:
	free(ctx->p_instance); ctx->p_instance = NULL;
	if ( nid ) edg_wll_NotifIdFree(nid);
	free(jobq);
	glite_lbu_FreeStmt(&jobs);
	return edg_wll_Error(ctx,NULL,NULL);
}

int edg_wll_NotifExpired(edg_wll_Context ctx,const char *notif)
{
	char	*dn = NULL,*dj = NULL;

	trio_asprintf(&dn,"delete from notif_registrations where notifid='%|Ss'",notif);
	trio_asprintf(&dj,"delete from notif_jobs where notifid='%|Ss'",notif);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, dn);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, dj);

	if (edg_wll_ExecSQL(ctx,dn,NULL) < 0 ||
		edg_wll_ExecSQL(ctx,dj,NULL) < 0)
	{
		char	*et,*ed;
		edg_wll_Error(ctx,&et,&ed);

		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, 
			"delete notification %s: %s (%s)", notif, et, ed);		
		free(et); free(ed);
	}

	free(dn);
	free(dj);
	return edg_wll_ResetError(ctx);
}


static int notif_match_conditions(edg_wll_Context ctx,const edg_wll_JobStat *oldstat, const edg_wll_JobStat *stat,const char *cond, int flags)
{
	edg_wll_QueryRec	**c,**p;
	int			match = 0,i;

	if (!cond) return 1;

	if (!(flags & EDG_WLL_NOTIF_TERMINAL_STATES) || // Either there is no terminal flag
		((flags & EDG_WLL_NOTIF_TERMINAL_STATES) && (EDG_WLL_JOB_TERMINAL_STATE[stat->state]) && // Or the new state is terminal
			((stat->state!=EDG_WLL_JOB_PURGED) || !(EDG_WLL_JOB_TERMINAL_STATE[oldstat->state])))) { // And in case it is purged, it was not in a terminal state before

		if (parseJobQueryRec(ctx,cond,strlen(cond),&c)) {
			glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, 
				"notif_match_conditions(): parseJobQueryRec failed");

			return 1;
		}

		match = match_status(ctx,oldstat,stat,(const edg_wll_QueryRec **) c);
		if ( c )
		{
			for (p = c; *p; p++) {
				for (i=0; (*p)[i].attr; i++)
					edg_wll_QueryRecFree((*p)+i);
				free(*p);
			}
			free(c);
		}
	}
	return match;
}

/* FIXME: does not favour any VOMS information in ACL
 * effective VOMS groups of the recipient are not available here, should be 
 * probably stored along with the registration.
 */
int edg_wll_NotifCheckACL(edg_wll_Context ctx,const edg_wll_JobStat *stat,const char *recip, int *authz_flags)
{
	int		ret;
	struct _edg_wll_GssPrincipal_data princ;
	edg_wll_Acl	acl = NULL;

	memset(&princ, 0, sizeof(princ));
	*authz_flags = 0;

	edg_wll_ResetError(ctx);
	if (strcmp(stat->owner,recip) == 0
		|| edg_wll_amIroot(recip,NULL,&ctx->authz_policy)) return 1;
	if (stat->payload_owner && strcmp(stat->payload_owner,recip) == 0)
		return 1;
	princ.name = (char *)recip;
	if (check_authz_policy(&ctx->authz_policy, &princ, READ_ALL))
		return 1;

	if (stat->acl) {
		acl =  calloc(1,sizeof *acl);
		ret = edg_wll_DecodeACL(stat->acl,&acl->value);
		if (ret) {
			edg_wll_FreeAcl(acl);
			edg_wll_SetError(ctx,EINVAL,"decoding ACL");
			return 0;
		}

		acl->string = stat->acl; 
		ret = edg_wll_CheckACL(ctx, acl, EDG_WLL_CHANGEACL_READ);
		acl->string = NULL;
		edg_wll_FreeAcl(acl);
		if (ret == 0)
			return 1;
		edg_wll_ResetError(ctx);
	}

	if (check_authz_policy(&ctx->authz_policy, &princ, STATUS_FOR_MONITORING)) {
		*authz_flags |= STATUS_FOR_MONITORING;
                return 1;
	}

	return 0;
}
