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
	edg_wll_JobStat *src,
	struct glue__ComputingActivity_USCOREt *js)
{
        int     i;
        char    buf[1000],*s = NULL,*aux,*aux2;

	memset(js, 0, sizeof(*js));

	// ID (required, glue:ID_t) = jobId
	s = edg_wlc_JobIdUnparse(src->jobId); 
	js->ID = soap_strdup(soap,s); 
	free(s); s=NULL;

	// BaseType (required, xsd:string) = "Activity"?
	js->BaseType = soap_strdup(soap,"Activity");

	// CreationTime (optional, xsd:dateTime) = submission time?
	js->CreationTime = soap_malloc(soap,sizeof *js->CreationTime);
	*js->CreationTime = src->stateEnterTimes[1+EDG_WLL_JOB_SUBMITTED];

	// Validity (optional, xsd:unsignedLong) = N/A
	js->Validity = 0;

	// Name (optional, xsd:string) = N/A
	js->Name = NULL;

	// Type (optional, glue:ComputingActivityType_t) = jobtype?
	// see glue__ComputingActivityType_USCOREt
	js->Type = soap_malloc(soap,sizeof *js->Type);
	switch (src->jobtype) {
		case EDG_WLL_STAT_SIMPLE:
			*js->Type = src->parent_job ? 
				glue__ComputingActivityType_USCOREt__collectionelement :
				glue__ComputingActivityType_USCOREt__single;
			break;
		case EDG_WLL_STAT_DAG:
		case EDG_WLL_STAT_COLLECTION:
		case EDG_WLL_STAT_FILE_TRANSFER_COLLECTION:
			*js->Type = glue__ComputingActivityType_USCOREt__single; /* XXX: no equivalent in glue */
			break;
		case EDG_WLL_STAT_PBS:
		case EDG_WLL_STAT_CONDOR:
		case EDG_WLL_STAT_CREAM:
		case EDG_WLL_STAT_FILE_TRANSFER:
			*js->Type = glue__ComputingActivityType_USCOREt__single;
			break;
		default:
			*js->Type = glue__ComputingActivityType_USCOREt__single;
			break;
	}

	// IDFromEndpoint (required, xsd:anyURI) = globusId?
	// TODO: put CREAM id here once it's available
	js->IDFromEndpoint = soap_strdup(soap,src->globusId);

	// LocalIDFromManager (optional, xsd:string) = localId
	js->LocalIDFromManager = soap_strdup(soap,src->localId);

	// JobDescription (required, glue:JobDescription_t) = matched_jdl
	js->JobDescription = soap_strdup(soap,src->matched_jdl);

	// State (required, glue:ComputingActivityState_t) = state
	// i.e. job status with buf,LB_GLUE_STATE_PREFIX?
	s = edg_wll_StatToString(src->state); 
	snprintf(buf,sizeof buf,LB_GLUE_STATE_PREFIX ":%s",s);
	buf[sizeof(buf) - 1] = 0;
	js->State = soap_strdup(soap,buf);
	free(s); s = NULL;

	// RestartState (optional, glue:ComputingActivityState_t) = N/A
	js->RestartState = NULL;

	// ExitCode (optional, xsd:int) 
	js->ExitCode = soap_malloc(soap,sizeof *js->ExitCode);
	*js->ExitCode = src->exit_code;

	// ComputingManagerExitCode (optional, xsd:string) = done_code
	s=edg_wll_DoneStatus_codeToString(src->done_code);
	js->ComputingManagerExitCode = soap_strdup(soap,s);
	free(s); s=NULL;

	// Error (optional, xsd:string) = failure_reasons
	if (src->failure_reasons) {
		js->__sizeError = 1;
		js->Error = soap_malloc(soap,sizeof js->Error[0]);
		js->Error[0] = soap_strdup(soap,src->failure_reasons);
	} else {
		js->__sizeError = 0;
		js->Error = NULL;
	}

	// WaitingPosition (optional, xsd:unsignedInt) = N/A
	js->WaitingPosition = 0;

	// UserDomain (optional, xsd:string) = user_fqans?
	if (src->user_fqans) {
		aux2 = strdup("");
		for (i=0; src->user_fqans && src->user_fqans[i]; i++) {
			asprintf(&aux,"%s\n%s",aux2,src->user_fqans[i]);
			free(aux2); aux2 = aux; aux = NULL;
		}
		js->UserDomain = soap_strdup(soap,aux2);
		free(aux2); aux2 = NULL;
	} else js->UserDomain = NULL;

