/**
 * \file producer.c
 * \author Jan Pospisil
 */

#ident "$Header$"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <syslog.h>
#include <string.h>
#include <netdb.h>

#include "glite/wmsutils/jobid/strmd5.h"
#include "glite/lb/consumer.h"
#include "glite/lb/producer.h"
#include "glite/lb/context-int.h"
#include "glite/lb/ulm_parse.h"
#include "glite/lb/trio.h"
#include "glite/lb/lb_gss.h"
#include "glite/lb/escape.h"

#include "prod_proto.h"

static const char* socket_path="/tmp/lb_proxy_store.sock";

#ifdef FAKE_VERSION
int edg_wll_DoLogEvent(edg_wll_Context context, edg_wll_LogLine logline);
int edg_wll_DoLogEventProxy(edg_wll_Context context, edg_wll_LogLine logline);
#else
/**
 *----------------------------------------------------------------------
 * Connects to local-logger and sends already formatted ULM string
 * \brief helper logging function
 * \param context	INOUT context to work with,
 * \param priority	IN priority flag (0 for async, 1 for sync)
 * \param logline	IN formated ULM string
 *----------------------------------------------------------------------
 */
static int edg_wll_DoLogEvent(
	edg_wll_Context context,
/*	int priority,  */
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
		switch(answer) {
                case EDG_WLL_GSS_ERROR_EOF:
			edg_wll_SetError(context,ENOTCONN,"edg_wll_gss_connect()");
			break;
                case EDG_WLL_GSS_ERROR_TIMEOUT:
			edg_wll_SetError(context,ETIMEDOUT,"edg_wll_gss_connect()");
			break;
                case EDG_WLL_GSS_ERROR_ERRNO:
			edg_wll_SetError(context,errno,"edg_wll_gss_connect()");
			break;
                case EDG_WLL_GSS_ERROR_GSS: 
			edg_wll_SetErrorGss(context, "failed to authenticate to server",&gss_stat);
			break;
		case EDG_WLL_GSS_ERROR_HERRNO:
			{ 
				const char *msg1;
				char *msg2;
				msg1 = hstrerror(errno);
				asprintf(&msg2, "edg_wll_gss_connect(): %s", msg1);
				edg_wll_SetError(context,EDG_WLL_ERROR_DNS, msg2);
				free(msg2);
			}
                        break;
		default:
			edg_wll_SetError(context,ECONNREFUSED,"edg_wll_gss_connect(): unknown");
			break;
		}
		goto edg_wll_DoLogEvent_end;
	}

   /* and send the message to the local-logger: */

	answer = edg_wll_log_proto_client(context,&con,logline/*,priority*/);

	switch(answer) {
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
			edg_wll_UpdateError(context,EINVAL,"edg_wll_DoLogEvent(): Error code mapped to EINVAL");
			break;
		case EDG_WLL_IL_PROTO:
		case EDG_WLL_IL_SYS:
		case EDG_WLL_IL_EVENTS_WAITING:
			edg_wll_UpdateError(context,EAGAIN,"edg_wll_DoLogEvent(): Error code mapped to EAGAIN");
			break;

		default:
			edg_wll_UpdateError(context,EAGAIN,"edg_wll_DoLogEvent(): Error code mapped to EAGAIN");
		break;
	}

edg_wll_DoLogEvent_end:
	if (con.context != GSS_C_NO_CONTEXT)
		edg_wll_gss_close(&con,&context->p_tmp_timeout);
	if (cred != GSS_C_NO_CREDENTIAL)
		gss_release_cred(&min_stat, &cred);

	return edg_wll_Error(context, NULL, NULL);
}

/**
 *----------------------------------------------------------------------
 * Connects to L&B Proxy and sends already formatted ULM string
 * \brief helper logging function
 * \param context	INOUT context to work with,
 * \param logline	IN formated ULM string
 *----------------------------------------------------------------------
 */
