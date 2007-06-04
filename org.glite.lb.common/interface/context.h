#ifndef _EDG_WORKLOAD_LOGGING_CLIENT_CONTEXT_H
#define _EDG_WORKLOAD_LOGGING_CLIENT_CONTEXT_H

/**
 * \file context.h
 * \brief L&B API common context (publicly visible) and related definitions
 */

#ifndef LB_STANDALONE
#include "glite/wmsutils/exception/exception_codes.h"
#endif
#include "glite/wmsutils/jobid/cjobid.h"

#ident "$Header$"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup context Context
 *
 *@{
 */

/** Opaque context type */
typedef struct _edg_wll_Context *edg_wll_Context;

/** Constants defining the context parameters */
typedef enum _edg_wll_ContextParam {
	EDG_WLL_PARAM_HOST,		/**< hostname to appear as event orgin */
	EDG_WLL_PARAM_SOURCE,		/**< event source component */
	EDG_WLL_PARAM_INSTANCE,		/**< instance of the source component */
	EDG_WLL_PARAM_LEVEL,		/**< logging level */
	EDG_WLL_PARAM_DESTINATION,	/**< logging destination host */
	EDG_WLL_PARAM_DESTINATION_PORT, /**< logging destination port */
	EDG_WLL_PARAM_LOG_TIMEOUT,	/**< logging timeout (asynchronous) */
	EDG_WLL_PARAM_LOG_SYNC_TIMEOUT,	/**< logging timeout (synchronous) */
	EDG_WLL_PARAM_QUERY_SERVER,	/**< default server name to query */
	EDG_WLL_PARAM_QUERY_SERVER_PORT,/**< default server port to query */
	EDG_WLL_PARAM_QUERY_SERVER_OVERRIDE,/**< host:port parameter setting override even values in jobid (useful for debugging & hacking only) */
	EDG_WLL_PARAM_QUERY_TIMEOUT,	/**< query timeout */
	EDG_WLL_PARAM_QUERY_JOBS_LIMIT,	/**< maximal query jobs result size */
	EDG_WLL_PARAM_QUERY_EVENTS_LIMIT,/**< maximal query events result size */
	EDG_WLL_PARAM_QUERY_RESULTS,	/**< maximal query result size */
	EDG_WLL_PARAM_CONNPOOL_SIZE,	/**< maximal number of open connections in connectionsHandle.poolSize */
	EDG_WLL_PARAM_NOTIF_SERVER,	/**< default notification server name */
	EDG_WLL_PARAM_NOTIF_SERVER_PORT,/**< default notification server port */
	EDG_WLL_PARAM_NOTIF_TIMEOUT,	/**< notif timeout */
	EDG_WLL_PARAM_X509_PROXY,	/**< proxy file to use for authentication */
	EDG_WLL_PARAM_X509_KEY,		/**< key file to use for authentication */
	EDG_WLL_PARAM_X509_CERT,	/**< certificate file to use for authentication */
	EDG_WLL_PARAM_LBPROXY_STORE_SOCK,/**< lbproxy store socket path */
	EDG_WLL_PARAM_LBPROXY_SERVE_SOCK,/**<  lbproxy serve socket path */
	EDG_WLL_PARAM_LBPROXY_USER,	/**< user credentials when logging to L&B Proxy */
	EDG_WLL_PARAM_JPREG_TMPDIR,		/**< maildir storage path */
	EDG_WLL_PARAM__LAST,		/**< marker, LB internal use only */
} edg_wll_ContextParam;

/** sets returned query results */
typedef enum _edg_wll_QueryResults {
	EDG_WLL_QUERYRES_UNDEF,		/* uninitialized value */
	EDG_WLL_QUERYRES_NONE,
	EDG_WLL_QUERYRES_ALL,
	EDG_WLL_QUERYRES_LIMITED,
	EDG_WLL_QUERYRES__LAST		/* marker, for internal use only */
} edg_wll_QueryResults;

/** identification of logging component */
typedef enum _edg_wll_Source {
	EDG_WLL_SOURCE_NONE,		/* uninitialized value */
	EDG_WLL_SOURCE_USER_INTERFACE,
	EDG_WLL_SOURCE_NETWORK_SERVER,
	EDG_WLL_SOURCE_WORKLOAD_MANAGER,
	EDG_WLL_SOURCE_BIG_HELPER,
	EDG_WLL_SOURCE_JOB_SUBMISSION,
	EDG_WLL_SOURCE_LOG_MONITOR,
	EDG_WLL_SOURCE_LRMS,
	EDG_WLL_SOURCE_APPLICATION,
	EDG_WLL_SOURCE_LB_SERVER,
	EDG_WLL_SOURCE__LAST		/* marker, for internal use only */
} edg_wll_Source;

/** Currently an alias. Will be replaced when migration NS -> WMProxy
 * is finished. */
#define EDG_WLL_SOURCE_WM_PROXY EDG_WLL_SOURCE_NETWORK_SERVER


/** Allocate an initialize a new context object.
 * \param[out] context 		returned context
 * \return 0 on success, ENOMEM if malloc() fails
 */
int edg_wll_InitContext(edg_wll_Context *context);

/** Destroy and free context object.
 * Also performs necessary cleanup (closing connections etc.)
 * \param[in] context 		context to free
 */
void edg_wll_FreeContext(edg_wll_Context context);

/** Set a context parameter.
 * \param[in,out] context 	context to work with
 * \param[in] param 		parameter to set
 * \param[in] ... 		value to set (if NULL or 0, default is used)
 * \retval 0 success
 * \retval EINVAL param is not a valid parameter, or invalid value
 */
int edg_wll_SetParam(
	edg_wll_Context context,
	edg_wll_ContextParam param,
	...
);

struct timeval; /* XXX: gcc, shut up! */

/** Set a context parameter of type int.
 * \param[in,out] ctx           context to work with
 * \param[in] param		parameter to set
 * \param[in] val		value to set
 * \retval 0 success
 * \retval EINVAL param is not a valid parameter, or invalid value
 */
int edg_wll_SetParamInt(edg_wll_Context ctx,edg_wll_ContextParam param,int val);

/** Set a context parameter of type string.
 * \param[in,out] ctx		context to work with
 * \param[in] param		parameter to set
 * \param[in] val		value to set (if NULL, default is used)
 * \retval 0 success
 * \retval EINVAL param is not a valid parameter, or invalid value
 */
int edg_wll_SetParamString(edg_wll_Context ctx,edg_wll_ContextParam param,const char *val);

/** Set a context parameter of type timeval.
 * \param[in,out] ctx		context to work with
 * \param[in] param		parameter to set
 * \param[in] val		value to set (if NULL, default is used)
 * \retval 0 success
 * \retval EINVAL param is not a valid parameter, or invalid value
 */
int edg_wll_SetParamTime(edg_wll_Context ctx,edg_wll_ContextParam param,const struct timeval *val);

/** Get current parameter value.
 * \param[in,out] context 	context to work with
 * \param[in] param 		parameter to retrieve
 * \param[out] ... 		pointer to output variable
 * \retval 0 success
 * \retval EINVAL param is not a valid parameter
 */
int edg_wll_GetParam(
	edg_wll_Context context,
	edg_wll_ContextParam param,
	...
);


/**
 * L&B subsystem specific error codes.
 * Besides them L&B functions return standard \a errno codes in their usual
 * meaning.
 */

/* XXX: cleanup required */

#ifndef GLITE_WMS_LOGGING_ERROR_BASE
#define GLITE_WMS_LOGGING_ERROR_BASE 1400
#endif

typedef enum _edg_wll_ErrorCode {
/** Base for L&B specific code. Use the constant from common/ */
	EDG_WLL_ERROR_BASE = GLITE_WMS_LOGGING_ERROR_BASE,
	EDG_WLL_ERROR_PARSE_BROKEN_ULM, /**< Parsing ULM line into edg_wll_Event structure */
	EDG_WLL_ERROR_PARSE_EVENT_UNDEF, /**< Undefined event name */
	EDG_WLL_ERROR_PARSE_MSG_INCOMPLETE, /**< Incomplete message (missing fields) */
	EDG_WLL_ERROR_PARSE_KEY_DUPLICITY, /**< Duplicate entry in message */
	EDG_WLL_ERROR_PARSE_KEY_MISUSE, /**< Entry not allowed for this message type */
	EDG_WLL_ERROR_PARSE_OK_WITH_EXTRA_FIELDS, /**< Additional, not understood fields found in message.
		The rest is OK therefore this is not a true error. */
        EDG_WLL_ERROR_XML_PARSE, /**< Error in parsing XML protocol. */
        EDG_WLL_ERROR_SERVER_RESPONSE, /**< Generic failure on server. See syslog on the server machine for details. */
        EDG_WLL_ERROR_JOBID_FORMAT, /**< Malformed jobid */
	EDG_WLL_ERROR_DB_CALL, /**< Failure of underlying database engine.
		See errDesc returned by edg_wll_ErrorCode(). */
	EDG_WLL_ERROR_URL_FORMAT, /**< Malformed URL */
	EDG_WLL_ERROR_MD5_CLASH, /**< MD5 hash same for different strings. Very unlikely :-). */
	EDG_WLL_ERROR_GSS, /**< Generic GSSAPI error. See errDesc returned by edg_wll_Error(). */
	EDG_WLL_ERROR_DNS, /**< DNS resolver error. See errDesc returned by edg_wll_Error(). */
	EDG_WLL_ERROR_NOJOBID,	/**< Attmepted call requires calling edg_wll_SetLoggingJob() first. */
	EDG_WLL_ERROR_NOINDEX,	/**< Query does not contain any conidion on indexed attribute. */
	EDG_WLL_IL_PROTO,	/**< Lbserver (proxy) store communication protocol error. */
	EDG_WLL_IL_SYS,         /**< Interlogger internal error. */
	EDG_WLL_IL_EVENTS_WAITING, /**< Interlogger still has events pending delivery. */
	EDG_WLL_ERROR_COMPARE_EVENTS, /**< Two compared events differ. */
	EDG_WLL_ERROR_SQL_PARSE, /**< Error in SQL parsing. */
} edg_wll_ErrorCode;

/**
 * Retrieve error details on recent API call
 * \param[in] context 		context to work with
 * \param[out] errText		standard error text
 *      (may be NULL - no text returned)
 * \param[out] errDesc		additional error description
 *      (may be NULL - no text returned)
 * \return Error code of the recent error
 */

int edg_wll_Error(
	edg_wll_Context context,
	char		**errText,
	char		**errDesc
);

/** Convert source code to printable string 
 */
char * edg_wll_SourceToString(edg_wll_Source src);

/** Convert name to source code
 * \return Matching code or EDG_WLL_SOURCE_NONE
 */
edg_wll_Source edg_wll_StringToSource(const char *name);

/** Convert Query result code to printable string 
 */
char * edg_wll_QResultToString(edg_wll_QueryResults res);

/** Convert name to Query result code
 * \return Matching code or EDG_WLL_SOURCE_NONE
 */
edg_wll_QueryResults edg_wll_StringToQResult(const char *name);

/**
 * type of sequence code (used when setting to the context)
 */
#define EDG_WLL_SEQ_NORMAL      1
#define EDG_WLL_SEQ_DUPLICATE   11
#define EDG_WLL_SEQ_PBS		2
#define EDG_WLL_SEQ_CONDOR	3

/**
 * initial sequence code for BigHelper
 */
#define EDG_WLL_SEQ_BIGHELPER_INITIAL	"UI=000002:NS=0000000000:WM=000000:BH=0000000001:JSS=000000:LM=000000:LRMS=000000:APP=000000:LBS=000000"

/**
 * the wms purger uses this sequence code while logging the cleared event
 * agreed with Salvatore Monforte
 */
#define EDG_WLL_SEQ_CLEAR	"UI=000009:NS=0000096669:WM=000000:BH=0000000000:JSS=000000:LM=000000:LRMS=000000:APP=000000:LBS=000000"

/**
 * used for logging abort event by wms components
 * agreed with Francesco Giacomini
 */
#define EDG_WLL_SEQ_ABORT	"UI=000000:NS=0000096660:WM=000000:BH=0000000000:JSS=000000:LM=000000:LRMS=000000:APP=000000:LBS=000000"


/** Retrieve current sequence code from the context */
char * edg_wll_GetSequenceCode(
	const edg_wll_Context	context
);

/**
 * retrieve the current logging JobId from the context
 */
int edg_wll_GetLoggingJob(
	const edg_wll_Context	context,
	edg_wlc_JobId	*jobid_out
);

/*
 *@} end of group
 */

#ifdef __cplusplus
}
#endif

#endif 
