#include <stdsoap2.h>

#include "glite/lb/context-int.h"
#include "glite/lb/consumer.h"

#include "jobstat.h"
#include "query.h"
#include "ws_plugin.h"
#include "bk_ws_H.h"

int edgwll2__JobStatus(
        struct soap						   *soap,
        char							   *jobid,
        struct edgwll__JobStatFlags		   *flags,
        struct edgwll2__JobStatusResponse  *out)
{
	edg_wll_Context		ctx = (edg_wll_Context) soap_lookup_plugin(soap, PLUGIN_ID);
	edg_wlc_JobId		j;
	edg_wll_JobStat		s;


	out->status = soap_malloc(soap, sizeof *out->status);

	if ( edg_wlc_JobIdParse(jobid, &j) )
	{
		edg_wll_SetError(ctx, EINVAL, jobid);
		edg_wll_ErrToFault(ctx, soap);
		return SOAP_FAULT;
	}

	if ( edg_wll_JobStatus(ctx, j, 0, &s) )
	{
		edg_wll_ErrToFault(ctx, soap);
		return SOAP_FAULT;
	}

	edg_wll_StatusToSoap(soap, &s, out->status);

	return SOAP_OK;
}

int edgwll2__QueryJobs(
	struct soap						   *soap,
	struct edgwll__QueryConditions	   *conditions,
	struct edgwll__JobStatFlags		   *flags,
	struct edgwll2__QueryJobsResponse  *out)
{
	edg_wll_Context		ctx = (edg_wll_Context) soap_lookup_plugin(soap, PLUGIN_ID);
	edg_wlc_JobId	   *jobsOut = NULL;
	edg_wll_JobStat	   *statesOut = NULL;
	edg_wll_QueryRec  **qr = NULL;
	int					fl,
						i, j,
						ret = SOAP_FAULT;


	out->states = soap_malloc(soap, sizeof(*out->states));
	out->jobs = soap_malloc(soap, sizeof(*out->jobs));
	if ( !out->states || !out->jobs ) goto cleanup;
	memset(out->states, 0, sizeof(*(out->states)));
	memset(out->jobs, 0, sizeof(*(out->jobs)));

	edg_wll_ResetError(ctx);
	edg_wll_SoapToJobStatFlags(flags, &fl);
	if ( !edg_wll_SoapToQueryCondsExt(conditions, &qr) ) {
		edg_wll_SetError(ctx, ENOMEM, "Couldn't create internal structures");
		goto cleanup;
	}
	if ( edg_wll_QueryJobsServer(ctx, qr, fl, &jobsOut, &statesOut) ) goto cleanup;
	if ( edg_wll_JobsQueryResToSoap(jobsOut, statesOut, out) ) goto cleanup;
	ret = SOAP_OK;

cleanup:
	if ( qr ) {
		for ( i = 0; qr[i]; i++ ) {
			for ( j = 0; qr[i][j].attr; j++ )
				edg_wll_QueryRecFree(&qr[i][j]);
			free(qr[i]);
		}
		free(qr);
	}
	if ( jobsOut ) {
		for ( i = 0; jobsOut[i]; i++ )
			edg_wlc_JobIdFree(jobsOut[i]);
		free(jobsOut);
	}
	if ( statesOut ) {
		for ( i = 0; statesOut[i].state; i++ )
			edg_wll_FreeStatus(&statesOut[i]);
		free(statesOut);
	}
	if ( ret == SOAP_FAULT ) edg_wll_ErrToFault(ctx, soap);

	return ret;
}

int edgwll2__UserJobs(
	struct soap						   *soap,
	struct edgwll2__UserJobsResponse   *out)
{
	out->jobs = NULL;
	out->states = NULL;

	return SOAP_OK;
}