static int edg_wll_DoLogEventProxy(
	edg_wll_Context context,
	edg_wll_LogLine logline)
{
	int	answer;
	struct sockaddr_un saddr;
	int 	flags;
	edg_wll_Connection conn;

	edg_wll_ResetError(context);
	answer = 0;

   /* open a connection to the L&B Proxy: */

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"Logging to L&B Proxy at socket %s\n",
		context->p_lbproxy_store_sock? context->p_lbproxy_store_sock: socket_path);
#endif
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


   /* and send the message to the L&B Proxy: */

	answer = edg_wll_log_proto_client_proxy(context,&conn,logline);
	
	close(conn.sock);

edg_wll_DoLogEventProxy_end:

	switch(answer) {
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
			edg_wll_UpdateError(context,EINVAL,"edg_wll_DoLogEventProxy(): Error code mapped to EINVAL");
			break;

		default:
			edg_wll_UpdateError(context,EAGAIN,"edg_wll_DoLogEventProxy(): Error code mapped to EAGAIN");
		break;
	}

	return edg_wll_Error(context, NULL, NULL);
}
#endif /* FAKE_VERSION */


/**
 *----------------------------------------------------------------------
 * Formats a logging message and sends it to local-logger
 * \brief master logging event function
 * \param context	INOUT context to work with,
 * \param priority	IN priority flag (0 for async, 1 for sync)
 * \param event		IN type of the event,
 * \param fmt		IN printf()-like format string,
 * \param ...		IN event specific values/data according to fmt.
 *----------------------------------------------------------------------
 */
static int edg_wll_LogEventMaster(
	edg_wll_Context context,
	int priority,
	edg_wll_EventCode event,
	char *fmt, ...)
{
	va_list	fmt_args;
	int	ret,answer;
	char	*fix,*var;
	char	*source,*eventName,*lvl, *fullid,*seq;
        struct timeval start_time;
	char	date[ULM_DATE_STRING_LENGTH+1];
	edg_wll_LogLine out;
	size_t  size;
	int	i;

	i = errno  =  size = 0;
	seq = fix = var = out = source = eventName = lvl = fullid = NULL;

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
	if (edg_wll_IncSequenceCode(context)) {
		ret = EINVAL;
		goto edg_wll_logeventmaster_end;
	}
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

   /* and send the message to the local-logger: */
	ret = edg_wll_DoLogEvent(context, /* priority,*/ out);

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

	if (ret) edg_wll_UpdateError(context,0,"Logging library ERROR: ");

	return edg_wll_Error(context,NULL,NULL);
}

/**
 *----------------------------------------------------------------------
 * Formats a logging message and sends it to L&B Proxy
 * \brief master proxy logging event function
 * \param context	INOUT context to work with,
 * \param event		IN type of the event,
 * \param fmt		IN printf()-like format string,
 * \param ...		IN event specific values/data according to fmt.
 *----------------------------------------------------------------------
 */
