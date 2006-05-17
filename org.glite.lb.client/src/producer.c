/**
 * \file producer.c
 * \author Jan Pospisil
 */

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
#include <netdb.h>

#include "glite/wmsutils/jobid/strmd5.h"
#include "glite/security/glite_gss.h"
#include "glite/lb/consumer.h"
#include "glite/lb/producer.h"
#include "glite/lb/context-int.h"
#include "glite/lb/ulm_parse.h"
#include "glite/lb/trio.h"
#include "glite/lb/lb_plain_io.h"
#include "glite/lb/escape.h"

#include "prod_proto.h"

static const char* socket_path="/tmp/lb_proxy_store.sock";

#ifdef FAKE_VERSION
int edg_wll_DoLogEvent(edg_wll_Context context, edg_wll_LogLine logline);
int edg_wll_DoLogEventProxy(edg_wll_Context context, edg_wll_LogLine logline);
int edg_wll_DoLogEventDirect(edg_wll_Context context, edg_wll_LogLine logline);
#else

/*
 *----------------------------------------------------------------------
 * handle_answers - handle answers from edg_wll_log_proto_client*
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
 * \param context	INOUT context to work with,
 * \param logline	IN formated ULM string
 *----------------------------------------------------------------------
 */
int edg_wll_DoLogEvent(
	edg_wll_Context context,
	edg_wll_LogLine logline)
{
	int	ret,answer;
	char	*my_subject_name = NULL;
	edg_wll_GssStatus	gss_stat;
	edg_wll_GssConnection	con;
	gss_cred_id_t	cred = GSS_C_NO_CREDENTIAL;
	OM_uint32	min_stat;

	edg_wll_ResetError(context);
	ret = answer = 0;
	memset(&con, 0, sizeof(con));

   /* open an authenticated connection to the local-logger: */
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"Logging to host %s, port %d\n",
			context->p_destination, context->p_dest_port);
#endif
	ret = edg_wll_gss_acquire_cred_gsi(
	      context->p_proxy_filename ? context->p_proxy_filename : context->p_cert_filename,
	      context->p_proxy_filename ? context->p_proxy_filename : context->p_key_filename,
	      &cred, &my_subject_name, &gss_stat);
	/* Give up if unable to prescribed credentials, otherwise go on anonymously */
	if (ret && context->p_proxy_filename) {
		edg_wll_SetErrorGss(context, "failed to load GSI credentials", &gss_stat);
		goto edg_wll_DoLogEvent_end;
	}
	      				   
	if (my_subject_name != NULL) {
#ifdef EDG_WLL_LOG_STUB
		fprintf(stderr,"Using certificate: %s\n",my_subject_name);
#endif
		free(my_subject_name);
	}
	if ((answer = edg_wll_gss_connect(cred,
			context->p_destination, context->p_dest_port, 
			&context->p_tmp_timeout, &con, &gss_stat)) < 0) {
		answer = edg_wll_log_proto_handle_gss_failures(context,answer,&gss_stat,"edg_wll_gss_connect()");
		goto edg_wll_DoLogEvent_end;
	}

   /* and send the message to the local-logger: */
	answer = edg_wll_log_proto_client(context,&con,logline);

edg_wll_DoLogEvent_end:
	if (con.context != GSS_C_NO_CONTEXT)
		edg_wll_gss_close(&con,&context->p_tmp_timeout);
	if (cred != GSS_C_NO_CREDENTIAL)
		gss_release_cred(&min_stat, &cred);

	return handle_answers(context,answer,"edg_wll_DoLogEvent()");
}

/**
 *----------------------------------------------------------------------
 * Connects to L&B Proxy and sends already formatted ULM string
 * \brief helper logging function
 * \param context	INOUT context to work with,
 * \param logline	IN formated ULM string
 *----------------------------------------------------------------------
 */
