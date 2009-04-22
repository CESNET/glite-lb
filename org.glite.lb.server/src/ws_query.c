#include <stdsoap2.h>
#include <expat.h>

#include "glite/lb/context-int.h"
#include "glite/lb/xml_parse.h"
#include "glite/lb/xml_conversions.h"


#include "soap_version.h"
#include "glite/security/glite_gsplugin.h"
#include "glite/security/glite_gscompat.h"

#include "jobstat.h"
#include "query.h"
#include "bk_ws_H.h"
#include "get_events.h"
#include "ws_fault.h"
#include "ws_typeref.h"
#include "lb_proto.h"

#if GSOAP_VERSION <= 20602
#define __lb__GetVersion __ns1__GetVersion
#define __lb__JobStatus __ns1__JobStatus
#define __lb__UserJobs __ns1__UserJobs
#define __lb__QueryJobs __ns1__QueryJobs
#define __lb__QueryEvents __ns1__QueryEvents
#endif

extern int debug;
#define dprintf(x) if (debug) printf x

static void freeQueryRecsExt(edg_wll_QueryRec **qr);
static void freeJobIds(edg_wlc_JobId *jobs);
static void freeJobStats(edg_wll_JobStat *stats);
static void freeEvents(edg_wll_Event *events);


SOAP_FMAC5 int SOAP_FMAC6 __lb__GetVersion(
	struct soap* soap,
	struct _lbe__GetVersion *in,
	struct _lbe__GetVersionResponse *out)
{
	dprintf(("[%d] WS call %s\n",getpid(),__FUNCTION__));

	out->version = soap_strdup(soap, VERSION);

	return out->version ? SOAP_OK : SOAP_FAULT;
}


SOAP_FMAC5 int SOAP_FMAC6 __lb__JobStatus(
	struct soap	*soap,
	struct _lbe__JobStatus *in,
	struct _lbe__JobStatusResponse *out)
{
	edg_wll_Context		ctx = (edg_wll_Context) glite_gsplugin_get_udata(soap);
	edg_wlc_JobId		j;
	edg_wll_JobStat		s;
	int	flags;


	dprintf(("[%d] WS call %s\n",getpid(),__FUNCTION__));

	if ( edg_wlc_JobIdParse(in->jobid, &j) )
	{
		edg_wll_SetError(ctx, EINVAL, in->jobid);
		edg_wll_ErrToFault(ctx, soap);
		return SOAP_FAULT;
	}

	edg_wll_SoapToJobStatFlags(in->flags, &flags);
	
	if (debug) {
		char *cjobid = NULL, *cflags = NULL;

		cjobid = edg_wlc_JobIdUnparse(j);
		cflags = edg_wll_stat_flags_to_string(flags);
		dprintf(("[%d] \n\t<flags>%s</flags>\n\t<jobId>%s</jobId>\n\n",getpid(),cflags,cjobid));
		free(cjobid);
		free(cflags);
	}

	if ( edg_wll_JobStatusServer(ctx, j, flags, &s) )
	{
		edg_wll_ErrToFault(ctx, soap);
		return SOAP_FAULT;
	}

	out->stat = soap_malloc(soap, sizeof(*out->stat));
	edg_wll_StatusToSoap(soap, &s, out->stat);

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __lb__QueryJobs(
	struct soap *soap,
	struct _lbe__QueryJobs *in,
	struct _lbe__QueryJobsResponse *out)
{
	edg_wll_Context    ctx;
	edg_wll_QueryRec **conditions;
	int                flags;
	edg_wlc_JobId	  *jobs;
	edg_wll_JobStat	  *states;
	int                ret = SOAP_FAULT;


	dprintf(("[%d] WS call %s\n",getpid(),__FUNCTION__));

	out->states = soap_malloc(soap, sizeof(*out->states));
	out->jobs = soap_malloc(soap, sizeof(*out->jobs));
	if ( !out->states || !out->jobs ) goto cleanup;

