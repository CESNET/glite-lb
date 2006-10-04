#ident "$Header$"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <syslog.h>
#include <string.h>

#include "glite/wmsutils/jobid/strmd5.h"
#include "glite/lb/ulm_parse.h"
#include "glite/lb/trio.h"

#include "glite/lb/producer.h"

#include "prod_proto.h"

#ifdef FAKE_VERSION
int edg_wll_DoLogEvent(edg_wll_Context context, edg_wll_LogLine logline);
int edg_wll_DoLogEventProxy(edg_wll_Context context, edg_wll_LogLine logline);
int edg_wll_DoLogEventDirect(edg_wll_Context context, edg_wll_LogLine logline);
#else

/**
 *----------------------------------------------------------------------
 * handle_answers - handle answers from edg_wll_log_*proto_client
 *----------------------------------------------------------------------
 */
static
int handle_answers(edg_wll_Context context, int code, const char *text)
{
        static char     err[256];

	switch(code) {
		case 0:
		case EINVAL:
		case ENOSPC:
		case ENOMEM:
		case EDG_WLL_ERROR_GSS:
		case EDG_WLL_ERROR_DNS:
		case ENOTCONN:
		case ECONNREFUSED:
		case ETIMEDOUT:
		case EAGAIN:
			break;
		case EDG_WLL_ERROR_PARSE_EVENT_UNDEF:
		case EDG_WLL_ERROR_PARSE_MSG_INCOMPLETE:
		case EDG_WLL_ERROR_PARSE_KEY_DUPLICITY:
		case EDG_WLL_ERROR_PARSE_KEY_MISUSE:
//		case EDG_WLL_ERROR_PARSE_OK_WITH_EXTRA_FIELDS:
                        snprintf(err, sizeof(err), "%s: Error code mapped to EINVAL", text);
                        edg_wll_UpdateError(context,EINVAL,err);
                        break;
		case EDG_WLL_IL_PROTO:
		case EDG_WLL_IL_SYS:
		case EDG_WLL_IL_EVENTS_WAITING:
                        snprintf(err, sizeof(err), "%s: Error code mapped to EAGAIN", text);
                        edg_wll_UpdateError(context,EAGAIN,err);
			break;
		default:
                        snprintf(err, sizeof(err), "%s: Error code mapped to EAGAIN", text);
                        edg_wll_UpdateError(context,EAGAIN,err);
			break;
	}

	return edg_wll_Error(context, NULL, NULL);
}

/**
 *----------------------------------------------------------------------
 * Connects to local-logger and sends already formatted ULM string
 * \brief helper logging function
 * \param[in,out] context	context to work with,
 * \param[in] logline		formated ULM string
 *----------------------------------------------------------------------
 */
int edg_wll_DoLogEvent(
	edg_wll_Context context,
	edg_wll_LogLine logline)
{
	int	answer = 0;
        edg_wll_GssConnection   con;
        edg_wll_ResetError(context);
        memset(&con, 0, sizeof(con));

   /* open a gss connection to local-logger: */
	if ((answer = edg_wll_log_connect(context,&con)) < 0) {
		goto edg_wll_DoLogEvent_end;
	}

   /* send the message to the local-logger: */
	answer = edg_wll_log_proto_client(context,&con,logline);

edg_wll_DoLogEvent_end:
	if (con.sock) edg_wll_gss_close(&con,&context->p_tmp_timeout);

	return handle_answers(context,answer,"edg_wll_DoLogEvent()");
}

/**
 *----------------------------------------------------------------------
 * Connects to L&B Proxy and sends already formatted ULM string
 * \brief helper logging function
 * \param[in,out] context	context to work with,
 * \param[in] logline		formated ULM string
 *----------------------------------------------------------------------
 */
int edg_wll_DoLogEventProxy(
	edg_wll_Context context,
	edg_wll_LogLine logline)
{
	int	answer = 0;
	edg_wll_PlainConnection con;

	edg_wll_ResetError(context);
	memset(&con, 0, sizeof(con));

   /* open a plain connection to L&B Proxy: */
	if ((answer = edg_wll_log_proxy_connect(context,&con)) < 0) {
		goto edg_wll_DoLogEventProxy_end;
	}

   /* and send the message to the L&B Proxy: */
	answer = edg_wll_log_proxy_proto_client(context,&con,logline);
	
edg_wll_DoLogEventProxy_end:
	if (con.sock) edg_wll_plain_close(&con);

	return handle_answers(context,answer,"edg_wll_DoLogEventProxy()");
}

/**
 *----------------------------------------------------------------------
 * Connects to bkserver and sends already formatted ULM string
 * \brief helper logging function
 * \param[in,out] context	context to work with,
 * \param[in] logline		formated ULM string
 *----------------------------------------------------------------------
 */
int edg_wll_DoLogEventDirect(
	edg_wll_Context context,
	edg_wll_LogLine logline)
{
	int 	answer = 0;
	edg_wll_GssConnection	con;

	edg_wll_ResetError(context);
	memset(&con, 0, sizeof(con));

   /* open a gss connection to bkserver: */
	if ((answer = edg_wll_log_direct_connect(context,&con)) < 0) {
		goto edg_wll_DoLogEventDirect_end;
	}

   /* and send the message to the bkserver: */
	answer = edg_wll_log_direct_proto_client(context,&con,logline);

edg_wll_DoLogEventDirect_end:
	edg_wll_gss_close(&con,&context->p_tmp_timeout);

	return handle_answers(context,answer,"edg_wll_DoLogEventDirect()");
}

