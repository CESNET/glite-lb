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

/* XXX: paralel registration is disabled until the race condition (via proxy first)
 * job owner assignment is solved */

#define LB_SERIAL_REG

#ifdef FAKE_VERSION
int edg_wll_DoLogEvent(edg_wll_Context ctx, edg_wll_LogLine logline);
int edg_wll_DoLogEventProxy(edg_wll_Context ctx, edg_wll_LogLine logline);
int edg_wll_DoLogEventDirect(edg_wll_Context ctx, edg_wll_LogLine logline);
#else

/**
 *----------------------------------------------------------------------
 * handle_errors - handle answers from logging functions
 *----------------------------------------------------------------------
 */
static
int handle_errors(edg_wll_Context ctx, int code, const char *text)
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
                        edg_wll_UpdateError(ctx,EINVAL,err);
                        break;
		case EDG_WLL_IL_PROTO:
		case EDG_WLL_IL_SYS:
		case EDG_WLL_IL_EVENTS_WAITING:
                        snprintf(err, sizeof(err), "%s: Error code mapped to EAGAIN", text);
                        edg_wll_UpdateError(ctx,EAGAIN,err);
			break;
		default:
                        snprintf(err, sizeof(err), "%s: Error code mapped to EAGAIN", text);
                        edg_wll_UpdateError(ctx,EAGAIN,err);
			break;
	}

	return edg_wll_Error(ctx, NULL, NULL);
}

/**
 *----------------------------------------------------------------------
 * Open a GSS connection to local-logger, send already formatted ULM string  
 *   and get answer back from local-logger
 * \brief connect to local-logger, send message and get answer back
 * \param[in,out] ctx		context to work with,
 * \param[in] logline		formated ULM string
 *----------------------------------------------------------------------
 */
int edg_wll_DoLogEvent(
	edg_wll_Context ctx,
	edg_wll_LogLine logline)
{
	int	ret = 0, answer = EAGAIN;
        int	conn = -1;
	int	attempt = 1;

        edg_wll_ResetError(ctx);
        memset(&conn,0,sizeof(conn));

	do {
		/* connect to local-logger */
		if ((ret = edg_wll_log_connect(ctx,&conn))) {
			edg_wll_UpdateError(ctx,EDG_WLL_IL_PROTO,"edg_wll_DoLogEvent(): edg_wll_log_connect error");
			goto edg_wll_DoLogEvent_end;
		}
	
		/* send message */
		if ((ret = edg_wll_log_write(ctx,conn,logline)) == -1) {
			answer = edg_wll_Error(ctx, NULL, NULL);
			edg_wll_UpdateError(ctx,EDG_WLL_IL_PROTO,"edg_wll_DoLogEvent(): edg_wll_log_write error");
			goto edg_wll_DoLogEvent_end;
		}
	
		/* get answer */
		ret = edg_wll_log_read(ctx,conn);
		answer = edg_wll_Error(ctx, NULL, NULL);
		if (ret == -1) 
			edg_wll_UpdateError(ctx,EDG_WLL_IL_PROTO,"edg_wll_DoLogEvent(): edg_wll_log_read error");
	
	edg_wll_DoLogEvent_end:
		if (ret == -1 && conn >= 0) edg_wll_log_close(ctx,conn);

	} while (++attempt <= 2 && (answer == ENOTCONN || answer == EPIPE));

	return handle_errors(ctx,answer,"edg_wll_DoLogEvent()");
}

/**
 *----------------------------------------------------------------------
 * Open a plain (UNIX socket) connection to L&B Proxy, send already formatted ULM string
 *   and get answer back from  L&B Proxy
 * \brief connect to lbproxy, send message and get answer back
 * \param[in,out] ctx		context to work with,
 * \param[in] logline		formated ULM string
 *----------------------------------------------------------------------
 */
int edg_wll_DoLogEventProxy(
	edg_wll_Context ctx,
	edg_wll_LogLine logline)
{
	int	ret = 0, answer = EAGAIN;
	edg_wll_PlainConnection conn;

	edg_wll_ResetError(ctx);
	memset(&conn,0,sizeof(conn));

	/* connect to lbproxy */
	if ((ret = edg_wll_log_proxy_connect(ctx,&conn))) {
		edg_wll_UpdateError(ctx,EDG_WLL_IL_PROTO,"edg_wll_DoLogEventProxy(): edg_wll_log_proxy_write error");
		goto edg_wll_DoLogEventProxy_end;
	}

	/* send message */
	if ((ret = edg_wll_log_proxy_write(ctx,&conn,logline)) == -1) {
		edg_wll_UpdateError(ctx,EDG_WLL_IL_PROTO,"edg_wll_DoLogEventProxy(): edg_wll_log_proxy_write error");
		goto edg_wll_DoLogEventProxy_end;
	}

	/* get answer */
	if ((ret = edg_wll_log_proxy_read(ctx,&conn)) == -1) {
		edg_wll_UpdateError(ctx,EDG_WLL_IL_PROTO,"edg_wll_DoLogEventProxy(): edg_wll_log_proxy_read error");
	} else {
		answer = edg_wll_Error(ctx, NULL, NULL);
	}

edg_wll_DoLogEventProxy_end:
	edg_wll_log_proxy_close(ctx,&conn);

	return handle_errors(ctx,answer,"edg_wll_DoLogEventProxy()");
}