	// Owner (required, xsd:string) = owner
	js->Owner = soap_strdup(soap,src->owner);

	// LocalOwner (optional, xsd:string) = N/A
	js->LocalOwner = NULL;

	// RequestedTotalWallTime (optional, xsd:unsignedLong) = N/A
	js->RequestedTotalWallTime = NULL;

	// RequestedTotalCPUTime (optional, xsd:unsignedLong) = N/A
	js->RequestedTotalCPUTime = 0;

	// RequestedSlots (optional, xsd:unsignedInt) = NodeNumber from JDL
	s = edg_wll_JDLField(src,"NodeNumber");
	js->RequestedSlots = soap_malloc(soap,sizeof *js->RequestedSlots);
	*js->RequestedSlots = s ? atoi(s) : 1;
	free(s); s = NULL;

	// RequestedApplicationEnvironment = N/A
	js->__sizeRequestedApplicationEnvironment = 0;
	js->RequestedApplicationEnvironment = NULL;

	// StdIn (optional, xsd:string) = StdInput from JDL
	if ((s = edg_wll_JDLField(src,"StdInput"))) js->StdIn = soap_strdup(soap,s);
	free(s); s = NULL;

	// StdOut (optional, xsd:string) = StdOutput from JDL
	if ((s = edg_wll_JDLField(src,"StdOutput"))) js->StdOut = soap_strdup(soap,s);
	free(s); s = NULL;

	// StdErr (optional, xsd:string) = StdError from JDL
	if ((s = edg_wll_JDLField(src,"StdError"))) js->StdErr = soap_strdup(soap,s);
	free(s); s = NULL;

	// LogDir (optional, xsd:string) = N/A
	js->LogDir = NULL;

	// ExecutionNode (optional, xsd:string) = ce_node?
	if (src->ce_node) {
		js->__sizeExecutionNode = 1;
		js->ExecutionNode = soap_malloc(soap,sizeof js->ExecutionNode[0]);
		js->ExecutionNode[0] = soap_strdup(soap,src->ce_node);
	} else {
		js->__sizeExecutionNode = 0;
		js->ExecutionNode = NULL;
	}

	// Queue (optional, xsd:string) = destination (ID of CE where the job is being sent)?
	js->Queue = soap_strdup(soap,src->destination);

	// UsedTotalWallTime (optional, xsd:unsignedLong) = N/A
	// TODO: put resource usage once available
	js->UsedTotalWallTime = 0;

	// UsedTotalCPUTime (optional, xsd:unsignedLong) = cpuTime?
	// TODO: put resource usage once available
	js->UsedTotalCPUTime = soap_malloc(soap,sizeof *js->UsedTotalCPUTime);
	*js->UsedTotalCPUTime = src->cpuTime;

	// UsedMainMemory (optional, xsd:unsignedLong) = N/A
	// TODO: put resource usage once available
	js->UsedMainMemory = 0;

	// SubmissionTime (optional, xsd:dateTime) = time of JOB_WAITING
	if (src->stateEnterTimes[1+EDG_WLL_JOB_WAITING]) {
		js->SubmissionTime = soap_malloc(soap,sizeof *js->SubmissionTime);
		*js->SubmissionTime = src->stateEnterTimes[1+EDG_WLL_JOB_WAITING];
	} else js->SubmissionTime = NULL;

	// ComputingManagerSubmissionTime (optional, xsd:dateTime) = time of JOB_SCHEDULED
	if (src->stateEnterTimes[1+EDG_WLL_JOB_SCHEDULED]) {
		js->ComputingManagerSubmissionTime = soap_malloc(soap,sizeof *js->ComputingManagerSubmissionTime);
		*js->ComputingManagerSubmissionTime = src->stateEnterTimes[1+EDG_WLL_JOB_SCHEDULED];
	} else js->ComputingManagerSubmissionTime = NULL;

	// StartTime (optional, xsd:dateTime) = time of JOB_RUNNING
	if (src->stateEnterTimes[1+EDG_WLL_JOB_RUNNING]) {
		js->StartTime = soap_malloc(soap,sizeof *js->StartTime);
		*js->StartTime = src->stateEnterTimes[1+EDG_WLL_JOB_RUNNING];
	} else js->StartTime = NULL;