static int edg_wll_LogEventMasterProxy(
	edg_wll_Context context,
	edg_wll_EventCode event,
	char *fmt, ...)
{
	va_list	fmt_args;
	int	ret,answer;
	char	*fix,*var,*dguser;
	char	*source,*eventName,*lvl, *fullid,*seq,*name_esc;
        struct timeval start_time;
	char	date[ULM_DATE_STRING_LENGTH+1];
	edg_wll_LogLine out;
	size_t  size;
	int	i;

	i = errno  =  size = 0;
	seq = fix = var = dguser = out = source = eventName = lvl = fullid = NULL;

	edg_wll_ResetError(context);

   /* default return value is "Try Again" */
	answer = ret = EAGAIN; 

   /* format the message: */
	va_start(fmt_args,fmt);

	gettimeofday(&start_time,0);
	if (edg_wll_ULMTimevalToDate(start_time.tv_sec,start_time.tv_usec,date) != 0) {
		edg_wll_SetError(context,ret = EINVAL,"edg_wll_LogEventMasterProxy(): edg_wll_ULMTimevalToDate() error"); 
		goto edg_wll_logeventmasterproxy_end; 
	}
 	source = edg_wll_SourceToString(context->p_source);
	lvl = edg_wll_LevelToString(context->p_level);
	eventName = edg_wll_EventToString(event);
	if (!eventName) { 
		edg_wll_SetError(context,ret = EINVAL,"edg_wll_LogEventMasterProxy(): event name not specified"); 
		goto edg_wll_logeventmasterproxy_end; 
	}
	if (!(fullid = edg_wlc_JobIdUnparse(context->p_jobid))) { 
		edg_wll_SetError(context,ret = EINVAL,"edg_wll_LogEventMasterProxy(): edg_wlc_JobIdUnparse() error"); 
		goto edg_wll_logeventmasterproxy_end;
	}
	seq = edg_wll_GetSequenceCode(context);
	if (edg_wll_IncSequenceCode(context)) {
		ret = EINVAL;
		goto edg_wll_logeventmasterproxy_end;
	}
	if (trio_asprintf(&fix,EDG_WLL_FORMAT_COMMON,
			date,context->p_host,lvl,1,
			source,context->p_instance ? context->p_instance : "",
			eventName,fullid,seq) == -1) {
		edg_wll_SetError(context,ret = ENOMEM,"edg_wll_LogEventMasterProxy(): trio_asprintf() error"); 
		goto edg_wll_logeventmasterproxy_end; 
	}
	if (trio_vasprintf(&var,fmt,fmt_args) == -1) { 
		edg_wll_SetError(context,ret = ENOMEM,"edg_wll_LogEventMasterProxy(): trio_vasprintf() error"); 
		goto edg_wll_logeventmasterproxy_end; 
	}
        /* format the DG.USER string */
/* XXX: put user credentials here probably from context */
        name_esc = edg_wll_LogEscape(context->p_user_lbproxy);
        if (asprintf(&dguser,"DG.USER=\"%s\" ",name_esc) == -1) {
		edg_wll_SetError(context,ret = ENOMEM,"edg_wll_LogEventMasterProxy(): asprintf() error"); 
		goto edg_wll_logeventmasterproxy_end; 
        }
	if (asprintf(&out,"%s%s%s\n",dguser,fix,var) == -1) { 
		edg_wll_SetError(context,ret = ENOMEM,"edg_wll_LogEventMasterProxy(): asprintf() error"); 
		goto edg_wll_logeventmasterproxy_end; 
	}
	size = strlen(out);

	if (size > EDG_WLL_LOG_SYNC_MAXMSGSIZE) {
		edg_wll_SetError(context,ret = ENOSPC,"edg_wll_LogEventMasterProxy(): Message size too large for synchronous transfer");
		goto edg_wll_logeventmasterproxy_end;
	}

#ifdef EDG_WLL_LOG_STUB
//	fprintf(stderr,"edg_wll_LogEvent (%d chars): %s",size,out);
#endif
	
	context->p_tmp_timeout = context->p_sync_timeout;

   /* and send the message to the L&B Proxy: */
	ret = edg_wll_DoLogEventProxy(context, out);

edg_wll_logeventmasterproxy_end:
	va_end(fmt_args);
	if (seq) free(seq); 
	if (fix) free(fix); 
	if (var) free(var); 
	if (dguser) free(dguser); 
	if (out) free(out);
	if (source) free(source);
	if (lvl) free(lvl);
	if (eventName) free(eventName);
	if (fullid) free(fullid);
	if (name_esc) free(name_esc); 

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

	ret=edg_wll_LogEventMaster(context,0,event,"%s",list);

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

	ret=edg_wll_LogEventMaster(context,1,event,"%s",list);

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

        ret=edg_wll_LogEventMasterProxy(context,event,"%s",list);

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

	ret = edg_wll_DoLogEvent(context, /* 1,*/ out);

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

	ret = edg_wll_DoLogEvent(context, /* 1,*/ out);

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
	if ((err = edg_wlc_JobIdDup(job,&context->p_jobid)))
		edg_wll_SetError(context,err,"edg_wll_SetLoggingJob(): edg_wlc_JobIdDup() error");

	else {
		if (!edg_wll_SetSequenceCode(context,code,flags))
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
	char	*code_loc;

        edg_wll_ResetError(context);

/* XXX: add user credentials somewhere - to context? */
	edg_wll_SetParamString(context, EDG_WLL_PARAM_LBPROXY_USER, user);

        if (!job) return edg_wll_SetError(context,EINVAL,"jobid is null");

        edg_wlc_JobIdFree(context->p_jobid);
        if ((err = edg_wlc_JobIdDup(job,&context->p_jobid))) {
                edg_wll_SetError(context,err,"edg_wll_SetLoggingJob(): edg_wlc_JobIdDup() error");
		goto err;
	}

	/* query LBProxyServer for sequence code if not user-suplied *?
	if (!code) {
		edg_wll_QuerySequenceCode(context, job, &code_loc);
		goto err;	
	}
	else
		code_loc = code;
	
	if (!edg_wll_SetSequenceCode(context,code_loc,flags))
/* XXX: ask proxy for last known sequence code */
		edg_wll_IncSequenceCode(context);
	
err:
        return edg_wll_Error(context,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Register job with L&B service.
 *-----------------------------------------------------------------------
 */
static int edg_wll_RegisterJobMaster(
        edg_wll_Context         context,
	int			pri,
        const edg_wlc_JobId     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
	edg_wlc_JobId		parent,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	char	*type_s = NULL,*intseed = NULL, *seq = NULL;
	char	*parent_s = NULL;
	int	err = 0;

	edg_wll_ResetError(context);

	intseed = seed ? strdup(seed) : 
		str2md5base64(seq = edg_wll_GetSequenceCode(context));

	free(seq);

	type_s = edg_wll_RegJobJobtypeToString(type);
	if (!type_s) return edg_wll_SetError(context,EINVAL,"edg_wll_RegisterJobMaster(): no jobtype specified");

	if ((type == EDG_WLL_REGJOB_DAG || type == EDG_WLL_REGJOB_PARTITIONED)
		&& num_subjobs > 0) 
			err = edg_wll_GenerateSubjobIds(context,job,
					num_subjobs,intseed,subjobs);

	parent_s = parent ? edg_wlc_JobIdUnparse(parent) : strdup("");

	if (err == 0 &&
		edg_wll_SetLoggingJob(context,job,NULL,EDG_WLL_SEQ_NORMAL) == 0)
			edg_wll_LogEventMaster(context,pri,
				EDG_WLL_EVENT_REGJOB,EDG_WLL_FORMAT_REGJOB,
				(char *)jdl,ns,parent_s,type_s,num_subjobs,intseed);

	free(type_s); free(intseed); free(parent_s);
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
	return edg_wll_RegisterJobMaster(context,1,job,type,jdl,ns, NULL, num_subjobs,seed,subjobs);
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
	return edg_wll_RegisterJobMaster(context,0,job,type,jdl,ns, NULL, num_subjobs,seed,subjobs);
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
	return edg_wll_RegisterJobMaster(context,0,job,type,jdl,ns, parent, num_subjobs,seed,subjobs);
}

int edg_wll_RegisterSubjobs(edg_wll_Context ctx,const edg_wlc_JobId parent,
		char const * const * jdls, const char * ns, edg_wlc_JobId const * subjobs)
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
						ns, parent, 0, NULL, NULL) != 0) break;
		pjdl++; psubjob++;
	}

	edg_wll_SetLoggingJob(ctx, oldctxjob, oldctxseq, EDG_WLL_SEQ_NORMAL);

	return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_ChangeACL(
		edg_wll_Context				ctx,
		const edg_wlc_JobId			jobid,
		const char				   *user_id,
		enum edg_wll_UserIdType		user_id_type,
		enum edg_wll_Permission		permission,
		enum edg_wll_PermissionType	permission_type,
		enum edg_wll_ACLOperation	operation)
{
	if ( edg_wll_SetLoggingJob(ctx, jobid, NULL, EDG_WLL_SEQ_NORMAL) == 0 )
		edg_wll_LogEventMaster(ctx, 1, EDG_WLL_EVENT_CHANGEACL, EDG_WLL_FORMAT_CHANGEACL,
				user_id, user_id_type, permission, permission_type, operation);


	return edg_wll_Error(ctx,NULL,NULL);
}