#endif /* FAKE_VERSION */

#define	LOGFLAG_ASYNC	0 /**< asynchronous logging */
#define	LOGFLAG_SYNC	1 /**< synchronous logging */
#define	LOGFLAG_NORMAL	2 /**< logging to local logger */
#define	LOGFLAG_PROXY	4 /**< logging to L&B Proxy */
#define	LOGFLAG_DIRECT	8 /**< logging directly to bkserver */

/**
 *----------------------------------------------------------------------
 * Formats a logging message 
 * \brief formats a logging message
 * \param[in,out] context	context to work with,
 * \param[in] flags		as defined by LOGFLAG_*
 * \param[in] event		type of the event,
 * \param[out] logline		formated logging message
 * \param[in] fmt		printf()-like format string,
 * \param[in] ...		event specific values/data according to fmt.
 *----------------------------------------------------------------------
 */
static int edg_wll_FormatLogLine(
	edg_wll_Context context,
	int flags,
	edg_wll_EventCode event,
	edg_wll_LogLine *logline,
	char *fmt, ...)
{
	va_list	fmt_args;
	int	priority;
	int	ret;
	char	*fix,*var,*dguser;
	char	*source,*eventName,*lvl,*fullid,*seq;
        struct timeval start_time;
	char	date[ULM_DATE_STRING_LENGTH+1];
	edg_wll_LogLine out;
	size_t  size;
	int	i;

	i = errno = size = ret = 0;
	seq = fix = var = dguser = out = source = eventName = lvl = fullid = NULL;
	priority = flags & LOGFLAG_SYNC;

	edg_wll_ResetError(context);

   /* format the message: */
	va_start(fmt_args,fmt);

	gettimeofday(&start_time,0);
	if (edg_wll_ULMTimevalToDate(start_time.tv_sec,start_time.tv_usec,date) != 0) {
		edg_wll_SetError(context,ret = EINVAL,"edg_wll_FormatLogLine(): edg_wll_ULMTimevalToDate() error"); 
		goto edg_wll_formatlogline_end; 
	}
 	source = edg_wll_SourceToString(context->p_source);
	lvl = edg_wll_LevelToString(context->p_level);
	eventName = edg_wll_EventToString(event);
	if (!eventName) { 
		edg_wll_SetError(context,ret = EINVAL,"edg_wll_FormatLogLine(): event name not specified"); 
		goto edg_wll_formatlogline_end; 
	}
	if (!(fullid = edg_wlc_JobIdUnparse(context->p_jobid))) { 
		edg_wll_SetError(context,ret = EINVAL,"edg_wll_FormatLogLine(): edg_wlc_JobIdUnparse() error"); 
		goto edg_wll_formatlogline_end;
	}
	seq = edg_wll_GetSequenceCode(context);

	if (trio_asprintf(&fix,EDG_WLL_FORMAT_COMMON,
			date,context->p_host,lvl,priority,
			source,context->p_instance ? context->p_instance : "",
			eventName,fullid,seq) == -1) {
		edg_wll_SetError(context,ret = ENOMEM,"edg_wll_FormatLogLine(): trio_asprintf() error"); 
		goto edg_wll_formatlogline_end; 
	}
	/* TODO: add always, probably new context->p_user */
	if ( ( (flags & LOGFLAG_PROXY) || (flags & LOGFLAG_DIRECT) ) && 
	   (context->p_user_lbproxy) ) {
		if (trio_asprintf(&dguser,EDG_WLL_FORMAT_USER,context->p_user_lbproxy) == -1) {
			edg_wll_SetError(context,ret = ENOMEM,"edg_wll_FormatLogLine(): trio_asprintf() error"); 
			goto edg_wll_formatlogline_end; 
		}
	} else {
		dguser = strdup("");
	}
	if (trio_vasprintf(&var,fmt,fmt_args) == -1) { 
		edg_wll_SetError(context,ret = ENOMEM,"edg_wll_FormatLogLine(): trio_vasprintf() error"); 
		goto edg_wll_formatlogline_end; 
	}
	if (asprintf(&out,"%s%s%s\n",fix,dguser,var) == -1) { 
		edg_wll_SetError(context,ret = ENOMEM,"edg_wll_FormatLogLine(): asprintf() error"); 
		goto edg_wll_formatlogline_end; 
	}
	size = strlen(out);

	if (priority && (size > EDG_WLL_LOG_SYNC_MAXMSGSIZE)) {
		edg_wll_SetError(context,ret = ENOSPC,"edg_wll_FormatLogLine(): Message size too large for synchronous transfer");
		goto edg_wll_formatlogline_end;
	}

#ifdef EDG_WLL_LOG_STUB
//	fprintf(stderr,"edg_wll_FormatLogLine (%d chars): %s",size,out);
#endif
	if (out) {
		*logline = out;
	} else {
		*logline = NULL;
	}
	
edg_wll_formatlogline_end:
	va_end(fmt_args);
	if (seq) free(seq); 
	if (fix) free(fix); 
	if (dguser) free(dguser);
	if (var) free(var); 
	if (source) free(source);
	if (lvl) free(lvl);
	if (eventName) free(eventName);
	if (fullid) free(fullid);

	if (ret) edg_wll_UpdateError(context,0,"Logging library ERROR: ");

	return edg_wll_Error(context,NULL,NULL);
}