	// ComputingManagerEndTime (optional, xsd:dateTime) = time of JOB_DONE?
	if (src->stateEnterTimes[1+EDG_WLL_JOB_DONE]) {
		js->ComputingManagerEndTime = soap_malloc(soap,sizeof *js->ComputingManagerEndTime);
		*js->ComputingManagerEndTime = src->stateEnterTimes[1+EDG_WLL_JOB_DONE];
	} else js->ComputingManagerEndTime = NULL;

	// EndTime (optional, xsd:dateTime) = time of JOB_DONE
	if (src->stateEnterTimes[1+EDG_WLL_JOB_DONE]) {
		js->EndTime = soap_malloc(soap,sizeof *js->EndTime);
		*js->EndTime = src->stateEnterTimes[1+EDG_WLL_JOB_DONE];
	} else js->EndTime = NULL;

	// WorkingAreaEraseTime (optional, xsd:dateTime) = time of JOB_CLEARED
	if (src->stateEnterTimes[1+EDG_WLL_JOB_CLEARED]) {
		js->WorkingAreaEraseTime = soap_malloc(soap,sizeof *js->WorkingAreaEraseTime);
		*js->WorkingAreaEraseTime = src->stateEnterTimes[1+EDG_WLL_JOB_CLEARED];
	} else js->WorkingAreaEraseTime = NULL;

	// ProxyExpirationTime (optional, xsd:dateTime) = N/A
	js->ProxyExpirationTime = 0;

	// SubmissionHost (optional, xsd:string) = ui_host
	js->SubmissionHost = soap_strdup(soap,src->ui_host);

	// SubmissionClientName (optional, xsd:string) = N/A
	js->SubmissionClientName = NULL;

	// OtherMessages (optional, xsd:string) = reason?
	if (src->reason) {
		js->__sizeOtherMessages = 1;
		js->OtherMessages = soap_malloc(soap,sizeof js->OtherMessages[0]);
		js->OtherMessages[0] = soap_strdup(soap,src->reason);
	} else {
		js->__sizeOtherMessages = 0;
		js->OtherMessages = NULL;
	}

	// Extensions (optional, glue:Extensions_t) =  user tags?
	// see glue__Extensions_USCOREt structure
	if (src->user_tags) {
		js->Extensions = soap_malloc(soap,sizeof *js->Extensions);
		for (i=0; src->user_tags[i].tag; i++);
		js->Extensions->__sizeExtension = i;
		js->Extensions->Extension = soap_malloc(soap,i*sizeof js->Extensions->Extension[0]);

		for (i=0; src->user_tags[i].tag; i++) {
			js->Extensions->Extension[i].Key = soap_strdup(soap,src->user_tags[i].tag);
			js->Extensions->Extension[i].__item = soap_strdup(soap,src->user_tags[i].value);
		}
	}
	else js->Extensions = NULL;

	// Associations (optional, glue:ComputingActivity_t-Associations) = subjobs?
	// see _glue__ComputingActivity_USCOREt_Associations structure
	if (src->children_num) {
		js->Associations = soap_malloc(soap,sizeof *js->Associations);
		memset(js->Associations,0,sizeof *js->Associations);
		js->Associations->__sizeActivityID = src->children_num;
		js->Associations->ActivityID = soap_malloc(soap,src->children_num * sizeof js->Associations->ActivityID[0]);
		for (i=0; src->children[i]; i++) js->Associations->ActivityID[i] = soap_strdup(soap,src->children[i]);
	}
	else js->Associations = NULL;


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

	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, out, status, struct glue__ComputingActivity_USCOREt, in->__sizeid);

	/* process each request individually: */
	for (i=0; i<in->__sizeid && in->id[i]; i++) {

		/* first parse jobId */
		if ( edg_wlc_JobIdParse(in->id[i], &j) ) {
			edg_wll_SetError(ctx, EINVAL, in->id[i]);
			edg_wll_ErrToFault(ctx, soap);
			return SOAP_FAULT;
		}

		flags = EDG_WLL_STAT_CLASSADS | EDG_WLL_STAT_CHILDREN;
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

	return SOAP_OK;
}