int edg_wll_DoLogEventProxy(
	edg_wll_Context context,
	edg_wll_LogLine logline)
{
	int	answer;
	int 	flags;
	char	*name_esc,*dguser;
	struct sockaddr_un saddr;
	edg_wll_PlainConnection conn;
	edg_wll_LogLine out;

	answer = 0;
	name_esc = dguser = out = NULL;

	edg_wll_ResetError(context);

   /* open a connection to the L&B Proxy: */
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"Logging to L&B Proxy at socket %s\n",
		context->p_lbproxy_store_sock? context->p_lbproxy_store_sock: socket_path);
#endif
	memset(&conn, 0, sizeof(conn));
	conn.sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (conn.sock < 0) {
		edg_wll_SetError(context,answer = errno,"socket() error");
		goto edg_wll_DoLogEventProxy_end;
	}
	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	strcpy(saddr.sun_path, context->p_lbproxy_store_sock?
				context->p_lbproxy_store_sock: socket_path);
	if ((flags = fcntl(conn.sock, F_GETFL, 0)) < 0 || fcntl(conn.sock, F_SETFL, flags | O_NONBLOCK) < 0) {
		edg_wll_SetError(context,answer = errno,"fcntl()");
		close(conn.sock);
		goto edg_wll_DoLogEventProxy_end;
	}
	if (connect(conn.sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		if(errno != EISCONN) {
			edg_wll_SetError(context,answer = errno,"connect()");
			close(conn.sock);
			goto edg_wll_DoLogEventProxy_end;
		}
	}

   /* add DG.USER to the message: */
        name_esc = edg_wll_LogEscape(context->p_user_lbproxy);
        if (asprintf(&dguser,"DG.USER=\"%s\" ",name_esc) == -1) {
		edg_wll_SetError(context,answer = ENOMEM,"edg_wll_LogEventMasterProxy(): asprintf() error"); 
		goto edg_wll_DoLogEventProxy_end; 
        }
	if (asprintf(&out,"%s%s",dguser,logline) == -1) { 
		edg_wll_SetError(context,answer = ENOMEM,"edg_wll_LogEventMasterProxy(): asprintf() error"); 
		goto edg_wll_DoLogEventProxy_end; 
	}

   /* and send the message to the L&B Proxy: */
	answer = edg_wll_log_proto_client_proxy(context,&conn,out);
	
edg_wll_DoLogEventProxy_end:
	edg_wll_plain_close(&conn);

	if (name_esc) free(name_esc);
	if (dguser) free(dguser);
	if (out) free(out);

	return handle_answers(context,answer,"edg_wll_DoLogEventProxy()");
}

/**
 *----------------------------------------------------------------------
 * Connects to bkserver and sends already formatted ULM string
 * \brief helper logging function
 * \param context	INOUT context to work with,
 * \param logline	IN formated ULM string
 *----------------------------------------------------------------------
 */
int edg_wll_DoLogEventDirect(
	edg_wll_Context context,
	edg_wll_LogLine logline)
{
	int	ret,answer;
	char	*my_subject_name,*name_esc,*dguser;
	char	*host;
	int	port;
	edg_wll_LogLine out;
	edg_wll_GssStatus	gss_stat;
	edg_wll_GssConnection	con;
	gss_cred_id_t	cred = GSS_C_NO_CREDENTIAL;
	OM_uint32	min_stat;

	ret = answer = 0;
	my_subject_name = name_esc = dguser = out = NULL;
	memset(&con, 0, sizeof(con));

	edg_wll_ResetError(context);

   /* get bkserver location: */
	edg_wlc_JobIdGetServerParts(context->p_jobid,&host,&port);
	port +=1;

   /* open an authenticated connection to the bkserver: */
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"Logging to bkserver host %s, port %d\n", host, port);
#endif
	ret = edg_wll_gss_acquire_cred_gsi(
	      context->p_proxy_filename ? context->p_proxy_filename : context->p_cert_filename,
	      context->p_proxy_filename ? context->p_proxy_filename : context->p_key_filename,
	      &cred, &my_subject_name, &gss_stat);
	/* Give up if unable to prescribed credentials, otherwise go on anonymously */
	if (ret && context->p_proxy_filename) {
		edg_wll_SetErrorGss(context, "failed to load GSI credentials", &gss_stat);
		goto edg_wll_DoLogEventDirect_end;
	}
	      				   
