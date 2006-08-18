#ident "$Header$"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "glite/lb-utils/db.h"
#include "glite/wmsutils/jobid/cjobid.h"
#include "glite/wmsutils/jobid/strmd5.h"
#include "glite/lb/context-int.h"
#include "glite/lb/trio.h"

#include "db_supp.h"

int edg_wll_UserJobs(
	edg_wll_Context ctx,
	edg_wlc_JobId	**jobs,
	edg_wll_JobStat	**states)
{
	char	*userid = strmd5(ctx->peerName,NULL),*stmt = NULL,
		*res = NULL;
	int	njobs = 0,ret,i;
	edg_wlc_JobId	*out = NULL;
	glite_lbu_Statement	sth = NULL;
	edg_wll_ErrorCode	err = 0;

	edg_wll_ResetError(ctx);

	trio_asprintf(&stmt,"select cert_subj from users where userid = '%|Ss'",userid);

	switch (glite_lbu_ExecSQL(ctx->dbctx,stmt,&sth)) {
		case 0: edg_wll_SetError(ctx,ENOENT,ctx->peerName);
		case -1: edg_wll_SetErrorDB(ctx); goto err;
		default:
			if (glite_lbu_FetchRow(sth,1,NULL,&res) < 0) {
				edg_wll_SetErrorDB(ctx);
				goto err;
			}
			if (strcmp(ctx->peerName,res)) {
				edg_wll_SetError(ctx,EDG_WLL_ERROR_MD5_CLASH,ctx->peerName);
				goto err;
			}
	}

	glite_lbu_FreeStmt(&sth);
	free(stmt); stmt = NULL;
	free(res); res = NULL;

	trio_asprintf(&stmt,"select dg_jobid from jobs where userid = '%|Ss'",userid);
	switch (njobs = glite_lbu_ExecSQL(ctx->dbctx,stmt,&sth)) {
		case 0: edg_wll_SetError(ctx,ENOENT,ctx->peerName);
		case -1: edg_wll_SetErrorDB(ctx); goto err;
	}

	out = malloc(sizeof(*out)*(njobs+1));
	memset(out,0,sizeof(*out)*(njobs+1));
	
	for (i=0; (ret = glite_lbu_FetchRow(sth,1,NULL,&res)); i++) {
		if (ret < 0) {
			edg_wll_SetErrorDB(ctx);
			goto err;
		}
		if ((ret = edg_wlc_JobIdParse(res,out+i))) {
			edg_wll_SetError(ctx,errno,res);
			goto err;
		}
		free(res); res = NULL;
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
