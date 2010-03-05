#include <stdsoap2.h>

#include "soap_version.h"
#include "glite/security/glite_gsplugin.h"
#include "glite/security/glite_gscompat.h"

#include "glite/lbu/log.h"
#include "glite/lb/context-int.h"
#include "glite/lb/xml_conversions.h"

#include "bk_ws_Stub.h"
#include "ws_fault.h"
#include "ws_typeref.h"
#include "jobstat.h"

extern int debug;

#define LB_GLUE_STATE_PREFIX "urn:org.glite.lb"

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
	for (i=0; i<in->__sizeid; i++) {
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

	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, out, status, struct glue__ComputingActivity_USCOREt, 1);

	/* process each request individually: */
	for (i=0; in->id[i]; i++) {
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

		/* fill in the response fields */
		GLITE_SECURITY_GSOAP_LIST_GET(out->status, i)->ID = in->id[i];
		GLITE_SECURITY_GSOAP_LIST_GET(out->status, i)->State = edg_wll_StatToString(s.state);
	}

	return SOAP_OK;
}