/**
 *----------------------------------------------------------------------
 * Open a GSS connection to L&B server, send already formatted ULM string  
 *   and get answer back from L&B server
 * \brief connect to bkserver, send message and get answer back
 * \param[in,out] ctx		context to work with,
 * \param[in] logline		formated ULM string
 *----------------------------------------------------------------------
 */
int edg_wll_DoLogEventDirect(
	edg_wll_Context ctx,
	edg_wll_LogLine logline)
{
	int 	ret = 0, answer = EAGAIN;
	edg_wll_GssConnection	conn;

	edg_wll_ResetError(ctx);
	memset(&conn,0,sizeof(conn));

	/* connect to bkserver */
	if ((ret = edg_wll_log_direct_connect(ctx,&conn))) {
		edg_wll_UpdateError(ctx,EDG_WLL_IL_PROTO,"edg_wll_DoLogEventDirect(): edg_wll_log_direct_connect error");
		goto edg_wll_DoLogEventDirect_end;
	}

	/* send message */
	if ((ret = edg_wll_log_direct_write(ctx,&conn,logline)) == -1) {
		edg_wll_UpdateError(ctx,EDG_WLL_IL_PROTO,"edg_wll_DoLogEventDirect(): edg_wll_log_direct_write error");
		goto edg_wll_DoLogEventDirect_end;
        }

	/* get answer */
	if ((ret = edg_wll_log_direct_read(ctx,&conn)) == -1) {
		edg_wll_UpdateError(ctx,EDG_WLL_IL_PROTO,"edg_wll_DoLogEventDirect(): edg_wll_log_direct_read error");
	} else {
		answer = edg_wll_Error(ctx, NULL, NULL);
	}

edg_wll_DoLogEventDirect_end:
	edg_wll_log_direct_close(ctx,&conn);

	return handle_errors(ctx,answer,"edg_wll_DoLogEventDirect()");
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
 * \param[in,out] ctx	context to work with,
 * \param[in] flags		as defined by LOGFLAG_*
 * \param[in] event		type of the event,
 * \param[out] logline		formated logging message
 * \param[in] fmt		printf()-like format string,
 * \param[in] ...		event specific values/data according to fmt.
 *----------------------------------------------------------------------
 */
static int edg_wll_FormatLogLine(
	edg_wll_Context ctx,
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

	edg_wll_ResetError(ctx);

   /* format the message: */
	va_start(fmt_args,fmt);

	gettimeofday(&start_time,0);
	if (edg_wll_ULMTimevalToDate(start_time.tv_sec,start_time.tv_usec,date) != 0) {
		edg_wll_SetError(ctx,ret = EINVAL,"edg_wll_FormatLogLine(): edg_wll_ULMTimevalToDate() error"); 
		goto edg_wll_formatlogline_end; 
	}
 	source = edg_wll_SourceToString(ctx->p_source);
	lvl = edg_wll_LevelToString(ctx->p_level);
	eventName = edg_wll_EventToString(event);
	if (!eventName) { 
		edg_wll_SetError(ctx,ret = EINVAL,"edg_wll_FormatLogLine(): event name not specified"); 
		goto edg_wll_formatlogline_end; 
	}
	if (!(fullid = edg_wlc_JobIdUnparse(ctx->p_jobid))) { 
		edg_wll_SetError(ctx,ret = EINVAL,"edg_wll_FormatLogLine(): edg_wlc_JobIdUnparse() error"); 
		goto edg_wll_formatlogline_end;
	}
	seq = edg_wll_GetSequenceCode(ctx);

	if (trio_asprintf(&fix,EDG_WLL_FORMAT_COMMON,
			date,ctx->p_host,lvl,priority,
			source,ctx->p_instance ? ctx->p_instance : "",
			eventName,fullid,seq) == -1) {
		edg_wll_SetError(ctx,ret = ENOMEM,"edg_wll_FormatLogLine(): trio_asprintf() error"); 
		goto edg_wll_formatlogline_end; 
	}
	/* TODO: add always, probably new ctx->p_user */
	if ( ( (flags & LOGFLAG_PROXY) || (flags & LOGFLAG_DIRECT) ) && 
	   (ctx->p_user_lbproxy) ) {
		if (trio_asprintf(&dguser,EDG_WLL_FORMAT_USER,ctx->p_user_lbproxy) == -1) {
			edg_wll_SetError(ctx,ret = ENOMEM,"edg_wll_FormatLogLine(): trio_asprintf() error"); 
			goto edg_wll_formatlogline_end; 
		}
	} else {
		dguser = strdup("");
	}
	if (trio_vasprintf(&var,fmt,fmt_args) == -1) { 
		edg_wll_SetError(ctx,ret = ENOMEM,"edg_wll_FormatLogLine(): trio_vasprintf() error"); 
		goto edg_wll_formatlogline_end; 
	}
	if (asprintf(&out,"%s%s%s\n",fix,dguser,var) == -1) { 
		edg_wll_SetError(ctx,ret = ENOMEM,"edg_wll_FormatLogLine(): asprintf() error"); 
		goto edg_wll_formatlogline_end; 
	}
	size = strlen(out);

	if (priority && (size > EDG_WLL_LOG_SYNC_MAXMSGSIZE)) {
		edg_wll_SetError(ctx,ret = ENOSPC,"edg_wll_FormatLogLine(): Message size too large for synchronous transfer");
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

	return edg_wll_Error(ctx,NULL,NULL);
}

/**
 *----------------------------------------------------------------------
 * Formats a logging message and sends it to local-logger
 * \brief master logging event function
 * \param[in,out] ctx		context to work with,
 * \param[in] flags		as defined by LOGFLAG_*
 * \param[in] event		type of the event,
 * \param[in] fmt		printf()-like format string,
 * \param[in] ...		event specific values/data according to fmt.
 *----------------------------------------------------------------------
 */
static int edg_wll_LogEventMaster(
	edg_wll_Context ctx,
	int flags,
	edg_wll_EventCode event,
	char *fmt, ...)
{
	va_list	fmt_args;
	int	priority;
	int	ret;
	edg_wll_LogLine in = NULL, out = NULL;

	priority = flags & LOGFLAG_SYNC;

	edg_wll_ResetError(ctx);

   /* default return value is "Try Again" */
	ret = EAGAIN; 

   /* format the message: */
	va_start(fmt_args,fmt);

	if (trio_vasprintf(&in,fmt,fmt_args) == -1) {
		edg_wll_UpdateError(ctx,ret = ENOMEM,"edg_wll_LogEventMaster(): trio_vasprintf() error");
		goto edg_wll_logeventmaster_end; 
	}

	if (edg_wll_FormatLogLine(ctx,flags,event,&out,"%s",in) != 0 ) {
		edg_wll_UpdateError(ctx,ret = EINVAL,"edg_wll_LogEventMaster(): edg_wll_FormatLogLine() error"); 
		goto edg_wll_logeventmaster_end; 
	}

#ifdef EDG_WLL_LOG_STUB
//	fprintf(stderr,"edg_wll_LogEventMaster (%d chars): %s",strlen(out),out);
#endif
	
	ctx->p_tmp_timeout.tv_sec = 0;
	ctx->p_tmp_timeout.tv_usec = 0;
	if (priority) {
		ctx->p_tmp_timeout = ctx->p_sync_timeout;
	}
	else {
		ctx->p_tmp_timeout = ctx->p_log_timeout;
	}

   /* and send the message */ 
#ifndef LB_PERF_DROP
	if (flags & LOGFLAG_NORMAL) {
		/* to the local-logger: */
		ret = edg_wll_DoLogEvent(ctx, out);
	} else if (flags & LOGFLAG_PROXY) {
		/* to the L&B Proxy: */
		ret = edg_wll_DoLogEventProxy(ctx, out);
	} else if (flags & LOGFLAG_DIRECT) {
		/* directly to the bkserver: */
		ret = edg_wll_DoLogEventDirect(ctx, out);
	} else {
		edg_wll_SetError(ctx,ret = EINVAL,"edg_wll_LogEventMaster(): wrong flag specified");
	}
#endif

edg_wll_logeventmaster_end:
	va_end(fmt_args);
	if (in) free(in);
	if (out) free(out);

	if (!ret) if(edg_wll_IncSequenceCode(ctx)) {
		edg_wll_SetError(ctx,ret = EINVAL,"edg_wll_LogEventMaster(): edg_wll_IncSequenceCode failed");
	}

	if (ret) edg_wll_UpdateError(ctx,0,"Logging library ERROR: ");

	return edg_wll_Error(ctx,NULL,NULL);
}


/**
 *----------------------------------------------------------------------
 * Formats a logging message and sends it asynchronously to local-logger
 * \brief generic asynchronous logging function
 *----------------------------------------------------------------------
 */
int edg_wll_LogEvent(
        edg_wll_Context ctx,
        edg_wll_EventCode event,
        char *fmt, ...)
{
	int	ret=0;
	char	*list=NULL;
	va_list	fmt_args;

	edg_wll_ResetError(ctx);

	va_start(fmt_args,fmt);
	if (trio_vasprintf(&list,fmt,fmt_args) == -1) { 
		edg_wll_SetError(ctx,ret = ENOMEM,"edg_wll_LogEvent(): trio_vasprintf() error"); 
		goto edg_wll_logevent_end; 
	}

	ret=edg_wll_LogEventMaster(ctx,LOGFLAG_NORMAL | LOGFLAG_ASYNC,event,"%s",list);

edg_wll_logevent_end:
	va_end(fmt_args);
        if (list) free(list);

	if (ret) edg_wll_UpdateError(ctx,0,"edg_wll_LogEvent(): ");

	return edg_wll_Error(ctx,NULL,NULL);
}

/**
 *----------------------------------------------------------------------
 * Formats a logging message and sends it synchronously to local-logger
 * \brief generic synchronous logging function
 * \note simple wrapper around edg_wll_LogEventMaster()
 *----------------------------------------------------------------------
 */
int edg_wll_LogEventSync(
        edg_wll_Context ctx,
        edg_wll_EventCode event,
        char *fmt, ...)
{
	int	ret=0;
	char	*list=NULL;
	va_list	fmt_args;

	edg_wll_ResetError(ctx);

	va_start(fmt_args,fmt);
	if (trio_vasprintf(&list,fmt,fmt_args) == -1) { 
		edg_wll_SetError(ctx,ret = ENOMEM,"edg_wll_LogEventSync(): trio_vasprintf() error"); 
		goto edg_wll_logeventsync_end; 
	}

	ret=edg_wll_LogEventMaster(ctx,LOGFLAG_NORMAL | LOGFLAG_SYNC,event,"%s",list);

edg_wll_logeventsync_end:
	va_end(fmt_args);
        if (list) free(list);

	if (ret) edg_wll_UpdateError(ctx,0,"edg_wll_LogEventSync(): ");

	return edg_wll_Error(ctx,NULL,NULL);
}

/**
 *----------------------------------------------------------------------
 * Formats a logging message and sends it synchronously to L&B Proxy
 * \brief generic synchronous logging function
 * \note simple wrapper around edg_wll_LogEventMaster()
 *----------------------------------------------------------------------
 */
int edg_wll_LogEventProxy(
        edg_wll_Context ctx,
        edg_wll_EventCode event,
        char *fmt, ...)
{
        int     ret=0;
        char    *list=NULL;
        va_list fmt_args;

        edg_wll_ResetError(ctx);

        va_start(fmt_args,fmt);
        if (trio_vasprintf(&list,fmt,fmt_args) == -1) {
                edg_wll_SetError(ctx,ret = ENOMEM,"edg_wll_LogEventProxy(): trio_vasprintf() error");
                goto edg_wll_logevent_end;
        }

        ret=edg_wll_LogEventMaster(ctx,LOGFLAG_PROXY | LOGFLAG_SYNC, event,"%s",list);

edg_wll_logevent_end:
        va_end(fmt_args);
        if (list) free(list);

        if (ret) edg_wll_UpdateError(ctx,0,"edg_wll_LogEventProxy(): ");

        return edg_wll_Error(ctx,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Instructs interlogger to to deliver all pending events related to current job
 * \brief flush events from interlogger
 * \note simple wrapper around edg_wll_LogEventMaster()
 *-----------------------------------------------------------------------
 */
int edg_wll_LogFlush(
        edg_wll_Context ctx,
        struct timeval *timeout)
{
	int 	ret = 0;
	edg_wll_LogLine out = NULL;
	char 	*fullid;
	char	date[ULM_DATE_STRING_LENGTH+1];
        struct timeval start_time;

	fullid = NULL;

	edg_wll_ResetError(ctx);

	gettimeofday(&start_time, 0);
	if (edg_wll_ULMTimevalToDate(start_time.tv_sec, start_time.tv_usec, date) != 0) {
		edg_wll_SetError(ctx,ret = EINVAL,"edg_wll_LogFlush(): edg_wll_ULMTimevalToDate() error"); 
		goto edg_wll_logflush_end; 
	}
	if (!(fullid = edg_wlc_JobIdUnparse(ctx->p_jobid))) { 
		ret = edg_wll_SetError(ctx,EINVAL,"edg_wll_LogFlush(): edg_wlc_JobIdUnparse() error");
		goto edg_wll_logflush_end;
	}

	if (trio_asprintf(&out, "DATE=%s HOST=\"%|Us\" PROG=internal LVL=system DG.PRIORITY=1 DG.TYPE=\"command\" DG.COMMAND=\"flush\" DG.TIMEOUT=\"%d\" DG.JOBID=\"%s\"\n", 
		    date, ctx->p_host, (timeout ? timeout->tv_sec : ctx->p_sync_timeout.tv_sec), fullid) == -1) {
		edg_wll_SetError(ctx,ret = EINVAL,"edg_wll_LogFlush(): trio_asprintf() error");
		goto edg_wll_logflush_end;
	}

	if (timeout)
		ctx->p_tmp_timeout = *timeout;
	else
		ctx->p_tmp_timeout = ctx->p_sync_timeout;

	ret = edg_wll_DoLogEvent(ctx, out);

edg_wll_logflush_end:
	if(out) free(out);
	if(fullid) free(fullid);
	
	if (ret) edg_wll_UpdateError(ctx,0,"edg_wll_LogFlush(): ");
	
	return edg_wll_Error(ctx,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Instructs interlogger to to deliver all pending events
 * \brief flush all events from interlogger
 * \note simple wrapper around edg_wll_LogEventMaster()
 *-----------------------------------------------------------------------
 */
int edg_wll_LogFlushAll(
        edg_wll_Context ctx,
        struct timeval *timeout)
{
	int 	ret = 0;
	edg_wll_LogLine out = NULL;
	char	date[ULM_DATE_STRING_LENGTH+1];
        struct timeval start_time;

	edg_wll_ResetError(ctx);

	gettimeofday(&start_time, 0);
	if (edg_wll_ULMTimevalToDate(start_time.tv_sec, start_time.tv_usec, date) != 0) {
		edg_wll_SetError(ctx,ret = EINVAL,"edg_wll_LogFlushAll(): edg_wll_ULMTimevalToDate() error"); 
		goto edg_wll_logflushall_end; 
	}

	if (trio_asprintf(&out, "DATE=%s HOST=\"%|Us\" PROG=internal LVL=system DG.PRIORITY=1 DG.TYPE=\"command\" DG.COMMAND=\"flush\" DG.TIMEOUT=\"%d\"\n", 
		    date, ctx->p_host, (timeout ? timeout->tv_sec : ctx->p_sync_timeout.tv_sec)) == -1) {
		edg_wll_SetError(ctx,ret = ENOMEM,"edg_wll_LogFlushAll(): trio_asprintf() error");
		goto edg_wll_logflushall_end;
	}

	if (timeout)
		ctx->p_tmp_timeout = *timeout;
	else
		ctx->p_tmp_timeout = ctx->p_sync_timeout;

	ret = edg_wll_DoLogEvent(ctx, out);

edg_wll_logflushall_end:
	if(out) free(out);
	
	if (ret) edg_wll_UpdateError(ctx,0,"edg_wll_LogFlushAll(): ");
	
	return edg_wll_Error(ctx,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Set a current job for given context.
 * \note Should be called before any logging call.
 *-----------------------------------------------------------------------
 */
int edg_wll_SetLoggingJob(
	edg_wll_Context ctx,
	const edg_wlc_JobId job,
	const char *code,
	int flags)
{
	int	err;

	edg_wll_ResetError(ctx);

	if (!job) return edg_wll_SetError(ctx,EINVAL,"edg_wll_SetLoggingJob(): jobid is null");

	edg_wlc_JobIdFree(ctx->p_jobid);
	if ((err = edg_wlc_JobIdDup(job,&ctx->p_jobid))) {
		edg_wll_SetError(ctx,err,"edg_wll_SetLoggingJob(): edg_wlc_JobIdDup() error");
	} else if (!edg_wll_SetSequenceCode(ctx,code,flags)) {
		edg_wll_IncSequenceCode(ctx);
	}

	/* add user credentials to context */
	{
		char	*my_subject_name = NULL;
		edg_wll_GssStatus	gss_stat;
		gss_cred_id_t	cred = GSS_C_NO_CREDENTIAL;
		OM_uint32	min_stat;

		/* acquire gss credentials */
		err = edg_wll_gss_acquire_cred_gsi(
		      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename,
		      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_key_filename,
		      &cred, &my_subject_name, &gss_stat);
		/* give up if unable to acquire prescribed credentials */
		if (err && ctx->p_proxy_filename) {
			edg_wll_SetErrorGss(ctx, "failed to load GSI credentials", &gss_stat);
			edg_wll_SetParamString(ctx, EDG_WLL_PARAM_LBPROXY_USER, EDG_WLL_LOG_USER_DEFAULT);
		} else {
			edg_wll_SetParamString(ctx, EDG_WLL_PARAM_LBPROXY_USER, my_subject_name);
		}
		if (cred != GSS_C_NO_CREDENTIAL)
			gss_release_cred(&min_stat, &cred);
		if (my_subject_name) free(my_subject_name);
	}

	return edg_wll_Error(ctx,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Set a current job for given context.
 * \note Should be called before any logging call.
 *-----------------------------------------------------------------------
 */
int edg_wll_SetLoggingJobProxy(
        edg_wll_Context ctx,
        const edg_wlc_JobId job,
        const char *code,
	const char *user,
        int flags)
{
        int     err;
	char	*code_loc = NULL;

        edg_wll_ResetError(ctx);

        if (!job) return edg_wll_SetError(ctx,EINVAL,"edg_wll_SetLoggingJobProxy(): jobid is null");

        edg_wlc_JobIdFree(ctx->p_jobid);
        if ((err = edg_wlc_JobIdDup(job,&ctx->p_jobid))) {
                edg_wll_SetError(ctx,err,"edg_wll_SetLoggingJobProxy(): edg_wlc_JobIdDup() error");
		goto edg_wll_setloggingjobproxy_end;
	}

	/* add user credentials to context */
	if (user) {
		edg_wll_SetParamString(ctx, EDG_WLL_PARAM_LBPROXY_USER, user);
	} else {
		char	*my_subject_name = NULL;
		edg_wll_GssStatus	gss_stat;
		gss_cred_id_t	cred = GSS_C_NO_CREDENTIAL;
		OM_uint32	min_stat;

		/* acquire gss credentials */
		err = edg_wll_gss_acquire_cred_gsi(
		      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename,
		      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_key_filename,
		      &cred, &my_subject_name, &gss_stat);
		/* give up if unable to acquire prescribed credentials */
		if (err && ctx->p_proxy_filename) {
			edg_wll_SetErrorGss(ctx, "failed to load GSI credentials", &gss_stat);
			edg_wll_SetParamString(ctx, EDG_WLL_PARAM_LBPROXY_USER, EDG_WLL_LOG_USER_DEFAULT);
		} else {
			edg_wll_SetParamString(ctx, EDG_WLL_PARAM_LBPROXY_USER, my_subject_name);
		}

		if (cred != GSS_C_NO_CREDENTIAL)
			gss_release_cred(&min_stat, &cred);
		if (my_subject_name) free(my_subject_name);
	}

	/* query LBProxyServer for sequence code if not user-suplied */
/* XXX: don't know if working properly */
	if (!code) {
		if (edg_wll_QuerySequenceCodeProxy(ctx, job, &code_loc))
			goto edg_wll_setloggingjobproxy_end;	
	} else {
		code_loc = strdup(code);
	}
	
	if (!edg_wll_SetSequenceCode(ctx,code_loc,flags)) {
		edg_wll_IncSequenceCode(ctx);
	}
	
edg_wll_setloggingjobproxy_end:
	if (code_loc) free(code_loc);

        return edg_wll_Error(ctx,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Register job with L&B service.
 *-----------------------------------------------------------------------
 */
static int edg_wll_RegisterJobMaster(
        edg_wll_Context         ctx,
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
	char	*seq,*type_s,*parent_s;
	int	err = 0;
	struct timeval sync_to;

	seq = type_s = parent_s = NULL;

	edg_wll_ResetError(ctx);
	memcpy(&sync_to, &ctx->p_sync_timeout, sizeof sync_to);

	type_s = edg_wll_RegJobJobtypeToString(type);
	if (!type_s) {
		edg_wll_SetError(ctx,EINVAL,"edg_wll_RegisterJobMaster(): no jobtype specified");
		goto edg_wll_registerjobmaster_end;
	}
	if ((type == EDG_WLL_REGJOB_DAG || 
	     type == EDG_WLL_REGJOB_PARTITIONED ||
	     type == EDG_WLL_REGJOB_COLLECTION)
		&& num_subjobs > 0) {
		err = edg_wll_GenerateSubjobIds(ctx,job,num_subjobs,seed,subjobs);
		edg_wll_SetSequenceCode(ctx, NULL, EDG_WLL_SEQ_NORMAL);
		/* increase log timeout on client (the same as on BK server) */
		ctx->p_sync_timeout.tv_sec += num_subjobs;
		if (ctx->p_sync_timeout.tv_sec > 86400) ctx->p_sync_timeout.tv_sec = 86400;
	}
	if (err) {
		edg_wll_UpdateError(ctx,EINVAL,"edg_wll_RegisterJobMaster(): edg_wll_GenerateSubjobIds() error");
		goto edg_wll_registerjobmaster_end;
	}
	parent_s = parent ? edg_wlc_JobIdUnparse(parent) : strdup("");

	if (flags & LOGFLAG_DIRECT) {
		/* SetLoggingJob and log directly the message */
		if (edg_wll_SetLoggingJob(ctx,job,NULL,EDG_WLL_SEQ_NORMAL) == 0) {
			edg_wll_LogEventMaster(ctx,LOGFLAG_DIRECT | LOGFLAG_SYNC,
				EDG_WLL_EVENT_REGJOB,EDG_WLL_FORMAT_REGJOB,
				(char *)jdl,ns,parent_s,type_s,num_subjobs,seed);
		}
	} else if (flags & LOGFLAG_PROXY) {
		/* SetLoggingJobProxy and and log to proxy */
		edg_wll_SetSequenceCode(ctx, NULL, EDG_WLL_SEQ_NORMAL);
		if (seq) free(seq);
		seq = edg_wll_GetSequenceCode(ctx);
		if (edg_wll_SetLoggingJobProxy(ctx,job,seq,NULL,EDG_WLL_SEQ_NORMAL) == 0) {
			edg_wll_LogEventMaster(ctx,LOGFLAG_PROXY | LOGFLAG_SYNC,
				EDG_WLL_EVENT_REGJOB,EDG_WLL_FORMAT_REGJOB,
				(char *)jdl,ns,parent_s,type_s,num_subjobs,seed);
		}
	} else if (flags & LOGFLAG_NORMAL) {
		/* SetLoggingJob and log normally the message through the local-logger */
		if (edg_wll_SetLoggingJob(ctx,job,NULL,EDG_WLL_SEQ_NORMAL) == 0) {
			edg_wll_LogEventMaster(ctx, LOGFLAG_NORMAL,
				EDG_WLL_EVENT_REGJOB,EDG_WLL_FORMAT_REGJOB,
				(char *)jdl,ns,parent_s,type_s,num_subjobs,seed);
		}
	} else {
		edg_wll_SetError(ctx,EINVAL,"edg_wll_RegisterJobMaster(): wrong flag specified");
	}

edg_wll_registerjobmaster_end:
	memcpy(&ctx->p_sync_timeout, &sync_to, sizeof sync_to);
	if (seq) free(seq);
	if (type_s) free(type_s); 
	if (parent_s) free(parent_s);
	return edg_wll_Error(ctx,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Register synchronously one job with L&B service 
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobSync(
        edg_wll_Context         ctx,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	return edg_wll_RegisterJobMaster(ctx,LOGFLAG_DIRECT,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs);
}

/**
 *-----------------------------------------------------------------------
 * Register (asynchronously) one job with L&B service 
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJob(
        edg_wll_Context         ctx,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	return edg_wll_RegisterJobMaster(ctx,LOGFLAG_DIRECT,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs);
}

#ifndef LB_SERIAL_REG

/**
 *-----------------------------------------------------------------------
 * Register one job with L&B Proxy service 
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 * this is new (!LB_SERIAL_REG) edg_wll_RegisterJobProxy 
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobProxy(
        edg_wll_Context         ctx,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	char	*seq,*type_s;
	edg_wll_LogLine	logline = NULL;
	int	ret = 0,n,count,fd;
	struct timeval sync_to;
	edg_wll_GssConnection   con_bkserver;
	edg_wll_PlainConnection con_lbproxy;
	fd_set fdset;

	seq = type_s = NULL;

	edg_wll_ResetError(ctx);
	memcpy(&sync_to, &ctx->p_sync_timeout, sizeof sync_to);
	memset(&con_bkserver, 0, sizeof(con_bkserver));
	memset(&con_lbproxy, 0, sizeof(con_lbproxy));

	FD_ZERO(&fdset);

	type_s = edg_wll_RegJobJobtypeToString(type);
	if (!type_s) {
		edg_wll_SetError(ctx,EINVAL,"edg_wll_RegisterJobProxy(): no jobtype specified");
		goto edg_wll_registerjobproxy_end;
	}
	if ((type == EDG_WLL_REGJOB_DAG || 
	     type == EDG_WLL_REGJOB_PARTITIONED ||
	     type == EDG_WLL_REGJOB_COLLECTION)
		&& num_subjobs > 0) {
		edg_wll_SetSequenceCode(ctx, NULL, EDG_WLL_SEQ_NORMAL);
		ret = edg_wll_GenerateSubjobIds(ctx,job,num_subjobs,seed,subjobs);
		/* increase log timeout on client (the same as on BK server) */
		ctx->p_sync_timeout.tv_sec += num_subjobs;
		if (ctx->p_sync_timeout.tv_sec > 86400) ctx->p_sync_timeout.tv_sec = 86400;
	}
	if (ret) {
		edg_wll_UpdateError(ctx,EINVAL,"edg_wll_RegisterJobProxy(): edg_wll_GenerateSubjobIds() error");
		goto edg_wll_registerjobproxy_end;
	}

	/* SetLoggingJobProxy */
	edg_wll_SetSequenceCode(ctx, NULL, EDG_WLL_SEQ_NORMAL);
	seq = edg_wll_GetSequenceCode(ctx);
	if (edg_wll_SetLoggingJobProxy(ctx,job,seq,NULL,EDG_WLL_SEQ_NORMAL) != 0) {
		edg_wll_UpdateError(ctx,EINVAL,"edg_wll_RegisterJobProxy(): edg_wll_SetLoggingJobProxy() error");
		goto edg_wll_registerjobproxy_end; 
	}

	/* format the RegJob event message */
	if (edg_wll_FormatLogLine(ctx,LOGFLAG_SYNC | LOGFLAG_PROXY | LOGFLAG_PROXY,
		EDG_WLL_EVENT_REGJOB,&logline,
		EDG_WLL_FORMAT_REGJOB,(char *)jdl,ns,"",type_s,num_subjobs,seed) != 0 ) {
		edg_wll_UpdateError(ctx,EINVAL,"edg_wll_RegisterJobProxy(): edg_wll_FormatLogLine() error");
		goto edg_wll_registerjobproxy_end; 
	}       

	/* do not forget to set the timeout!!! */
	ctx->p_tmp_timeout = ctx->p_sync_timeout;

   /* and now do the pseudo-parallel registration: */

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_RegisterJobProxy: start (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
	/* connect to bkserver */
	if ((ret = edg_wll_log_direct_connect(ctx,&con_bkserver))) {
		edg_wll_UpdateError(ctx,EAGAIN,"edg_wll_RegisterJobProxy(): edg_wll_log_direct_connect error");
		goto edg_wll_registerjobproxy_end; 
	}
	/* connect to lbproxy */
	if ((ret = edg_wll_log_proxy_connect(ctx,&con_lbproxy))) {
		edg_wll_UpdateError(ctx,EAGAIN,"edg_wll_RegisterJobProxy(): edg_wll_log_proxy_connect error");
		goto edg_wll_registerjobproxy_end; 
	}
	/* send to bkserver */
	if ((ret = edg_wll_log_direct_write(ctx,&con_bkserver,logline)) == -1) {
		edg_wll_UpdateError(ctx,EAGAIN,"edg_wll_RegisterJobProxy(): edg_wll_log_direct_write error");
		goto edg_wll_registerjobproxy_end; 
	}
	/* send to lbproxy */
	if ((ret = edg_wll_log_proxy_write(ctx,&con_lbproxy,logline)) == -1) {
		edg_wll_UpdateError(ctx,EAGAIN,"edg_wll_RegisterJobProxy(): edg_wll_log_proxy_write error");
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
#ifdef EDG_WLL_LOG_STUB
		fprintf(stderr,"edg_wll_RegisterJobProxy: calling select (remaining timeout %d.%06d sec)\n",
			(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
		fd = select(n,&fdset,NULL,NULL,&ctx->p_tmp_timeout);
		switch (fd) {
		case 0: /* timeout */
			edg_wll_UpdateError(ctx,EAGAIN,"edg_wll_RegisterJobProxy(): select() timeouted");
			count = 0;
			break;
		case -1: /* error */
			switch(errno) {
			case EINTR:
				continue;
			default:
				edg_wll_UpdateError(ctx,errno,"edg_wll_RegisterJobProxy(): select() error"); 
			}
		default:
			break;
		}
		if (FD_ISSET(con_bkserver.sock,&fdset)) {
			/* read answer from bkserver */
			if ((ret = edg_wll_log_direct_read(ctx,&con_bkserver)) == -1) {
				edg_wll_UpdateError(ctx,EAGAIN,"edg_wll_RegisterJobProxy(): edg_wll_log_direct_read error");
				goto edg_wll_registerjobproxy_end; 
			}
			count -= 1;
		}	
		if (FD_ISSET(con_lbproxy.sock,&fdset)) {
			/* read answer from lbproxy */
			if ((ret = edg_wll_log_proxy_read(ctx,&con_lbproxy)) == -1) {
				edg_wll_UpdateError(ctx,EAGAIN,"edg_wll_RegisterJobProxy(): edg_wll_log_proxy_read error");
				goto edg_wll_registerjobproxy_end; 
			}
			count -= 1;
		}	
	}

edg_wll_registerjobproxy_end:
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_RegisterJobProxy: done (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
	if (con_bkserver.sock) edg_wll_gss_close(&con_bkserver,&ctx->p_tmp_timeout);
	if (con_lbproxy.sock) edg_wll_plain_close(&con_lbproxy);

	memcpy(&ctx->p_sync_timeout, &sync_to, sizeof sync_to);
	if (type_s) free(type_s);
	if (seq) free(seq);
	if (logline) free(logline);

	return edg_wll_Error(ctx,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Register one job with L&B Proxy service 
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 * this is original edg_wll_RegisterJobProxy 
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobProxyOld(
        edg_wll_Context         ctx,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	/* first register with bkserver */
	int ret = edg_wll_RegisterJobMaster(ctx,LOGFLAG_DIRECT,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs);
	if (ret) {
		edg_wll_UpdateError(ctx,0,"edg_wll_RegisterJobProxyOld(): unable to register with bkserver");
		return edg_wll_Error(ctx,NULL,NULL);
	}
	/* and then with L&B Proxy */
	return edg_wll_RegisterJobMaster(ctx,LOGFLAG_PROXY,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs);
}

#else /* LB_SERIAL_REG */

/**
 *-----------------------------------------------------------------------
 * Register one job with L&B Proxy service 
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 * this is original edg_wll_RegisterJobProxy 
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobProxy(
        edg_wll_Context         ctx,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	/* first register with bkserver */
	int ret = edg_wll_RegisterJobMaster(ctx,LOGFLAG_DIRECT,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs);
	if (ret) {
		edg_wll_UpdateError(ctx,0,"edg_wll_RegisterJobProxy(): unable to register with bkserver");
		return edg_wll_Error(ctx,NULL,NULL);
	}
	/* and then with L&B Proxy */
	return edg_wll_RegisterJobMaster(ctx,LOGFLAG_PROXY,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs);
}

#endif /* LB_SERIAL_REG */


#ifndef LB_SERIAL_REG

/**
 *-----------------------------------------------------------------------
 * Register one job with L&B Proxy service ONLY
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 * useful for performace measuring
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobProxyOnly(
        edg_wll_Context         ctx,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	return edg_wll_RegisterJobMaster(ctx,LOGFLAG_PROXY,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs);
}

#endif /* LB_SERIAL_REG */

/**
 *-----------------------------------------------------------------------
 * Register one subjob with L&B service
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 *-----------------------------------------------------------------------
 */
static
int edg_wll_RegisterSubjob(
        edg_wll_Context         ctx,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
	edg_wlc_JobId		parent,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
/* XXX: what is that ? */
#ifdef LB_PERF
	return edg_wll_RegisterJobMaster(ctx,LOGFLAG_DIRECT,job,type,jdl,ns,parent,num_subjobs,seed,subjobs);
#else
	return edg_wll_RegisterJobMaster(ctx,LOGFLAG_NORMAL,job,type,jdl,ns,parent,num_subjobs,seed,subjobs);
#endif
}

/**
 *-----------------------------------------------------------------------
 * Register one subjob with L&B Proxy service
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 *-----------------------------------------------------------------------
 */
static
int edg_wll_RegisterSubjobProxy(
        edg_wll_Context         ctx,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
	edg_wlc_JobId		parent,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	return edg_wll_RegisterJobMaster(ctx,LOGFLAG_PROXY,job,type,jdl,ns,parent,num_subjobs,seed,subjobs);
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
