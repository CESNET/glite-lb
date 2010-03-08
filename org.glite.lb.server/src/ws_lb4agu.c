#include <stdsoap2.h>

#include "soap_version.h"
#include "glite/security/glite_gsplugin.h"
#include "glite/security/glite_gscompat.h"

#include "glite/lbu/log.h"
#include "glite/lb/context-int.h"
#include "glite/lb/xml_conversions.h"

#include "bk_ws_Stub.h"
#include "bk_ws_H.h"
#include "ws_fault.h"
#include "ws_typeref.h"
#include "jobstat.h"

extern int debug;

#define LB_GLUE_STATE_PREFIX "urn:org.glite.lb"

/**
 * __lb4agu__GetActivityStatus - return states of given jobs (list of jobIds)
 * \param[in,out] soap	soap to work with
 * \param[in] in	list of jobIds
 * \param[out] out	list of job states for each jobId
 * \returns SOAP_OK or SOAP_FAULT
 * TODO: distinguish also faults
 * 	UNKNOWNACTIVITYID
 *	UNABLETORETRIEVESTATUS
 */
SOAP_FMAC5 int SOAP_FMAC6 __lb4agu__GetActivityStatus(
	struct soap* soap,
	struct _lb4ague__GetActivityStatusRequest *in,
	struct _lb4ague__GetActivityStatusResponse *out)
{
	edg_wll_Context	ctx = (edg_wll_Context) glite_gsplugin_get_udata(soap);
	edg_wlc_JobId	j;
	edg_wll_JobStat	s;
	int		i,flags;

	glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_DEBUG,
		"[%d] WS call %s", getpid(), __FUNCTION__);

	if (!in) return SOAP_FAULT;
	if (!in->id) return SOAP_FAULT;

	out->status = soap_malloc(soap, in->__sizeid * (sizeof(char *)));

	/* process each request individually: */
	for (i=0; i<in->__sizeid && in->id[i]; i++) {
		char	buf[1000],*stat = NULL;

		/* first parse jobId */
		if ( edg_wlc_JobIdParse(in->id[i], &j) ) {
			edg_wll_SetError(ctx, EINVAL, in->id[i]);
			edg_wll_ErrToFault(ctx, soap);
			return SOAP_FAULT;
		}

		/* FIXME: what are the flags? */
		flags = 0;
		if (debug) {
			char *cjobid = NULL, *cflags = NULL;

			cjobid = edg_wlc_JobIdUnparse(j);
			cflags = edg_wll_stat_flags_to_string(flags);
			glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST,
				LOG_PRIORITY_DEBUG,
				"[%d] \n\t<flags>%s</flags>\n\t<jobId>%s</jobId>\n",
				getpid(), cflags, cjobid);
			free(cjobid);
			free(cflags);
		}

		/* get job status */
		if ( edg_wll_JobStatusServer(ctx, j, flags, &s) ) {
			edg_wll_ErrToFault(ctx, soap);
			return SOAP_FAULT;
		}

		/* fill in the response fields */
		snprintf(buf,sizeof buf,LB_GLUE_STATE_PREFIX ":%s",stat = edg_wll_StatToString(s.state));
		buf[sizeof(buf) - 1] = 0;
		out->status[i] = soap_strdup(soap,buf);
		free(stat);
	}
	out->__sizestatus = in->__sizeid;

	return SOAP_OK;
}

/**
 * edg_wll_JobStatusToGlueComputingActivity - edg_wll_JobStat to glue__ComputingActivity_USCOREt
 * \param[in,out] soap	soap to work with
 * \param[in] src	job status in the LB native (edg_wll_JobStat) structure
 * \param[out] js	job status in the Glue2 soap (glue__ComputingActivity_USCOREt) structure
 * \returns SOAP_OK or SOAP_FAULT
 */
static int edg_wll_JobStatusToGlueComputingActivity(
	struct soap *soap,
	edg_wll_JobStat const *src,
	struct glue__ComputingActivity_USCOREt *js)
{
        //int     i,j;
        char    buf[1000],*s = NULL;

	memset(js, 0, sizeof(*js));
	// ID (required, glue:ID_t) = jobId
	s = edg_wlc_JobIdUnparse(src->jobId); 
	js->ID = soap_strdup(soap,s); 
	free(s); s=NULL;

	// BaseType (required, xsd:string) = type of job?
	// how does it differ from Type?
	js->BaseType = soap_lbt__RegJobJobtype2s(soap,src->jobtype);

	// CreationTime (optional, xsd:dateTime) = submission time?
	js->CreationTime = src->stateEnterTimes[1+EDG_WLL_JOB_SUBMITTED];

	// Validity (optional, xsd:unsignedLong) = ?
	js->Validity = 0;