/**
 *----------------------------------------------------------------------
 * Formats a logging message and sends it to local-logger
 * \brief master logging event function
 * \param[in,out] context	INOUT context to work with,
 * \param[in] flags		as defined by LOGFLAG_*
 * \param[in] event		type of the event,
 * \param[in] fmt		printf()-like format string,
 * \param[in] ...		event specific values/data according to fmt.
 *----------------------------------------------------------------------
 */
static int edg_wll_LogEventMaster(
	edg_wll_Context context,
	int flags,
	edg_wll_EventCode event,
	char *fmt, ...)
{
	va_list	fmt_args;
	int	priority;
	int	ret;
	edg_wll_LogLine in,out;

	priority = flags & LOGFLAG_SYNC;

	edg_wll_ResetError(context);

   /* default return value is "Try Again" */
	ret = EAGAIN; 

   /* format the message: */
	va_start(fmt_args,fmt);

	if (trio_vasprintf(&in,fmt,fmt_args) == -1) {
		edg_wll_UpdateError(context,ret = ENOMEM,"edg_wll_LogEventMaster(): trio_vasprintf() error");
		goto edg_wll_logeventmaster_end; 
	}

	if (edg_wll_FormatLogLine(context,flags,event,&out,"%s",in) != 0 ) {
		edg_wll_UpdateError(context,ret = EINVAL,"edg_wll_LogEventMaster(): edg_wll_FormatLogLine() error"); 
		goto edg_wll_logeventmaster_end; 
	}

#ifdef EDG_WLL_LOG_STUB
//	fprintf(stderr,"edg_wll_LogEventMaster (%d chars): %s",strlen(out),out);
#endif
	
	context->p_tmp_timeout.tv_sec = 0;
	context->p_tmp_timeout.tv_usec = 0;
	if (priority) {
		context->p_tmp_timeout = context->p_sync_timeout;
	}
	else {
		context->p_tmp_timeout = context->p_log_timeout;
	}

   /* and send the message */ 
#ifndef LB_PERF_DROP
	if (flags & LOGFLAG_NORMAL) {
		/* to the local-logger: */
		ret = edg_wll_DoLogEvent(context, out);
	} else if (flags & LOGFLAG_PROXY) {
		/* to the L&B Proxy: */
		ret = edg_wll_DoLogEventProxy(context, out);
	} else if (flags & LOGFLAG_DIRECT) {
		/* directly to the bkserver: */
		ret = edg_wll_DoLogEventDirect(context, out);
	} else {
		edg_wll_SetError(context,ret = EINVAL,"edg_wll_LogEventMaster(): wrong flag specified");
	}
#endif

edg_wll_logeventmaster_end:
	va_end(fmt_args);
	if (in) free(in);
	if (out) free(out);

	if (!ret) if(edg_wll_IncSequenceCode(context)) {
		edg_wll_SetError(context,ret = EINVAL,"edg_wll_LogEventMaster(): edg_wll_IncSequenceCode failed");
	}

	if (ret) edg_wll_UpdateError(context,0,"Logging library ERROR: ");

	return edg_wll_Error(context,NULL,NULL);
}


/**
 *----------------------------------------------------------------------
 * Formats a logging message and sends it asynchronously to local-logger
 * \brief generic asynchronous logging function
 *----------------------------------------------------------------------
 */
int edg_wll_LogEvent(
        edg_wll_Context context,
        edg_wll_EventCode event,
        char *fmt, ...)
{
	int	ret=0;
	char	*list=NULL;
	va_list	fmt_args;

	edg_wll_ResetError(context);

	va_start(fmt_args,fmt);
	if (trio_vasprintf(&list,fmt,fmt_args) == -1) { 
		edg_wll_SetError(context,ret = ENOMEM,"edg_wll_LogEvent(): trio_vasprintf() error"); 
		goto edg_wll_logevent_end; 
	}

	ret=edg_wll_LogEventMaster(context,LOGFLAG_NORMAL | LOGFLAG_ASYNC,event,"%s",list);

edg_wll_logevent_end:
	va_end(fmt_args);
        if (list) free(list);

	if (ret) edg_wll_UpdateError(context,0,"edg_wll_LogEvent(): ");

	return edg_wll_Error(context,NULL,NULL);
}

/**
 *----------------------------------------------------------------------
 * Formats a logging message and sends it synchronously to local-logger
 * \brief generic synchronous logging function
 * \note simple wrapper around edg_wll_LogEventMaster()
 *----------------------------------------------------------------------
 */
int edg_wll_LogEventSync(
        edg_wll_Context context,
        edg_wll_EventCode event,
        char *fmt, ...)
{
	int	ret=0;
	char	*list=NULL;
	va_list	fmt_args;

	edg_wll_ResetError(context);

	va_start(fmt_args,fmt);
	if (trio_vasprintf(&list,fmt,fmt_args) == -1) { 
		edg_wll_SetError(context,ret = ENOMEM,"edg_wll_LogEventSync(): trio_vasprintf() error"); 
		goto edg_wll_logeventsync_end; 
	}

	ret=edg_wll_LogEventMaster(context,LOGFLAG_NORMAL | LOGFLAG_SYNC,event,"%s",list);

edg_wll_logeventsync_end:
	va_end(fmt_args);
        if (list) free(list);

	if (ret) edg_wll_UpdateError(context,0,"edg_wll_LogEventSync(): ");

	return edg_wll_Error(context,NULL,NULL);
}