	ctx = (edg_wll_Context) glite_gsplugin_get_udata(soap);
	jobs = NULL;
	states = NULL;

	edg_wll_ResetError(ctx);
	if ( edg_wll_SoapToQueryCondsExt(in->conditions, in->__sizeconditions, &conditions) ) {
		edg_wll_SetError(ctx, ENOMEM, "Couldn't create internal structures");
		goto err;
	}
	edg_wll_SoapToJobStatFlags(in->flags, &flags);

	if (debug) {
		char *message = NULL;

		if (edg_wll_QueryJobsRequestToXML(ctx, 
				(const edg_wll_QueryRec **) conditions, 
				flags, &message)) {
			dprintf(("[%d] %s\n",getpid(),"edg_wll_QueryJobsRequestToXML() returned error"));
		}
		else {
			dprintf(("[%d] \n%s\n\n",getpid(),message));
		}
		free(message);
	}

	if (edg_wll_QueryJobsServer(ctx, (const edg_wll_QueryRec **)conditions, flags, &jobs, &states) != 0) goto err;

	if (edg_wll_JobsQueryResToSoap(soap, jobs, states, out) != SOAP_OK) goto err;
	ret = SOAP_OK;

err:
	if ( ret == SOAP_FAULT ) edg_wll_ErrToFault(ctx, soap);
cleanup:
	freeQueryRecsExt(conditions);
	freeJobIds(jobs);
	freeJobStats(states);

	return ret;
}


SOAP_FMAC5 int SOAP_FMAC6 __lb__UserJobs(
	struct soap *soap,
	struct _lbe__UserJobs *in,
	struct _lbe__UserJobsResponse *out)
{
	edg_wll_Context ctx;
	edg_wlc_JobId	*jobs;
	edg_wll_JobStat	*states;


	dprintf(("[%d] WS call %s\n",getpid(),__FUNCTION__));

	ctx = (edg_wll_Context) glite_gsplugin_get_udata(soap);
	memset(out, 0, sizeof *out);
	if (edg_wll_UserJobsServer(ctx, &jobs, &states) != 0) goto fault;
	if (edg_wll_UserJobsResToSoap(soap, jobs, states, out) != SOAP_OK) {
		edg_wll_SetError(ctx, ENOMEM, "Couldn't create internal structures");
		goto freefault;
	}
	freeJobIds(jobs);
	freeJobStats(states);
	return SOAP_OK;
freefault:
	freeJobIds(jobs);
	freeJobStats(states);
fault:
	edg_wll_ErrToFault(ctx, soap);
	return SOAP_FAULT;
}


SOAP_FMAC5 int SOAP_FMAC6 __lb__QueryEvents(
	struct soap *soap,
	struct _lbe__QueryEvents *in,
	struct _lbe__QueryEventsResponse *out)
{
	edg_wll_Context		ctx = (edg_wll_Context) glite_gsplugin_get_udata(soap);
	edg_wll_QueryRec 	**job_conditions;
	edg_wll_QueryRec 	**event_conditions;
	edg_wll_Event 		*events = NULL;
	int			ret = SOAP_OK;


	dprintf(("[%d] WS call %s\n",getpid(),__FUNCTION__));

	edg_wll_ResetError(ctx);
	if ( edg_wll_SoapToQueryCondsExt(in->jobConditions, in->__sizejobConditions, 
		&job_conditions) )
	{
		edg_wll_SetError(ctx, ENOMEM, "Couldn't create internal structures");
		edg_wll_ErrToFault(ctx, soap);
		ret = SOAP_FAULT;
		goto cleanup;
	}

	if ( edg_wll_SoapToQueryCondsExt(in->eventConditions, in->__sizeeventConditions, 
		&event_conditions) )
	{
		edg_wll_SetError(ctx, ENOMEM, "Couldn't create internal structures");
		edg_wll_ErrToFault(ctx, soap);
		ret = SOAP_FAULT;
		goto cleanup;
	}