	// Name (optional, xsd:string) = ?
	js->Name = NULL;

	// Type (optional, glue:ComputingActivityType_t) = jobtype ?
	// see glue__ComputingActivityType_USCOREt
	switch (src->jobtype) {
		case EDG_WLL_STAT_SIMPLE:
			js->Type = glue__ComputingActivityType_USCOREt__single;
			break;
		case EDG_WLL_STAT_DAG:
			js->Type = glue__ComputingActivityType_USCOREt__parallelelement;
			break;
		case EDG_WLL_STAT_COLLECTION:
			js->Type = glue__ComputingActivityType_USCOREt__collectionelement;
			break;
		case EDG_WLL_STAT_PBS:
		case EDG_WLL_STAT_CONDOR:
		case EDG_WLL_STAT_CREAM:
		case EDG_WLL_STAT_FILE_TRANSFER:
		case EDG_WLL_STAT_FILE_TRANSFER_COLLECTION:
			js->Type = glue__ComputingActivityType_USCOREt__workflownode;
			break;
		default:
			js->Type = glue__ComputingActivityType_USCOREt__single;
			break;
	}

	// IDFromEndpoint (required, xsd:anyURI) = destination CE?
	js->IDFromEndpoint = soap_strdup(soap,src->destination);

	// LocalIDFromManager (optional, xsd:string) = localId?
	js->LocalIDFromManager = soap_strdup(soap,src->localId);

	// JobDescription (required, glue:JobDescription_t) = matched_jdl ?
	js->JobDescription = soap_strdup(soap,src->matched_jdl);

	// State (required, glue:ComputingActivityState_t) = job state?
	// i.e. job status with buf,LB_GLUE_STATE_PREFIX?
	s = edg_wll_StatToString(src->state); 
	snprintf(buf,sizeof buf,LB_GLUE_STATE_PREFIX ":%s",s);
	buf[sizeof(buf) - 1] = 0;
	js->State = soap_strdup(soap,buf);
	free(s); s = NULL;

	// RestartState (optional, glue:ComputingActivityState_t) = ?
	js->RestartState = NULL;

	// ExitCode (optional, xsd:int) = ?
	js->ExitCode = 0;

	// ComputingManagerExitCode (optional, xsd:string) = ?
	js->ComputingManagerExitCode = NULL;

	// Error (optional, xsd:string) = failure_reasons?
	js->__sizeError = sizeof(src->failure_reasons);
	js->Error = soap_strdup(soap,src->failure_reasons);

	// WaitingPosition (optional, xsd:unsignedInt) = ?
	js->WaitingPosition = 0;

	// UserDomain (optional, xsd:string) = ?
	js->UserDomain = NULL;

	// Owner (required, xsd:string) = ?
	js->Owner = soap_strdup(soap,src->owner);

	// LocalOwner (optional, xsd:string) = ?
	js->LocalOwner = NULL;

	// RequestedTotalWallTime (optional, xsd:unsignedLong) = ?
	// from JDL?
	js->RequestedTotalWallTime = NULL;

	// RequestedTotalCPUTime (optional, xsd:unsignedLong) = ?
	// from JDL?
	js->RequestedTotalCPUTime = 0;

	// RequestedSlots (optional, xsd:unsignedInt) = ?
	// from JDL?
	js->RequestedSlots = 0;

	// RequestedApplicationEnvironment = ?
	js->__sizeRequestedApplicationEnvironment = 0;
	js->RequestedApplicationEnvironment = NULL;

	// StdIn (optional, xsd:string) = ?
	js->StdIn = NULL;

	// StdOut (optional, xsd:string) = ?
	js->StdOut = NULL;

	// StdErr (optional, xsd:string) = ?
	js->StdErr = NULL; 

	// LogDir (optional, xsd:string) = ?
	js->LogDir = NULL;

	// ExecutionNode (optional, xsd:string) = ce_node?
	js->__sizeExecutionNode = sizeof(src->ce_node);
	js->ExecutionNode = soap_strdup(soap,src->ce_node);

	// Queue (optional, xsd:string) = ?
	js->Queue = NULL;

	// UsedTotalWallTime (optional, xsd:unsignedLong) = ?
	js->UsedTotalWallTime = 0;

	// UsedTotalCPUTime (optional, xsd:unsignedLong) = cpuTime?
	js->UsedTotalCPUTime = src->cpuTime;

	// UsedMainMemory (optional, xsd:unsignedLong) = ?
	js->UsedMainMemory = 0;

	// SubmissionTime (optional, xsd:dateTime) = time of JOB_SUBMITTED?
	// how does it differ from the CreationTime?
	js->SubmissionTime = src->stateEnterTimes[1+EDG_WLL_JOB_SUBMITTED];

