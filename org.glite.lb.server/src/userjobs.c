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

#include "glite/wmsutils/jobid/cjobid.h"
#include "glite/wmsutils/jobid/strmd5.h"
#include "glite/lb/context-int.h"
#include "glite/lb/trio.h"

#include "lbs_db.h"

int edg_wll_UserJobs(
	edg_wll_Context ctx,
	edg_wlc_JobId	**jobs,
	edg_wll_JobStat	**states)
{
	char	*userid, *stmt = NULL,
		*res = NULL;
	char	*can_peername;
	int	njobs = 0,ret,i;
	edg_wlc_JobId	*out = NULL;
	edg_wll_Stmt	sth = NULL;
	edg_wll_ErrorCode	err = 0;

	edg_wll_ResetError(ctx);
	
	if (!ctx->peerName) {
		return edg_wll_SetError(ctx,EPERM, "user not authenticated (edg_wll_UserJobs)");
	}
	can_peername = edg_wll_gss_normalize_subj(ctx->peerName, 0);
	userid = strmd5(can_peername,NULL);
	free(can_peername);

	trio_asprintf(&stmt,"select cert_subj from users where userid = '%|Ss'",userid);

	switch (edg_wll_ExecStmt(ctx,stmt,&sth)) {
		case 0: edg_wll_SetError(ctx,ENOENT,ctx->peerName);
		case -1: goto err;
		default:
			if (edg_wll_FetchRow(sth,&res) < 0) goto err;
			if (!edg_wll_gss_equal_subj(ctx->peerName,res)) {
				edg_wll_SetError(ctx,EDG_WLL_ERROR_MD5_CLASH,ctx->peerName);
				goto err;
			}
	}

	edg_wll_FreeStmt(&sth);
	free(stmt); stmt = NULL;
	free(res); res = NULL;

	trio_asprintf(&stmt,"select dg_jobid from jobs where userid = '%|Ss'",userid);
	switch (njobs = edg_wll_ExecStmt(ctx,stmt,&sth)) {
		case 0: edg_wll_SetError(ctx,ENOENT,ctx->peerName);
		case -1: goto err;
	}

	out = malloc(sizeof(*out)*(njobs+1));
	memset(out,0,sizeof(*out)*(njobs+1));
	
	for (i=0; (ret = edg_wll_FetchRow(sth,&res)); i++) {
		if (ret < 0) goto err;
		if ((ret = edg_wlc_JobIdParse(res,out+i))) {
			edg_wll_SetError(ctx,errno,res);
			goto err;
		}
		free(res); res = NULL;
	}

err:
	free(res);
	free(stmt);
	edg_wll_FreeStmt(&sth);
	if ((err = edg_wll_Error(ctx,NULL,NULL))) {
		if (out) {
		    for (i=0; i<njobs; i++)
			edg_wlc_JobIdFree(out[i]);
		    free(out);
		}
	} else *jobs = out;

	return err;
}