	if (debug) {
		char *message = NULL;

		if (edg_wll_QueryEventsRequestToXML(ctx, 
				(const edg_wll_QueryRec **) job_conditions, 
				(const edg_wll_QueryRec **) event_conditions,
				&message)) {
			dprintf(("[%d] %s\n",getpid(),"edg_wll_QueryEventsRequestToXML() returned error"));
		}
		else {
			dprintf(("[%d] \n%s\n\n",getpid(),message));
		}
		free(message);
	}

	if (edg_wll_QueryEventsServer(ctx, ctx->noAuth, 
        	(const edg_wll_QueryRec **)job_conditions,
		(const edg_wll_QueryRec **)event_conditions,
        	&events))
	{
		edg_wll_ErrToFault(ctx, soap);
		ret = SOAP_FAULT;
		goto cleanup;
	}

	if (edg_wll_EventsQueryResToSoap(soap, events, out) != SOAP_OK) 
	{
		ret = SOAP_FAULT;
		goto cleanup;
	}

cleanup:
	freeQueryRecsExt(job_conditions);
	freeQueryRecsExt(event_conditions);
	freeEvents(events);

	return ret;
}

SOAP_FMAC5 int SOAP_FMAC6 __lb__NotifNew(
	struct soap *soap,
	struct _lbe__NotifNew *in,
	struct _lbe__NotifNewResponse *out
) {
	edg_wll_Context		ctx = (edg_wll_Context) glite_gsplugin_get_udata(soap);
	edg_wll_QueryRec 	**conditions = NULL;
	int			flags;
	edg_wll_NotifId		nid = NULL;
	int			ret = SOAP_OK;

	dprintf(("[%d] WS call %s\n",getpid(),__FUNCTION__));

	edg_wll_ResetError(ctx);
	if ( edg_wll_SoapToQueryCondsExt(in->conditions, in->__sizeconditions, &conditions) )
	{
		edg_wll_SetError(ctx, ENOMEM, "Couldn't create internal structures");
		edg_wll_ErrToFault(ctx, soap);
		ret = SOAP_FAULT;
		goto cleanup;
	}
	edg_wll_SoapToJobStatFlags(in->flags, &flags);

	if (edg_wll_NotifIdParse(in->notifId,&nid)) {
		edg_wll_SetError(ctx,EINVAL,"Parse notifid");
		edg_wll_ErrToFault(ctx, soap);
		ret = SOAP_FAULT;
		goto cleanup;
	}

	out->valid = in->valid ? *in->valid : 0;
	if (edg_wll_NotifNewServer(ctx,conditions,flags,in->destination,nid,&out->valid)) {
		edg_wll_ErrToFault(ctx, soap);
		ret = SOAP_FAULT;
		goto cleanup;
	}


cleanup:
	edg_wll_NotifIdFree(nid);
	freeQueryRecsExt(conditions);
	return ret;
}


static void freeQueryRecsExt(edg_wll_QueryRec **qr) {
	int i, j;

	if ( qr ) {
		for ( i = 0; qr[i]; i++ ) {
			if (qr[i]) {
				for ( j = 0; qr[i][j].attr; j++ ) edg_wll_QueryRecFree(&qr[i][j]);
				free(qr[i]);
			}
		}
		free(qr);
	}
}


static void freeJobIds(edg_wlc_JobId *jobs) {
	int i;

	if ( jobs ) {
		for ( i = 0; jobs[i]; i++ )
			edg_wlc_JobIdFree(jobs[i]);
		free(jobs);
	}
}


static void freeJobStats(edg_wll_JobStat *stats) {
	int i;

	if ( stats ) {
		for ( i = 0; stats[i].state; i++ )
			edg_wll_FreeStatus(&stats[i]);
		free(stats);
	}
}


static void freeEvents(edg_wll_Event *events)
{
	int i;

	if (events != NULL) {
		for (i=0; events[i].type != EDG_WLL_EVENT_UNDEF; i++)
			edg_wll_FreeEvent(&(events[i]));
		edg_wll_FreeEvent(&(events[i])); /* free last line */
		free(events);
	}
}