	// ComputingManagerSubmissionTime (optional, xsd:dateTime) = time of JOB_SCHEDULED?
	js->ComputingManagerSubmissionTime = src->stateEnterTimes[1+EDG_WLL_JOB_SCHEDULED];

	// StartTime (optional, xsd:dateTime) = time of JOB_RUNNING?
	js->StartTime = src->stateEnterTimes[1+EDG_WLL_JOB_RUNNING];

	// ComputingManagerEndTime (optional, xsd:dateTime) = ?
	js->ComputingManagerEndTime = 0;

	// EndTime (optional, xsd:dateTime) = time of JOB_DONE?
	js->EndTime = src->stateEnterTimes[1+EDG_WLL_JOB_DONE];

	// WorkingAreaEraseTime (optional, xsd:dateTime) = time of JOB_CLEARED?
	js->WorkingAreaEraseTime = src->stateEnterTimes[1+EDG_WLL_JOB_CLEARED];

	// ProxyExpirationTime (optional, xsd:dateTime) = ?
	js->ProxyExpirationTime = 0;

	// SubmissionHost (optional, xsd:string) = ui_host?
	js->SubmissionHost = soap_strdup(soap,src->ui_host);

	// SubmissionClientName (optional, xsd:string) = ?
	js->SubmissionClientName = NULL;

	// OtherMessages (optional, xsd:string) = reason?
	js->__sizeOtherMessages = sizeof(src->reason);
	js->OtherMessages = soap_strdup(soap,src->reason);

	// Extensions (optional, glue:Extensions_t) =  user tags?
	// see glue__Extensions_USCOREt structure
	js->Extensions = NULL;

	// Associations (optional, glue:ComputingActivity_t-Associations) = subjobs?
	// see _glue__ComputingActivity_USCOREt_Associations structure
	js->Associations = NULL;

	return SOAP_OK;
}

/**
 * __lb4agu__GetActivityInfo - return complete state information (history?) of given jobs (list of jobIds)
 * \param[in,out] soap	soap to work with
 * \param[in] in	list of jobIds
 * \param[out] out	list of complete job state information for each jobId
 * \returns SOAP_OK or SOAP_FAULT
 * TODO: distinguish also faults
 * 	UNKNOWNACTIVITYID
 *	UNKNOWNGLUE2ACTIVITYATTRIBUTE
 */
SOAP_FMAC5 int SOAP_FMAC6 __lb4agu__GetActivityInfo(
	struct soap* soap,
	struct _lb4ague__GetActivityInfoRequest *in,
	struct _lb4ague__GetActivityInfoResponse *out)
{
	edg_wll_Context	ctx = (edg_wll_Context) glite_gsplugin_get_udata(soap);
	edg_wlc_JobId	j;
	edg_wll_JobStat	s;
	int		i,flags;

	glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST, LOG_PRIORITY_DEBUG,
		"[%d] WS call %s", getpid(), __FUNCTION__);

	if (!in) return SOAP_FAULT;
	if (!in->id) return SOAP_FAULT;

	out = soap_malloc(soap, sizeof(*out));

	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, out, status, struct glue__ComputingActivity_USCOREt, in->__sizeid);

	/* process each request individually: */
	for (i=0; i<in->__sizeid && in->id[i]; i++) {

		/* first parse jobId */
		if ( edg_wlc_JobIdParse(in->id[i], &j) ) {
			edg_wll_SetError(ctx, EINVAL, in->id[i]);
			edg_wll_ErrToFault(ctx, soap);
			return SOAP_FAULT;
		}

		/* FIXME: what are the flags? */
		flags = 0;
		if (debug) {
			char *cjobid = NULL, *cflags = NULL;

			cjobid = edg_wlc_JobIdUnparse(j);
			cflags = edg_wll_stat_flags_to_string(flags);
			glite_common_log(LOG_CATEGORY_LB_SERVER_REQUEST,
				LOG_PRIORITY_DEBUG,
				"[%d] \t<flags>%s</flags>\n\t<jobId>%s</jobId>",
				getpid(), cflags, cjobid);
			free(cjobid);
			free(cflags);
		}

		/* get job status */
		if ( edg_wll_JobStatusServer(ctx, j, flags, &s) ) {
			edg_wll_ErrToFault(ctx, soap);
			return SOAP_FAULT;
		}

		/* fill in the glue__ComputingActivity_USCOREt structure */
		if (edg_wll_JobStatusToGlueComputingActivity(soap, &s, 
			GLITE_SECURITY_GSOAP_LIST_GET(out->status, i))) {
			return SOAP_FAULT;
		}
	}
	out->__sizestatus = in->__sizeid;
	GLITE_SECURITY_GSOAP_LIST_DESTROY(soap, out, status);

	return SOAP_OK;
}