#ifdef EDG_WLL_LOG_STUB
	if (my_subject_name) {
		fprintf(stderr,"Using certificate: %s\n",my_subject_name);
	} else {
		fprintf(stderr,"Going on anonymously\n");
	}
#endif
	if ((answer = edg_wll_gss_connect(cred,host,port,
			&context->p_tmp_timeout, &con, &gss_stat)) < 0) {
		answer = edg_wll_log_proto_handle_gss_failures(context,answer,&gss_stat,"edg_wll_gss_connect()");
		goto edg_wll_DoLogEventDirect_end;
	}

   /* add DG.USER to the message: */
        name_esc = edg_wll_LogEscape(my_subject_name);
        if (asprintf(&dguser,"DG.USER=\"%s\" ",name_esc) == -1) {
		edg_wll_SetError(context,answer = ENOMEM,"edg_wll_DoLogEventDirect(): asprintf() error"); 
		goto edg_wll_DoLogEventDirect_end; 
        }
	if (asprintf(&out,"%s%s\n",dguser,logline) == -1) { 
		edg_wll_SetError(context,answer = ENOMEM,"edg_wll_DoLogEventDirect(): asprintf() error"); 
		goto edg_wll_DoLogEventDirect_end; 
	}

   /* and send the message to the bkserver: */
	answer = edg_wll_log_proto_client_direct(context,&con,out);

edg_wll_DoLogEventDirect_end:
	if (con.context != GSS_C_NO_CONTEXT)
		edg_wll_gss_close(&con,&context->p_tmp_timeout);
	if (cred != GSS_C_NO_CREDENTIAL)
		gss_release_cred(&min_stat, &cred);
	if (host) free(host);
	if (name_esc) free(name_esc);
	if (dguser) free(dguser);
	if (out) free(out);
	if(my_subject_name) free(my_subject_name);

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
 * Formats a logging message and sends it to local-logger
 * \brief master logging event function
 * \param context	INOUT context to work with,
 * \param flags		IN as defined by LOGFLAG_*
 * \param event		IN type of the event,
 * \param fmt		IN printf()-like format string,
 * \param ...		IN event specific values/data according to fmt.
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
	int	ret,answer;
	char	*fix,*var;
	char	*source,*eventName,*lvl, *fullid,*seq;
        struct timeval start_time;
	char	date[ULM_DATE_STRING_LENGTH+1];
	edg_wll_LogLine out;
	size_t  size;
	int	i;

	i = errno = size = 0;
	seq = fix = var = out = source = eventName = lvl = fullid = NULL;
	priority = flags & LOGFLAG_SYNC;

	edg_wll_ResetError(context);

   /* default return value is "Try Again" */
	answer = ret = EAGAIN; 

   /* format the message: */
	va_start(fmt_args,fmt);

	gettimeofday(&start_time,0);
	if (edg_wll_ULMTimevalToDate(start_time.tv_sec,start_time.tv_usec,date) != 0) {
		edg_wll_SetError(context,ret = EINVAL,"edg_wll_LogEventMaster(): edg_wll_ULMTimevalToDate() error"); 
		goto edg_wll_logeventmaster_end; 
	}
 	source = edg_wll_SourceToString(context->p_source);
	lvl = edg_wll_LevelToString(context->p_level);
	eventName = edg_wll_EventToString(event);
	if (!eventName) { 
		edg_wll_SetError(context,ret = EINVAL,"edg_wll_LogEventMaster(): event name not specified"); 
		goto edg_wll_logeventmaster_end; 
	}
	if (!(fullid = edg_wlc_JobIdUnparse(context->p_jobid))) { 
		edg_wll_SetError(context,ret = EINVAL,"edg_wll_LogEventMaster(): edg_wlc_JobIdUnparse() error"); 
		goto edg_wll_logeventmaster_end;
	}
	seq = edg_wll_GetSequenceCode(context);

	if (trio_asprintf(&fix,EDG_WLL_FORMAT_COMMON,
			date,context->p_host,lvl,priority,
			source,context->p_instance ? context->p_instance : "",
			eventName,fullid,seq) == -1) {
		edg_wll_SetError(context,ret = ENOMEM,"edg_wll_LogEventMaster(): trio_asprintf() error"); 
		goto edg_wll_logeventmaster_end; 
	}
	if (trio_vasprintf(&var,fmt,fmt_args) == -1) { 
		edg_wll_SetError(context,ret = ENOMEM,"edg_wll_LogEventMaster(): trio_vasprintf() error"); 
		goto edg_wll_logeventmaster_end; 
	}
	if (asprintf(&out,"%s%s\n",fix,var) == -1) { 
		edg_wll_SetError(context,ret = ENOMEM,"edg_wll_LogEventMaster(): asprintf() error"); 
		goto edg_wll_logeventmaster_end; 
	}
	size = strlen(out);

	if (priority && (size > EDG_WLL_LOG_SYNC_MAXMSGSIZE)) {
		edg_wll_SetError(context,ret = ENOSPC,"edg_wll_LogEventMaster(): Message size too large for synchronous transfer");
		goto edg_wll_logeventmaster_end;
	}

