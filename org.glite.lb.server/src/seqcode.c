#ident "$Header$"

#include <errno.h>

#include "glite/lbu/trio.h"
#include "glite/lb/context-int.h"

#include "get_events.h"
#include "store.h"
#include "jobstat.h"
/*
#include "lb_authz.h"
*/
#include "db_supp.h"
#include "lb_proto.h"


int edg_wll_QuerySequenceCodeServer(edg_wll_Context ctx, edg_wlc_JobId jobid, const char *source, char **seqcode)
{
	glite_lbu_Statement	sh;
	intJobStat	   *istat = NULL;
	char		   *jobid_md5 = NULL,
				   *stmt = NULL,
				   *res = NULL,
				   *res_rest = NULL;
	int				nstates;


	edg_wll_ResetError(ctx);
	jobid_md5 = edg_wlc_JobIdGetUnique(jobid);

	trio_asprintf(&stmt,
				"select int_status from states "
				"where jobid='%|Ss' and version='%|Ss'",
				jobid_md5, INTSTAT_VERSION);

	if ( stmt == NULL ) return edg_wll_SetError(ctx, ENOMEM, NULL);

	if ( (nstates = edg_wll_ExecSQL(ctx, stmt, &sh)) < 0 ) goto cleanup;
	if ( nstates == 0 ) {
		edg_wll_SetError(ctx, ENOENT, "no state in DB");
		goto cleanup;
	}
	if ( edg_wll_FetchRow(ctx, sh, 1, NULL, &res) < 0 ) goto cleanup;

	istat = dec_intJobStat(res, &res_rest);
	if ( res_rest  && istat ) {
		*seqcode = istat->last_seqcode;
		istat->last_seqcode = NULL;
	}
	else edg_wll_SetError(ctx, EDG_WLL_ERROR_DB_CALL,
						"error decoding DB intJobStatus");

cleanup:
	free(res);
	free(jobid_md5);
	free(stmt);
	glite_lbu_FreeStmt(&sh);
	if ( istat ) {
		destroy_intJobStat(istat);
		free(istat);
	}

	return edg_wll_Error(ctx, NULL, NULL);
}