/**
 *----------------------------------------------------------------------
 * Formats a logging message and sends it synchronously to L&B Proxy
 * \brief generic synchronous logging function
 * \note simple wrapper around edg_wll_LogEventMaster()
 *----------------------------------------------------------------------
 */
int edg_wll_LogEventProxy(
        edg_wll_Context context,
        edg_wll_EventCode event,
        char *fmt, ...)
{
        int     ret=0;
        char    *list=NULL;
        va_list fmt_args;

        edg_wll_ResetError(context);

        va_start(fmt_args,fmt);
        if (trio_vasprintf(&list,fmt,fmt_args) == -1) {
                edg_wll_SetError(context,ret = ENOMEM,"edg_wll_LogEventProxy(): trio_vasprintf() error");
                goto edg_wll_logevent_end;
        }

        ret=edg_wll_LogEventMaster(context,LOGFLAG_PROXY | LOGFLAG_SYNC, event,"%s",list);

edg_wll_logevent_end:
        va_end(fmt_args);
        if (list) free(list);

        if (ret) edg_wll_UpdateError(context,0,"edg_wll_LogEventProxy(): ");

        return edg_wll_Error(context,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Instructs interlogger to to deliver all pending events related to current job
 * \brief flush events from interlogger
 * \note simple wrapper around edg_wll_LogEventMaster()
 *-----------------------------------------------------------------------
 */
int edg_wll_LogFlush(
        edg_wll_Context context,
        struct timeval *timeout)
{
	int 	ret = 0;
	edg_wll_LogLine out = NULL;
	char 	*fullid;
	char	date[ULM_DATE_STRING_LENGTH+1];
        struct timeval start_time;

	fullid = NULL;

	edg_wll_ResetError(context);

	gettimeofday(&start_time, 0);
	if (edg_wll_ULMTimevalToDate(start_time.tv_sec, start_time.tv_usec, date) != 0) {
		edg_wll_SetError(context,ret = EINVAL,"edg_wll_ULMTimevalToDate()"); 
		goto edg_wll_logflush_end; 
	}
	if (!(fullid = edg_wlc_JobIdUnparse(context->p_jobid))) { 
		ret = edg_wll_SetError(context,EINVAL,"edg_wlc_JobIdUnparse()");
		goto edg_wll_logflush_end;
	}

	if (trio_asprintf(&out, "DATE=%s HOST=\"%|Us\" PROG=internal LVL=system DG.PRIORITY=1 DG.TYPE=\"command\" DG.COMMAND=\"flush\" DG.TIMEOUT=\"%d\" DG.JOBID=\"%s\"\n", 
		    date, context->p_host, (timeout ? timeout->tv_sec : context->p_sync_timeout.tv_sec), fullid) == -1) {
		edg_wll_SetError(context,ret = EINVAL,"trio_asprintf");
		goto edg_wll_logflush_end;
	}

	if (timeout)
		context->p_tmp_timeout = *timeout;
	else
		context->p_tmp_timeout = context->p_sync_timeout;

	ret = edg_wll_DoLogEvent(context, out);

edg_wll_logflush_end:
	if(out) free(out);
	if(fullid) free(fullid);
	
	if (ret) edg_wll_UpdateError(context,0,"edg_wll_LogFlush(): ");
	
	return edg_wll_Error(context,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Instructs interlogger to to deliver all pending events
 * \brief flush all events from interlogger
 * \note simple wrapper around edg_wll_LogEventMaster()
 *-----------------------------------------------------------------------
 */
int edg_wll_LogFlushAll(
        edg_wll_Context context,
        struct timeval *timeout)
{
	int 	ret = 0;
	edg_wll_LogLine out = NULL;
	char	date[ULM_DATE_STRING_LENGTH+1];
        struct timeval start_time;

	edg_wll_ResetError(context);

	gettimeofday(&start_time, 0);
	if (edg_wll_ULMTimevalToDate(start_time.tv_sec, start_time.tv_usec, date) != 0) {
		edg_wll_SetError(context,ret = EINVAL,"edg_wll_ULMTimevalToDate()"); 
		goto edg_wll_logflushall_end; 
	}

	if (trio_asprintf(&out, "DATE=%s HOST=\"%|Us\" PROG=internal LVL=system DG.PRIORITY=1 DG.TYPE=\"command\" DG.COMMAND=\"flush\" DG.TIMEOUT=\"%d\"\n", 
		    date, context->p_host, (timeout ? timeout->tv_sec : context->p_sync_timeout.tv_sec)) == -1) {
		edg_wll_SetError(context,ret = ENOMEM,"trio_asprintf");
		goto edg_wll_logflushall_end;
	}

	if (timeout)
		context->p_tmp_timeout = *timeout;
	else
		context->p_tmp_timeout = context->p_sync_timeout;

	ret = edg_wll_DoLogEvent(context, out);

edg_wll_logflushall_end:
	if(out) free(out);
	
	if (ret) edg_wll_UpdateError(context,0,"edg_wll_LogFlushAll(): ");
	
	return edg_wll_Error(context,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Set a current job for given context.
 * \note Should be called before any logging call.
 *-----------------------------------------------------------------------
 */
int edg_wll_SetLoggingJob(
	edg_wll_Context context,
	const edg_wlc_JobId job,
	const char *code,
	int flags)
{
	int	err;

	edg_wll_ResetError(context);

	if (!job) return edg_wll_SetError(context,EINVAL,"jobid is null");

	edg_wlc_JobIdFree(context->p_jobid);
	if ((err = edg_wlc_JobIdDup(job,&context->p_jobid))) {
		edg_wll_SetError(context,err,"edg_wll_SetLoggingJob(): edg_wlc_JobIdDup() error");
	} else if (!edg_wll_SetSequenceCode(context,code,flags)) {
		edg_wll_IncSequenceCode(context);
	}

	return edg_wll_Error(context,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Set a current job for given context.
 * \note Should be called before any logging call.
 *-----------------------------------------------------------------------
 */
int edg_wll_SetLoggingJobProxy(
        edg_wll_Context context,
        const edg_wlc_JobId job,
        const char *code,
	const char *user,
        int flags)
{
        int     err;
	char	*code_loc = NULL;
	char	*my_subject_name = NULL;
	edg_wll_GssStatus	gss_stat;
	gss_cred_id_t	cred = GSS_C_NO_CREDENTIAL;
	OM_uint32	min_stat;

        edg_wll_ResetError(context);

        if (!job) return edg_wll_SetError(context,EINVAL,"jobid is null");

        edg_wlc_JobIdFree(context->p_jobid);
        if ((err = edg_wlc_JobIdDup(job,&context->p_jobid))) {
                edg_wll_SetError(context,err,"edg_wll_SetLoggingJob(): edg_wlc_JobIdDup() error");
		goto edg_wll_setloggingjobproxy_end;
	}

	/* add user credentials to context */
	if (user) {
		edg_wll_SetParamString(context, EDG_WLL_PARAM_LBPROXY_USER, user);
	} else {
		/* acquire gss credentials */
		err = edg_wll_gss_acquire_cred_gsi(
		      context->p_proxy_filename ? context->p_proxy_filename : context->p_cert_filename,
		      context->p_proxy_filename ? context->p_proxy_filename : context->p_key_filename,
		      &cred, &my_subject_name, &gss_stat);
		/* give up if unable to acquire prescribed credentials */
		if (err && context->p_proxy_filename) {
			edg_wll_SetErrorGss(context, "failed to load GSI credentials", &gss_stat);
			goto edg_wll_setloggingjobproxy_end;
		}
		edg_wll_SetParamString(context, EDG_WLL_PARAM_LBPROXY_USER, my_subject_name);
	}

	/* query LBProxyServer for sequence code if not user-suplied */
/* XXX: don't know if working properly */
	if (!code) {
		if (edg_wll_QuerySequenceCodeProxy(context, job, &code_loc))
			goto edg_wll_setloggingjobproxy_end;	
	} else {
		code_loc = strdup(code);
	}
	
	if (!edg_wll_SetSequenceCode(context,code_loc,flags)) {
		edg_wll_IncSequenceCode(context);
	}
	
edg_wll_setloggingjobproxy_end:
	if (code_loc) free(code_loc);
	if (cred != GSS_C_NO_CREDENTIAL)
		gss_release_cred(&min_stat, &cred);
	if (my_subject_name) free(my_subject_name);

        return edg_wll_Error(context,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Register job with L&B service.
 *-----------------------------------------------------------------------
 */
static int edg_wll_RegisterJobMaster(
        edg_wll_Context         context,
	int			flags,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
	edg_wlc_JobId		parent,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	char	*seq,*type_s,*intseed,*parent_s;
	int	err = 0;
	struct timeval sync_to;

	seq = type_s = intseed = parent_s = NULL;

	edg_wll_ResetError(context);
	memcpy(&sync_to, &context->p_sync_timeout, sizeof sync_to);

	intseed = seed ? strdup(seed) : 
		str2md5base64(seq = edg_wll_GetSequenceCode(context));

	type_s = edg_wll_RegJobJobtypeToString(type);
	if (!type_s) {
		edg_wll_SetError(context,EINVAL,"edg_wll_RegisterJobMaster(): no jobtype specified");
		goto edg_wll_registerjobmaster_end;
	}
	if ((type == EDG_WLL_REGJOB_DAG || 
	     type == EDG_WLL_REGJOB_PARTITIONED ||
	     type == EDG_WLL_REGJOB_COLLECTION)
		&& num_subjobs > 0) {
		err = edg_wll_GenerateSubjobIds(context,job,num_subjobs,intseed,subjobs);
		/* increase log timeout on client (the same as on BK server) */
		context->p_sync_timeout.tv_sec += num_subjobs;
		if (context->p_sync_timeout.tv_sec > 86400) context->p_sync_timeout.tv_sec = 86400;
	}
	if (err) {
		edg_wll_UpdateError(context,EINVAL,"edg_wll_RegisterJobMaster(): edg_wll_GenerateSubjobIds() error");
		goto edg_wll_registerjobmaster_end;
	}
	parent_s = parent ? edg_wlc_JobIdUnparse(parent) : strdup("");

	if (flags & LOGFLAG_DIRECT) {
		/* SetLoggingJob and log directly the message */
		if (edg_wll_SetLoggingJob(context,job,NULL,EDG_WLL_SEQ_NORMAL) == 0) {
			edg_wll_LogEventMaster(context,LOGFLAG_DIRECT | LOGFLAG_SYNC,
				EDG_WLL_EVENT_REGJOB,EDG_WLL_FORMAT_REGJOB,
				(char *)jdl,ns,parent_s,type_s,num_subjobs,intseed);
		}
	} else if (flags & LOGFLAG_PROXY) {
		/* SetLoggingJobProxy and and log to proxy */
		edg_wll_SetSequenceCode(context, NULL, EDG_WLL_SEQ_NORMAL);
		if (seq) free(seq);
		seq = edg_wll_GetSequenceCode(context);
		if (edg_wll_SetLoggingJobProxy(context,job,seq,NULL,EDG_WLL_SEQ_NORMAL) == 0) {
			edg_wll_LogEventMaster(context,LOGFLAG_PROXY | LOGFLAG_SYNC,
				EDG_WLL_EVENT_REGJOB,EDG_WLL_FORMAT_REGJOB,
				(char *)jdl,ns,parent_s,type_s,num_subjobs,intseed);
		}
	} else if (flags & LOGFLAG_NORMAL) {
		/* SetLoggingJob and log normally the message through the locallogger */
		if (edg_wll_SetLoggingJob(context,job,NULL,EDG_WLL_SEQ_NORMAL) == 0) {
			edg_wll_LogEventMaster(context, LOGFLAG_NORMAL,
				EDG_WLL_EVENT_REGJOB,EDG_WLL_FORMAT_REGJOB,
				(char *)jdl,ns,parent_s,type_s,num_subjobs,intseed);
		}
	} else {
		edg_wll_SetError(context,EINVAL,"edg_wll_RegisterJobMaster(): wrong flag specified");
	}

edg_wll_registerjobmaster_end:
	memcpy(&context->p_sync_timeout, &sync_to, sizeof sync_to);
	if (seq) free(seq);
	if (type_s) free(type_s); 
	if (intseed) free(intseed); 
	if (parent_s) free(parent_s);
	return edg_wll_Error(context,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Register synchronously one job with L&B service 
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobSync(
        edg_wll_Context         context,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	return edg_wll_RegisterJobMaster(context,LOGFLAG_DIRECT,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs);
}

/**
 *-----------------------------------------------------------------------
 * Register (asynchronously) one job with L&B service 
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJob(
        edg_wll_Context         context,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	return edg_wll_RegisterJobMaster(context,LOGFLAG_DIRECT,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs);
}

#ifdef LB_PERF

/**
 *-----------------------------------------------------------------------
 * Register one job with L&B Proxy service 
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 * this is new (LB_PERF) edg_wll_RegisterJobProxy 
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobProxy(
        edg_wll_Context         context,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
#define	MY_SEED	"edg_wll_RegisterJobProxy()"
	char	*seq,*type_s;
	edg_wll_LogLine	logline = NULL;
	int	ret = 0,n,count,fd;
	struct timeval sync_to;
	edg_wll_GssConnection   con_bkserver;
	edg_wll_PlainConnection con_lbproxy;
	fd_set fdset;

	seq = type_s = NULL;

	edg_wll_ResetError(context);
	memcpy(&sync_to, &context->p_sync_timeout, sizeof sync_to);
	memset(&con_bkserver, 0, sizeof(con_bkserver));
	memset(&con_lbproxy, 0, sizeof(con_lbproxy));
	FD_ZERO(&fdset);

	type_s = edg_wll_RegJobJobtypeToString(type);
	if (!type_s) {
		edg_wll_SetError(context,EINVAL,"edg_wll_RegisterJobProxy(): no jobtype specified");
		goto edg_wll_registerjobproxy_end;
	}
	if ((type == EDG_WLL_REGJOB_DAG || 
	     type == EDG_WLL_REGJOB_PARTITIONED ||
	     type == EDG_WLL_REGJOB_COLLECTION)
		&& num_subjobs > 0) {
		ret = edg_wll_GenerateSubjobIds(context,job,num_subjobs,seed ? seed : MY_SEED,subjobs);
		/* increase log timeout on client (the same as on BK server) */
		context->p_sync_timeout.tv_sec += num_subjobs;
		if (context->p_sync_timeout.tv_sec > 86400) context->p_sync_timeout.tv_sec = 86400;
	}
	if (ret) {
		edg_wll_UpdateError(context,EINVAL,"edg_wll_RegisterJobProxy(): edg_wll_GenerateSubjobIds() error");
		goto edg_wll_registerjobproxy_end;
	}

	/* SetLoggingJobProxy */
	edg_wll_SetSequenceCode(context, NULL, EDG_WLL_SEQ_NORMAL);
	seq = edg_wll_GetSequenceCode(context);
	if (edg_wll_SetLoggingJobProxy(context,job,seq,NULL,EDG_WLL_SEQ_NORMAL) != 0) {
		edg_wll_UpdateError(context,EINVAL,"edg_wll_RegisterJobProxy(): edg_wll_SetLoggingJobProxy() error");
		goto edg_wll_registerjobproxy_end; 
	}

	/* format the RegJob event message */
	if (edg_wll_FormatLogLine(context,LOGFLAG_SYNC,EDG_WLL_EVENT_REGJOB,&logline,EDG_WLL_FORMAT_REGJOB,
		(char *)jdl,ns,"",type_s,num_subjobs,seed ? seed : MY_SEED) != 0 ) {
		edg_wll_UpdateError(context,EINVAL,"edg_wll_RegisterJobProxy(): edg_wll_FormatLogLine() error");
		goto edg_wll_registerjobproxy_end; 
	}       

   /* and now do the pseudo-parallel registration: */

	/* connect to bkserver */
	if ((ret = edg_wll_log_direct_connect(context,&con_bkserver)) < 0) {
		edg_wll_UpdateError(context,EAGAIN,"edg_wll_RegisterJobProxy(): edg_wll_log_direct_connect error");
		goto edg_wll_registerjobproxy_end; 
	}
	/* connect to lbproxy */
	if ((ret = edg_wll_log_proxy_connect(context,&con_lbproxy)) < 0) {
		edg_wll_UpdateError(context,EAGAIN,"edg_wll_RegisterJobProxy(): edg_wll_log_proxy_connect error");
		goto edg_wll_registerjobproxy_end; 
	}
	/* send to bkserver */
/*
	if ((ret = edg_wll_log_direct_write(context,&con_bkserver,logline)) == -1) {
		edg_wll_UpdateError(context,EAGAIN,"edg_wll_RegisterJobProxy(): edg_wll_log_direct_write error");
		goto edg_wll_registerjobproxy_end; 
	}
*/
	/* send to lbproxy */
	if ((ret = edg_wll_log_proxy_write(context,&con_lbproxy,logline)) == -1) {
		edg_wll_UpdateError(context,EAGAIN,"edg_wll_RegisterJobProxy(): edg_wll_log_proxy_write error");
		goto edg_wll_registerjobproxy_end; 
	}
	/* select and read the answers */
	count = 2;
	while (count > 0) {
		FD_SET(con_bkserver.sock,&fdset);
		n = con_bkserver.sock;
		FD_SET(con_lbproxy.sock,&fdset);
		if (con_lbproxy.sock > n) n = con_lbproxy.sock;
		n += 1;
		fd = select(n,&fdset,NULL,NULL,&context->p_tmp_timeout);
		switch (fd) {
		case 0: /* timeout */
			edg_wll_UpdateError(context,EAGAIN,"edg_wll_RegisterJobProxy(): select() timeouted");
			count = 0;
			break;
		case -1: /* error */
			switch(errno) {
			case EINTR:
				continue;
			default:
				edg_wll_UpdateError(context,errno,"edg_wll_RegisterJobProxy(): select() error"); 
			}
		default:
			break;
		}
		if (FD_ISSET(con_bkserver.sock,&fdset)) {
			/* read answer from bkserver */
			if ((ret = edg_wll_log_direct_read(context,&con_bkserver)) == -1) {
				edg_wll_UpdateError(context,EAGAIN,"edg_wll_RegisterJobProxy(): edg_wll_log_direct_read error");
				goto edg_wll_registerjobproxy_end; 
			}
			count -= 1;
		}	
		if (FD_ISSET(con_lbproxy.sock,&fdset)) {
			/* read answer from lbproxy */
			if ((ret = edg_wll_log_proxy_read(context,&con_lbproxy)) == -1) {
				edg_wll_UpdateError(context,EAGAIN,"edg_wll_RegisterJobProxy(): edg_wll_log_proxy_read error");
				goto edg_wll_registerjobproxy_end; 
			}
			count -= 1;
		}	
	}

edg_wll_registerjobproxy_end:
	if (con_bkserver.sock) edg_wll_gss_close(&con_bkserver,&context->p_tmp_timeout);
	if (con_lbproxy.sock) edg_wll_plain_close(&con_lbproxy);

	memcpy(&context->p_sync_timeout, &sync_to, sizeof sync_to);
	if (type_s) free(type_s);
	if (seq) free(seq);
	if (logline) free(logline);

	return edg_wll_Error(context,NULL,NULL);
#undef MY_SEED
}

#else /* LB_PERF */

/**
 *-----------------------------------------------------------------------
 * Register one job with L&B Proxy service 
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 * this is original edg_wll_RegisterJobProxy 
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobProxy(
        edg_wll_Context         context,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
#define	MY_SEED	"edg_wll_RegisterJobProxy()"
	/* first register with bkserver */
	int ret = edg_wll_RegisterJobMaster(context,LOGFLAG_DIRECT,job,type,jdl,ns,NULL,num_subjobs,seed ? seed : MY_SEED,subjobs);
	if (ret) {
		edg_wll_UpdateError(context,0,"edg_wll_RegisterJobProxy(): unable to register with bkserver");
		return edg_wll_Error(context,NULL,NULL);
	}
	/* and then with L&B Proxy */
	return edg_wll_RegisterJobMaster(context,LOGFLAG_PROXY,job,type,jdl,ns,NULL,num_subjobs,seed ? seed : MY_SEED,subjobs);
#undef MY_SEED
}

#endif /* LB_PERF */


#ifdef LB_PERF

/**
 *-----------------------------------------------------------------------
 * Register one job with L&B Proxy service ONLY
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 * useful for performace measuring
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobProxyOnly(
        edg_wll_Context         context,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
#define	MY_SEED	"edg_wll_RegisterJobProxyOnly()"
	return edg_wll_RegisterJobMaster(context,LOGFLAG_PROXY,job,type,jdl,ns,NULL,num_subjobs,seed ? seed : MY_SEED,subjobs);
#undef	MY_SEED
}

#endif /* LB_PERF */

/**
 *-----------------------------------------------------------------------
 * Register one subjob with L&B service
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 *-----------------------------------------------------------------------
 */
static
int edg_wll_RegisterSubjob(
        edg_wll_Context         context,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
	edg_wlc_JobId		parent,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	return edg_wll_RegisterJobMaster(context,LOGFLAG_NORMAL,job,type,jdl,ns,parent,num_subjobs,seed,subjobs);
}

/**
 *-----------------------------------------------------------------------
 * Register one subjob with L&B Proxy service
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 *-----------------------------------------------------------------------
 */
static
int edg_wll_RegisterSubjobProxy(
        edg_wll_Context         context,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
	edg_wlc_JobId		parent,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	return edg_wll_RegisterJobMaster(context,LOGFLAG_PROXY,job,type,jdl,ns,parent,num_subjobs,seed,subjobs);
}

/**
 *-----------------------------------------------------------------------
 * Register batch of subjobs with L&B service
 * \note simple wrapper around edg_wll_RegisterSubjob()
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterSubjobs(
	edg_wll_Context 	ctx,
	const edg_wlc_JobId 	parent,
	char const * const * 	jdls, 
	const char * 		ns, 
	edg_wlc_JobId const * 	subjobs)
{
	char const * const	*pjdl;
	edg_wlc_JobId const	*psubjob;
	edg_wlc_JobId		oldctxjob;
	char *			oldctxseq;
	int                     errcode = 0;
	char *                  errdesc = NULL;

	if (edg_wll_GetLoggingJob(ctx, &oldctxjob)) return edg_wll_Error(ctx, NULL, NULL);
	oldctxseq = edg_wll_GetSequenceCode(ctx);

	pjdl = jdls;
	psubjob = subjobs;
	
	while (*pjdl != NULL) {
		if (edg_wll_RegisterSubjob(ctx, *psubjob, EDG_WLL_REGJOB_SIMPLE, *pjdl,
						ns, parent, 0, NULL, NULL) != 0) {
			errcode = edg_wll_Error(ctx, NULL, &errdesc);
			goto edg_wll_registersubjobs_end;
		}
		pjdl++; psubjob++;
	}

edg_wll_registersubjobs_end:
	edg_wll_SetLoggingJob(ctx, oldctxjob, oldctxseq, EDG_WLL_SEQ_NORMAL);

	if (errcode) {
                edg_wll_SetError(ctx, errcode, errdesc);
                free(errdesc);
        }
	return edg_wll_Error(ctx, NULL, NULL);
}

/**
 *-----------------------------------------------------------------------
 * Register batch of subjobs with L&B Proxy service
 * \note simple wrapper around edg_wll_RegisterSubjobProxy()
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterSubjobsProxy(
	edg_wll_Context 	ctx,
	const edg_wlc_JobId 	parent,
	char const * const * 	jdls, 
	const char * 		ns, 
	edg_wlc_JobId const * 	subjobs)
{
	char const * const	*pjdl;
	edg_wlc_JobId const	*psubjob;
	edg_wlc_JobId		oldctxjob;
	char *			oldctxseq;
	int                     errcode = 0;
	char *                  errdesc = NULL;

	if (edg_wll_GetLoggingJob(ctx, &oldctxjob)) return edg_wll_Error(ctx, NULL, NULL);
	oldctxseq = edg_wll_GetSequenceCode(ctx);

	pjdl = jdls;
	psubjob = subjobs;
	
	while (*pjdl != NULL) {
		if (edg_wll_RegisterSubjobProxy(ctx, *psubjob, EDG_WLL_REGJOB_SIMPLE, *pjdl,
						ns, parent, 0, NULL, NULL) != 0) {
			errcode = edg_wll_Error(ctx, NULL, &errdesc);
			goto edg_wll_registersubjobsproxy_end;
		}
		pjdl++; psubjob++;
	}

edg_wll_registersubjobsproxy_end:
	edg_wll_SetLoggingJobProxy(ctx, oldctxjob, oldctxseq, NULL, EDG_WLL_SEQ_NORMAL);

	if (errcode) {
                edg_wll_SetError(ctx, errcode, errdesc);
                free(errdesc);
        }
	return edg_wll_Error(ctx, NULL, NULL);
}

/**
 *-----------------------------------------------------------------------
 * Change ACL for given job
 *-----------------------------------------------------------------------
 */
int edg_wll_ChangeACL(
		edg_wll_Context			ctx,
		const edg_wlc_JobId		jobid,
		const char			*user_id,
		enum edg_wll_UserIdType		user_id_type,
		enum edg_wll_Permission		permission,
		enum edg_wll_PermissionType	permission_type,
		enum edg_wll_ACLOperation	operation)
{
	if ( edg_wll_SetLoggingJob(ctx, jobid, NULL, EDG_WLL_SEQ_NORMAL) == 0 ) {
		edg_wll_LogEventMaster(ctx, LOGFLAG_NORMAL | LOGFLAG_SYNC, 
			EDG_WLL_EVENT_CHANGEACL, EDG_WLL_FORMAT_CHANGEACL,
			user_id, user_id_type, permission, permission_type, operation);
	}

	return edg_wll_Error(ctx,NULL,NULL);
}