#ifdef EDG_WLL_LOG_STUB
//	fprintf(stderr,"edg_wll_LogEvent (%d chars): %s",size,out);
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

edg_wll_logeventmaster_end:
	va_end(fmt_args);
	if (seq) free(seq); 
	if (fix) free(fix); 
	if (var) free(var); 
	if (out) free(out);
	if (source) free(source);
	if (lvl) free(lvl);
	if (eventName) free(eventName);
	if (fullid) free(fullid);

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

        edg_wll_ResetError(context);

        if (!job) return edg_wll_SetError(context,EINVAL,"jobid is null");

        edg_wlc_JobIdFree(context->p_jobid);
        if ((err = edg_wlc_JobIdDup(job,&context->p_jobid))) {
                edg_wll_SetError(context,err,"edg_wll_SetLoggingJob(): edg_wlc_JobIdDup() error");
		goto edg_wll_setloggingjobproxy_end;
	}

	/* add user credentials to context */
	edg_wll_SetParamString(context, EDG_WLL_PARAM_LBPROXY_USER, user);

	/* query LBProxyServer for sequence code if not user-suplied */
/* FIXME: doesn't work yet */
	if (!code) {
		edg_wll_QuerySequenceCodeProxy(context, job, &code_loc);
		goto edg_wll_setloggingjobproxy_end;	
	} else {
		code_loc = strdup(code);
	}
	
	if (!edg_wll_SetSequenceCode(context,code_loc,flags)) {
		edg_wll_IncSequenceCode(context);
	}
	
