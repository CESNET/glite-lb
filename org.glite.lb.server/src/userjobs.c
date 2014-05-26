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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "glite/jobid/cjobid.h"
#include "glite/jobid/strmd5.h"
#include "glite/lbu/trio.h"
#include "glite/lb/context-int.h"
#include "glite/lbu/log.h"

#include "jobstat.h"
#include "db_supp.h"
#include "query.h"
#include "lb_proto.h"

int edg_wll_UserJobsServer(
	edg_wll_Context ctx,
	int flags,
	edg_wlc_JobId	**jobs,
	edg_wll_JobStat	**states
)
{
	char	*userid, *stmt = NULL, *opt_userid,
		*res = NULL;
	char	*can_peername;
	char	*srv_name = NULL;
	int	name_len;
	int	njobs = 0,ret,i,idx;
	edg_wlc_JobId	*out = NULL;
	glite_lbu_Statement	sth = NULL;
	edg_wll_ErrorCode	err = 0;

	edg_wll_ResetError(ctx);
	
	if (!ctx->peerName) {
		return edg_wll_SetError(ctx,EPERM, "user not authenticated (edg_wll_UserJobs)");
	}
	can_peername = edg_wll_gss_normalize_subj(ctx->peerName, 0);
	userid = strmd5(can_peername,NULL);
	free(can_peername);

	trio_asprintf(&stmt,"select cert_subj from users where userid = '%|Ss'",userid);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	switch (edg_wll_ExecSQL(ctx,stmt,&sth)) {
		case 0: edg_wll_SetError(ctx,ENOENT,ctx->peerName);
		case -1: goto err;
		default:
			if (edg_wll_FetchRow(ctx,sth,1,NULL,&res) < 0) goto err;
			if (!edg_wll_gss_equal_subj(ctx->peerName,res)) {
				edg_wll_SetError(ctx,EDG_WLL_ERROR_MD5_CLASH,ctx->peerName);
				goto err;
			}
	}

	glite_lbu_FreeStmt(&sth);
	free(stmt); stmt = NULL;
	free(res); res = NULL;

	trio_asprintf (&opt_userid,"userid = '%|Ss'", userid);

	//Alternative identities
	if(ctx->id_mapping.num) {
		int i;
		char *alter, *opt_tmp;

		for (i = 0; i< ctx->id_mapping.num; i++) {
			alter = NULL;
			if (edg_wll_gss_equal_subj(ctx->peerName, ctx->id_mapping.rules[i].a))
				alter = strmd5(ctx->id_mapping.rules[i].b, NULL);
			else if (edg_wll_gss_equal_subj(ctx->peerName, ctx->id_mapping.rules[i].b))
				alter = strmd5(ctx->id_mapping.rules[i].a, NULL);
			if (alter) {
				trio_asprintf(&opt_tmp, "%s OR userid = '%|Ss'", opt_userid, alter);
				free(opt_userid);
				opt_userid = opt_tmp;
			}
		}
	}

	trio_asprintf(&stmt,"select dg_jobid from jobs where (%s) and grey='0'",opt_userid);
	free(opt_userid);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
	switch (njobs = edg_wll_ExecSQL(ctx,stmt,&sth)) {
		case 0: edg_wll_SetError(ctx,ENOENT,ctx->peerName);
		case -1: goto err;
	}

	out = malloc(sizeof(*out)*(njobs+1));
	memset(out,0,sizeof(*out)*(njobs+1));
	
	name_len = asprintf(&srv_name, "https://%s:%d/", 
		ctx->srvName, ctx->srvPort);
	idx = 0;
	for (i=0; (ret = edg_wll_FetchRow(ctx,sth,1,NULL,&res)); i++) {
		if (ret < 0) goto err;
		if (strncmp(res, srv_name, name_len)){
			njobs--;
			free(res);
			continue;
		}
		if ((ret = edg_wlc_JobIdParse(res,out+idx))) {
			edg_wll_SetError(ctx,errno,res);
			goto err;
		}
		idx++;
		free(res); res = NULL;
	}
	free(srv_name);

	if (states) {
		edg_wll_QueryRec	oc[2],*ocp[2] = { oc, NULL };

		memset(oc, 0, sizeof(oc));
		oc[0].attr = EDG_WLL_QUERY_ATTR_OWNER;
		oc[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

		if (check_job_query_index(ctx,(const edg_wll_QueryRec **)ocp)) {
			edg_wll_ResetError(ctx);
			*states = NULL;
			goto err;
		}

		*states = calloc(njobs+1, sizeof(**states));
		idx = 0;
		for (i = 0; i < njobs; i++) {
			if (edg_wll_JobStatusServer(ctx, out[idx], flags, &(*states)[idx]) != 0)
				edg_wll_ResetError(ctx);
			idx++;
		}
	}
err:
	free(res);
	free(stmt);
	glite_lbu_FreeStmt(&sth);
	if ((err = edg_wll_Error(ctx,NULL,NULL))) {
		if (out) {
		    for (i=0; i<njobs; i++)
			edg_wlc_JobIdFree(out[i]);
		    free(out);
		}
	} else *jobs = out;

	return err;
}