edg_wll_setloggingjobproxy_end:
	if (code_loc) free(code_loc);

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
	char	*seq,*type_s,*intseed,*parent_s,*user_dn;
	int	err = 0;
        edg_wll_GssStatus       gss_stat;
        gss_cred_id_t   cred = GSS_C_NO_CREDENTIAL;
	OM_uint32       min_stat;

	seq = type_s = intseed = parent_s = user_dn = NULL;

	edg_wll_ResetError(context);

	intseed = seed ? strdup(seed) : 
		str2md5base64(seq = edg_wll_GetSequenceCode(context));

	type_s = edg_wll_RegJobJobtypeToString(type);
	if (!type_s) {
		edg_wll_SetError(context,EINVAL,"edg_wll_RegisterJobMaster(): no jobtype specified");
		goto edg_wll_registerjobmaster_end;
	}
	if ((type == EDG_WLL_REGJOB_DAG || type == EDG_WLL_REGJOB_PARTITIONED)
		&& num_subjobs > 0) {
		err = edg_wll_GenerateSubjobIds(context,job,num_subjobs,intseed,subjobs);
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
		/* first obtain the certiicate */
		err = edg_wll_gss_acquire_cred_gsi(
		      context->p_proxy_filename ? context->p_proxy_filename : context->p_cert_filename,
		      context->p_proxy_filename ? context->p_proxy_filename : context->p_key_filename,
		      &cred, &user_dn, &gss_stat);
		/* Give up if unable to obtain credentials */
		if (err && context->p_proxy_filename) {
			edg_wll_SetErrorGss(context, "failed to load GSI credentials", &gss_stat);
			goto edg_wll_registerjobmaster_end;
		}
		/* SetLoggingJobProxy and and log to proxy */
		edg_wll_SetSequenceCode(context, NULL, EDG_WLL_SEQ_NORMAL);
		if (seq) free(seq);
		seq = edg_wll_GetSequenceCode(context);
		if (edg_wll_SetLoggingJobProxy(context,job,seq,user_dn,EDG_WLL_SEQ_NORMAL) == 0) {
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
        if (cred != GSS_C_NO_CREDENTIAL)
                gss_release_cred(&min_stat, &cred);
	if (seq) free(seq);
	if (type_s) free(type_s); 
	if (intseed) free(intseed); 
	if (parent_s) free(parent_s);
	return edg_wll_Error(context,NULL,NULL);
}

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
	int ret = edg_wll_RegisterJob(context,job,type,jdl,ns,num_subjobs,seed ? seed : MY_SEED,subjobs);
	if (ret) {
		edg_wll_UpdateError(context,0,"edg_wll_RegisterJobProxy(): unable to register with bkserver");
		return edg_wll_Error(context,NULL,NULL);
	}
	/* and then with L&B Proxy */
	return edg_wll_RegisterJobMaster(context,LOGFLAG_PROXY,job,type,jdl,ns,NULL,num_subjobs,seed ? seed : MY_SEED,subjobs);
}

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

	if (edg_wll_GetLoggingJob(ctx, &oldctxjob)) return edg_wll_Error(ctx, NULL, NULL);
	oldctxseq = edg_wll_GetSequenceCode(ctx);

	pjdl = jdls;
	psubjob = subjobs;
	
	while (*pjdl != NULL) {
		if (edg_wll_RegisterSubjob(ctx, *psubjob, EDG_WLL_REGJOB_SIMPLE, *pjdl,
						ns, parent, 0, NULL, NULL) != 0) {
			goto edg_wll_registersubjobs_end; 
		}
		pjdl++; psubjob++;
	}

	edg_wll_SetLoggingJob(ctx, oldctxjob, oldctxseq, EDG_WLL_SEQ_NORMAL);

edg_wll_registersubjobs_end:
	return edg_wll_Error(ctx, NULL, NULL);
}

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

	if (edg_wll_GetLoggingJob(ctx, &oldctxjob)) return edg_wll_Error(ctx, NULL, NULL);
	oldctxseq = edg_wll_GetSequenceCode(ctx);

	pjdl = jdls;
	psubjob = subjobs;
	
	while (*pjdl != NULL) {
		if (edg_wll_RegisterSubjobProxy(ctx, *psubjob, EDG_WLL_REGJOB_SIMPLE, *pjdl,
						ns, parent, 0, NULL, NULL) != 0) {
			goto edg_wll_registersubjobsproxy_end;
		}
		pjdl++; psubjob++;
	}

edg_wll_registersubjobsproxy_end:
	edg_wll_SetLoggingJobProxy(ctx, oldctxjob, oldctxseq, NULL, EDG_WLL_SEQ_NORMAL);

	return edg_wll_Error(ctx, NULL, NULL);
}

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
