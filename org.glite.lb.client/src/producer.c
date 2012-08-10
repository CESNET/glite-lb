#ident "$Header$"
/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <syslog.h>
#include <string.h>

#include "glite/jobid/cjobid.h"
#include "glite/jobid/strmd5.h"
#include "glite/lbu/trio.h"
#include "glite/lb/ulm_parse.h"
#include "glite/lb/context-int.h" 

#include "producer.h"
#include "prod_proto.h"
#include "consumer.h" // for QuerySequenceCode

#ifdef FAKE_VERSION
int edg_wll_DoLogEvent(edg_wll_Context ctx, edg_wll_LogLine logline);
int edg_wll_DoLogEventServer(edg_wll_Context ctx, int flags, edg_wll_LogLine logline);
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
		case EPERM:
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
		if (ret == -1 && conn >= 0) { edg_wll_log_close(ctx,conn); }
		else if(conn >= 0) { edg_wll_connectionUnlock(ctx, conn); }

	} while (++attempt <= 2 && (answer == ENOTCONN || answer == EPIPE));

	return handle_errors(ctx,answer,"edg_wll_DoLogEvent()");
}

/**
 *----------------------------------------------------------------------
 * Open a pseudo parallel L&B Proxy (UNIX socket) and L&B server (GSS),
 * send already formatted ULM string and get answer back 
 * \brief connect to lbproxy, send message and get answer back
 * \param[in,out] ctx	context to work with,
 * \param[in] flags	as defined by EDG_WLL_LOGFLAG_*
 * \param[in] logline	formated ULM string
 *----------------------------------------------------------------------
 */
int edg_wll_DoLogEventServer(
        edg_wll_Context ctx,
	int flags,
        edg_wll_LogLine logline)
{
        edg_wll_PlainConnection con_lbproxy;
        edg_wll_GssConnection   con_bkserver;
        fd_set fdset;
	int count,fd,fd_n,proxy_answer=0,direct_answer=0;
	int answer = EAGAIN, ret = 0;

        edg_wll_ResetError(ctx);
        memset(&con_lbproxy, 0, sizeof(con_lbproxy));
        memset(&con_bkserver, 0, sizeof(con_bkserver));

	/* CONNECT */
	count=0;
	if (flags & EDG_WLL_LOGFLAG_PROXY) {
		/* connect to lbproxy */
		if ((ret = edg_wll_log_proxy_connect(ctx,&con_lbproxy))) {
			edg_wll_UpdateError(ctx,EAGAIN,"edg_wll_DoLogEventServer(): edg_wll_log_proxy_connect error");
			goto edg_wll_DoLogEventServer_end;
		}
		count++;
	}
	if (flags & EDG_WLL_LOGFLAG_DIRECT) {
		/* connect to bkserver */
		if ((ret = edg_wll_log_direct_connect(ctx,&con_bkserver))) {
			edg_wll_UpdateError(ctx,EAGAIN,"edg_wll_DoLogEventServer(): edg_wll_log_direct_connect error");
			goto edg_wll_DoLogEventServer_end;
		}
		count++;
	}

	/* SEND MESSAGE */
	if (flags & EDG_WLL_LOGFLAG_PROXY) {
		/* send to lbproxy */
		if ((ret = edg_wll_log_proxy_write(ctx,&con_lbproxy,logline)) == -1) {
                        answer = edg_wll_Error(ctx, NULL, NULL);
			edg_wll_UpdateError(ctx,EDG_WLL_IL_PROTO,"edg_wll_DoLogEventServer(): edg_wll_log_proxy_write error");
			goto edg_wll_DoLogEventServer_end;
		}
	}
	if (flags & EDG_WLL_LOGFLAG_DIRECT) {
		/* send to bkserver */
		if ((ret = edg_wll_log_direct_write(ctx,&con_bkserver,logline)) == -1) {
                        answer = edg_wll_Error(ctx, NULL, NULL);
			edg_wll_UpdateError(ctx,EAGAIN,"edg_wll_DoLogEventServer(): edg_wll_log_direct_write error");
			goto edg_wll_DoLogEventServer_end;
		}
	}

	/* READ ANSWER */
	while (count > 0) {

		FD_ZERO(&fdset);

		fd_n=0;
		if ((flags & EDG_WLL_LOGFLAG_DIRECT)&&(!direct_answer)) {
			FD_SET(con_bkserver.sock,&fdset);
			if (con_bkserver.sock > fd_n) fd_n = con_bkserver.sock;
		}
		if ((flags & EDG_WLL_LOGFLAG_PROXY)&&(!proxy_answer)) {
			FD_SET(con_lbproxy.sock,&fdset);
			if (con_lbproxy.sock > fd_n) fd_n = con_lbproxy.sock;
		}
		fd_n += 1;

#ifdef EDG_WLL_LOG_STUB
		fprintf(stderr,"edg_wll_DoLogEventServer(): calling select (remaining timeout %d.%06d sec)\n",
			(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
		fd = select(fd_n,&fdset,NULL,NULL,&ctx->p_tmp_timeout);
		switch (fd) {
		case 0: /* timeout */
			edg_wll_UpdateError(ctx,EAGAIN,"edg_wll_DoLogEventServer(): select() timeouted");
			count = 0;
			goto edg_wll_DoLogEventServer_end; 
			break;
		case -1: /* error */
			switch(errno) {
			case EINTR:
				continue;
			default:
				edg_wll_UpdateError(ctx,errno,"edg_wll_DoLogEventServer(): select() error"); 
				goto edg_wll_DoLogEventServer_end; 
			}
		default:
			break;
		}
		/* XXX: read only from an apropriate descriptor 
			FD_ISSET can't be true unless mathching bit in flags was set
		*/
		if (FD_ISSET(con_lbproxy.sock,&fdset)) {
			/* read answer from lbproxy */
			if ((ret = edg_wll_log_proxy_read(ctx,&con_lbproxy)) == -1) {
				edg_wll_UpdateError(ctx,EAGAIN,"edg_wll_DoLogEventServer(): edg_wll_log_proxy_read error");
				goto edg_wll_DoLogEventServer_end; 
			}
			count -= 1;
			proxy_answer = 1;
		}	
		if (FD_ISSET(con_bkserver.sock,&fdset)) {
			/* read answer from bkserver */
			if ((ret = edg_wll_log_direct_read(ctx,&con_bkserver)) == -1) {
				edg_wll_UpdateError(ctx,EAGAIN,"edg_wll_DoLogEventServer(): edg_wll_log_direct_read error");
				goto edg_wll_DoLogEventServer_end; 
			}
			count -= 1;
			direct_answer = 1;
		}	
	}

edg_wll_DoLogEventServer_end:
	edg_wll_log_proxy_close(ctx,&con_lbproxy);
	edg_wll_log_direct_close(ctx,&con_bkserver);

	return handle_errors(ctx,answer,"edg_wll_DoLogEventServer()");

}

#endif /* FAKE_VERSION */

/**
 *----------------------------------------------------------------------
 * Formats a logging message 
 * \brief formats a logging message
 * \param[in,out] ctx	context to work with,
 * \param[in] flags		as defined by EDG_WLL_LOGFLAG_*
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
	priority = flags;

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
/* TODO: merge - add always, probably new ctx->p_user 
	has this already been agreed? */
	if ( ( (flags & EDG_WLL_LOGFLAG_PROXY) || (flags & EDG_WLL_LOGFLAG_DIRECT) ) && 
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

	if ((flags & (EDG_WLL_LOGFLAG_DIRECT|EDG_WLL_LOGFLAG_SYNC)) && (size > EDG_WLL_LOG_SYNC_MAXMSGSIZE)) {
		edg_wll_SetError(ctx,ret = ENOSPC,"edg_wll_FormatLogLine(): Message size too large for synchronous transfer");
		goto edg_wll_formatlogline_end;
	}

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_FormatLogLine (%d chars): %s",size,out);
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
 * Formats a logging message and sends it to a correct destination
 * \brief master logging event function
 * \param[in,out] ctx		context to work with,
 * \param[in] flags		logging flags indicating the destination
 * 	EDG_WLL_LOGFLAG_LOCAL	- local-logger
 * 	EDG_WLL_LOGFLAG_PROXY	- lbproxy
 * 	EDG_WLL_LOGFLAG_DIRECT	- bkserver
 * \param[in] event		type of the event,
 * \param[in] fmt		printf()-like format string,
 * \param[in] fmt_args		event specific values/data according to fmt.
 *----------------------------------------------------------------------
 */
static int edg_wll_LogEventMasterVa(
	edg_wll_Context ctx,
	int flags,
	edg_wll_EventCode event,
	char *fmt, va_list fmt_args)
{
//	va_list	fmt_args;
	int     ret = 0;
	edg_wll_LogLine in = NULL, out = NULL;
        int     err_store;
        char    *err_desc_store = NULL;

	if ((flags & (EDG_WLL_LOGFLAG_LOCAL|EDG_WLL_LOGFLAG_PROXY|EDG_WLL_LOGFLAG_DIRECT)) == 0) {
		return edg_wll_SetError(ctx,ret = EINVAL,"edg_wll_LogEventMaster(): no known flag specified");
	}

	/* format the message */
	//va_start(fmt_args,fmt);

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
	
#ifndef LB_PERF_DROP
	ctx->p_tmp_timeout = (flags & (EDG_WLL_LOGFLAG_DIRECT | EDG_WLL_LOGFLAG_SYNC)) 
		? ctx->p_sync_timeout : ctx->p_log_timeout;

	/* send the message and read answer back */
        if (flags & EDG_WLL_LOGFLAG_LOCAL) {
                ret = edg_wll_DoLogEvent(ctx, out);
                if (ret) goto edg_wll_logeventmaster_end;
        }
        if (flags & (EDG_WLL_LOGFLAG_PROXY | EDG_WLL_LOGFLAG_DIRECT)) {
                ret = edg_wll_DoLogEventServer(ctx, flags, out);
                if (ret) goto edg_wll_logeventmaster_end;
        }
#endif

edg_wll_logeventmaster_end:
//	va_end(fmt_args);
	if (in) free(in);
	if (out) free(out);

        if (ctx->errCode) { 
                err_store = ctx->errCode;
                err_desc_store = strdup(ctx->errDesc); }

	if(edg_wll_IncSequenceCode(ctx)) {
		edg_wll_SetError(ctx,ret = EINVAL,"edg_wll_LogEventMaster(): edg_wll_IncSequenceCode failed");
	}

	if (err_desc_store) {
		edg_wll_SetError(ctx, err_store, err_desc_store);
		free(err_desc_store); }

	if (ret) edg_wll_UpdateError(ctx,0,"Logging library ERROR: ");

	return edg_wll_Error(ctx,NULL,NULL);
}

/**
 *----------------------------------------------------------------------
 * Formats a logging message and sends it to a correct destination
 * \brief master logging event function
 * \note simple wrapper around edg_wll_LogEventMasterVa() 
 * \brief master logging event function
 * \param[in,out] ctx		context to work with,
 * \param[in] flags		logging flags indicating the destination
 * 	EDG_WLL_LOGFLAG_LOCAL	- local-logger
 * 	EDG_WLL_LOGFLAG_PROXY	- lbproxy
 * 	EDG_WLL_LOGFLAG_DIRECT	- bkserver
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
	int	ret;
	va_list	fmt_args;
	va_start(fmt_args,fmt);
	ret = edg_wll_LogEventMasterVa(ctx,flags,event,fmt,fmt_args);
	va_end(fmt_args);
	return ret;
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
	va_list	fmt_args;

	edg_wll_ResetError(ctx);

	va_start(fmt_args,fmt);
	ret=edg_wll_LogEventMasterVa(ctx,EDG_WLL_LOGFLAG_LOCAL,
		event,fmt,fmt_args);
	if (ret) edg_wll_UpdateError(ctx,0,"edg_wll_LogEvent(): ");
	va_end(fmt_args);

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
	va_list	fmt_args;

	edg_wll_ResetError(ctx);

	va_start(fmt_args,fmt);
	ret=edg_wll_LogEventMasterVa(ctx,EDG_WLL_LOGFLAG_LOCAL | EDG_WLL_LOGFLAG_SYNC,
		event,fmt,fmt_args);
	if (ret) edg_wll_UpdateError(ctx,0,"edg_wll_LogEventSync(): ");
	va_end(fmt_args);

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
        va_list fmt_args;

        edg_wll_ResetError(ctx);

        va_start(fmt_args,fmt);
        ret=edg_wll_LogEventMasterVa(ctx,EDG_WLL_LOGFLAG_PROXY,
		event,fmt,fmt_args);
        if (ret) edg_wll_UpdateError(ctx,0,"edg_wll_LogEventProxy(): ");
        va_end(fmt_args);

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
 * Master function for setting a current job for given context.
 * \note Should be called before any logging call.
 * \param[in,out] context       context to work with
 * \param[in] job               further logging calls are related to this job
 * \param[in] code              sequence code as obtained from previous component
 * \param[in] user              user credentials
 * \param[in] seq_code_flags    flags on code handling (\see API documentation)
 * \param[in] logging flags	as defined by EDG_WLL_LOGFLAG_*
 *-----------------------------------------------------------------------
 */
static int edg_wll_SetLoggingJobMaster(
        edg_wll_Context ctx,
        glite_jobid_const_t job,
        const char *code,
	const char *user,
        int seq_code_flags,
	int logging_flags)
{
        int     err;
	char	*code_loc = NULL;
	char    *p_user;

        edg_wll_ResetError(ctx);

        if (!job) return edg_wll_SetError(ctx,EINVAL,"edg_wll_SetLoggingJobMaster(): jobid is null");

        edg_wlc_JobIdFree(ctx->p_jobid);
        if ((err = edg_wlc_JobIdDup(job,&ctx->p_jobid))) {
                edg_wll_SetError(ctx,err,"edg_wll_SetLoggingJobMaster(): edg_wlc_JobIdDup() error");
		goto edg_wll_setloggingjobmaster_end;
	}

	/* add user credentials to context */
	if ((logging_flags & EDG_WLL_LOGFLAG_PROXY) && user) {
		edg_wll_SetParamString(ctx, EDG_WLL_PARAM_LBPROXY_USER, user);
	} else {
		edg_wll_GssStatus	gss_stat;
		edg_wll_GssCred	cred = NULL;

		/* do not overwrite user param if already set */
		edg_wll_GetParam(ctx, EDG_WLL_PARAM_LBPROXY_USER, &p_user);
		if(NULL == p_user) {
			/* acquire  gss credentials */
			err = edg_wll_gss_acquire_cred(
				ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename,
				ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_key_filename,
				&cred, &gss_stat);
			/* give up if unable to acquire prescribed credentials */
			if (err) {
				edg_wll_SetErrorGss(ctx, "failed to load GSI credentials", &gss_stat);
				
				// XXX: stop here - further changes need to be done in 
				//	edg_wll_gss_connect() to support annonymous connetion
				return edg_wll_SetError(ctx, ENOENT, "No credentials found.");
				
				edg_wll_SetParamString(ctx, EDG_WLL_PARAM_LBPROXY_USER, EDG_WLL_LOG_USER_DEFAULT);
			} else {
				edg_wll_SetParamString(ctx, EDG_WLL_PARAM_LBPROXY_USER, cred->name);
			}
			if (cred != NULL)
				edg_wll_gss_release_cred(&cred, NULL);
		}
	}

	/* query LBProxyServer for sequence code if not user-suplied */
	/* TODO: merge - check if it is really working properly after the unification of proxy and server */
	if (logging_flags & EDG_WLL_LOGFLAG_PROXY) {
		if (!code) {
			if (edg_wll_QuerySequenceCodeProxy(ctx, job, &code_loc))
				goto edg_wll_setloggingjobmaster_end;	
		}
	}
		
	if (!edg_wll_SetSequenceCode(ctx, code ? code : code_loc, seq_code_flags)) {
		edg_wll_IncSequenceCode(ctx);
	}
	
edg_wll_setloggingjobmaster_end:
	if (code_loc) free(code_loc);

        return edg_wll_Error(ctx,NULL,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Set a current job for given context.
 * \note simple wrappers around edg_wll_SetLoggingJobMaster()
 *-----------------------------------------------------------------------
 */
int edg_wll_SetLoggingJob(
        edg_wll_Context ctx,
        glite_jobid_const_t job,
        const char *code,
        int seq_code_flags)
{
	return edg_wll_SetLoggingJobMaster(ctx,job,code,NULL,seq_code_flags,/* XXX */ 0);
}

int edg_wll_SetLoggingJobProxy(
        edg_wll_Context ctx, 
        glite_jobid_const_t job,
        const char *code,
        const char *user,
        int seq_code_flags)
{
	return edg_wll_SetLoggingJobMaster(ctx,job,code,user,seq_code_flags,EDG_WLL_LOGFLAG_PROXY);
}

/**
 *-----------------------------------------------------------------------
 * Master function for registering a job with L&B service.
 * \brief generic job registration
 * \param[in,out] context       context to work with
 * \param[in] flags		as defined by EDG_WLL_LOGFLAG_*
 * \param[in] job               jobId
 * \param[in] type              EDG_WLL_JOB_SIMPLE,  EDG_WLL_JOB_DAG, or EDG_WLL_JOB_PARTITIONABLE
 * \param[in] jdl               user-specified JDL
 * \param[in] ns                network server contact
 * \param[in] num_subjobs       number of subjobs to create
 * \param[in] seed              seed used for subjob id's generator.
 *      Use non-NULL value to be able to regenerate the set of jobid's
 * \param[out] subjobs          returned subjob id's
 * \param[in] wms_dn            DN of WMS handling the job
 *
 *-----------------------------------------------------------------------
 */
static int edg_wll_RegisterJobMaster(
        edg_wll_Context         ctx,
	int			flags,
        glite_jobid_const_t     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
	glite_jobid_const_t	parent,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs,
	char **			wms_dn)
{
	char	*seq,*type_s,*parent_s,*wms_dn_s;
	int	err = 0,i,seq_type;
	struct timeval sync_to;

	seq = type_s = parent_s = wms_dn_s = NULL;

	edg_wll_ResetError(ctx);
	/* use corerct sequence number type for given type of job */
	switch(type) {
	case EDG_WLL_REGJOB_PBS:
		seq_type = EDG_WLL_SEQ_PBS;
		break;
	case EDG_WLL_REGJOB_CREAM:
		seq_type = EDG_WLL_SEQ_CREAM;
		break;
	case EDG_WLL_REGJOB_CONDOR:
		seq_type = EDG_WLL_SEQ_CONDOR;
		break;
	default:
		seq_type = EDG_WLL_SEQ_NORMAL;
		break;
	}
	memcpy(&sync_to, &ctx->p_sync_timeout, sizeof sync_to);

	if ( ((flags & (EDG_WLL_LOGFLAG_DIRECT | EDG_WLL_LOGFLAG_LOCAL)) == 
				(EDG_WLL_LOGFLAG_DIRECT | EDG_WLL_LOGFLAG_LOCAL)) ||
	     ((flags & (EDG_WLL_LOGFLAG_PROXY | EDG_WLL_LOGFLAG_LOCAL)) == 
	      			(EDG_WLL_LOGFLAG_PROXY | EDG_WLL_LOGFLAG_LOCAL))
	) {
		edg_wll_SetError(ctx,EINVAL,"edg_wll_RegisterJobMaster(): wrong flag specified");
		goto edg_wll_registerjobmaster_end;
	}

	type_s = edg_wll_RegJobJobtypeToString(type);
	if (!type_s) {
		edg_wll_SetError(ctx,EINVAL,"edg_wll_RegisterJobMaster(): no jobtype specified");
		goto edg_wll_registerjobmaster_end;
	}
	if ((type == EDG_WLL_REGJOB_DAG || 
	     type == EDG_WLL_REGJOB_PARTITIONED ||
	     type == EDG_WLL_REGJOB_COLLECTION ||
	     type == EDG_WLL_REGJOB_FILE_TRANSFER_COLLECTION)
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
	if (wms_dn) {
		char *aux,*aux2;
		aux2 = strdup("");
		for (i=0; wms_dn[i]; i++) {
			asprintf(&aux,"%s%s\n",aux2,wms_dn[i]);
			free(aux2); aux2 = aux; aux = NULL;
		}
		wms_dn_s = strdup(aux2);
		free(aux2); aux2 = NULL;
	}

	if (flags & EDG_WLL_LOGFLAG_PROXY) {
		edg_wll_SetSequenceCode(ctx, NULL, seq_type);
		seq = edg_wll_GetSequenceCode(ctx);
	}
	err=edg_wll_SetLoggingJobMaster(ctx,job,seq,NULL,seq_type,flags);

	if (err != 0) {
                edg_wll_UpdateError(ctx,EINVAL,"edg_wll_RegisterJobMaster(): unable to set logging job");
                goto edg_wll_registerjobmaster_end; 

	}

	/* send the RegJob event message */
        if ((err = edg_wll_LogEventMaster(ctx,flags,
                EDG_WLL_EVENT_REGJOB, EDG_WLL_FORMAT_REGJOB,
		(char *)jdl,ns,parent_s,type_s,num_subjobs,seed,wms_dn_s)) != 0 ) {
                edg_wll_UpdateError(ctx,EINVAL,"edg_wll_RegisterJobMaster(): unable to register job");
                goto edg_wll_registerjobmaster_end; 
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
 * Register (asynchronously) one job with L&B service 
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJob(
        edg_wll_Context         ctx,
        glite_jobid_const_t     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	return edg_wll_RegisterJobMaster(ctx,EDG_WLL_LOGFLAG_DIRECT,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Asynchronous job registration with an extra ACL specifying WMS to acces the job 
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobExt(
        edg_wll_Context         ctx,
        glite_jobid_const_t     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs,
	char **			wms_dn,
	int			logging_flags)
{
	int flags;
	flags = logging_flags & EDG_WLL_LOGLFLAG_EXCL;	/* the only supported flag */
	flags |= EDG_WLL_LOGFLAG_DIRECT;

	return edg_wll_RegisterJobMaster(ctx,flags,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs,wms_dn);
}

/**
 *-----------------------------------------------------------------------
 * Register one job with L&B Proxy service 
 * \note simple wrapper around edg_wll_RegisterJobProxyMaster()
 * this is new (!LB_SERIAL_REG) edg_wll_RegisterJobProxy 
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobProxy(
        edg_wll_Context         ctx,
        glite_jobid_const_t     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
        return edg_wll_RegisterJobMaster(ctx,EDG_WLL_LOGFLAG_PROXY | EDG_WLL_LOGFLAG_DIRECT,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Proxy job registration with an extra ACL specifying WMS to acces the job 
 * \note simple wrapper around edg_wll_RegisterJobProxyMaster()
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobProxyExt(
        edg_wll_Context         ctx,
        glite_jobid_const_t     job,
        enum edg_wll_RegJobJobtype      type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs,
        char **                 wms_dn,
	int			logging_flags)
{
	int flags;
	flags = logging_flags & EDG_WLL_LOGLFLAG_EXCL;	/* the only supported flag */
	flags |= (EDG_WLL_LOGFLAG_PROXY | EDG_WLL_LOGFLAG_DIRECT);

        return edg_wll_RegisterJobMaster(ctx,flags,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs,wms_dn);
}

#ifdef LB_PERF
/**
 *-----------------------------------------------------------------------
 * Register one job with L&B Proxy service ONLY
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 * useful for performace measuring
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobProxyOnly(
        edg_wll_Context         ctx,
        glite_jobid_const_t     job,
        enum edg_wll_RegJobJobtype	type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	return edg_wll_RegisterJobMaster(ctx,EDG_WLL_LOGFLAG_PROXY,job,type,jdl,ns,NULL,num_subjobs,seed,subjobs,NULL);
}

/**
 *-----------------------------------------------------------------------
 * Register one job with L&B Proxy service
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 * this is original edg_wll_RegisterJobProxy, krept only for performance measuring
 *-----------------------------------------------------------------------
 */
int edg_wll_RegisterJobProxyOld(
        edg_wll_Context         ctx,
        glite_jobid_const_t     job,
        enum edg_wll_RegJobJobtype      type,
        const char *            jdl,
        const char *            ns,
        int                     num_subjobs,
        const char *            seed,
        edg_wlc_JobId **        subjobs)
{
	int	ret=0;

        /* first register with bkserver ... */
        ret=edg_wll_RegisterJobMaster(ctx,EDG_WLL_LOGFLAG_DIRECT,
		job,type,jdl,ns,NULL,num_subjobs,seed,subjobs,NULL);
	if (ret) return ret;

        /* ... and then with L&B Proxy */
        ret=edg_wll_RegisterJobMaster(ctx,EDG_WLL_LOGFLAG_PROXY,
		job,type,jdl,ns,NULL,num_subjobs,seed,subjobs,NULL);

	return ret;
}

#endif

/**
 *-----------------------------------------------------------------------
 * Master function for registering batch of subjobs 
 * \note simple wrapper around edg_wll_RegisterJobMaster()
 *-----------------------------------------------------------------------
 */
static int edg_wll_RegisterSubjobsMaster(
	edg_wll_Context 	ctx,
	int			logging_flags,
	glite_jobid_const_t 	parent,
	enum edg_wll_RegJobJobtype      type,
	char const * const * 	jdls, 
	const char * 		ns, 
	int 			nsubjobs,
	edg_wlc_JobId const * 	subjobs)
{
	char const * const	*pjdl;
	edg_wlc_JobId const	*psubjob;
	edg_wlc_JobId		oldctxjob;
	char *			oldctxseq;
	int                     errcode = 0;
	char *                  errdesc = NULL;
	int 			i;

	if (edg_wll_GetLoggingJob(ctx, &oldctxjob)) return edg_wll_Error(ctx, NULL, NULL);
	oldctxseq = edg_wll_GetSequenceCode(ctx);

	pjdl = jdls;
	psubjob = subjobs;
	
	if (type == EDG_WLL_REGJOB_SIMPLE)
		while (*pjdl != NULL) {
			if (edg_wll_RegisterJobMaster(ctx, logging_flags,
				*psubjob, EDG_WLL_REGJOB_SIMPLE, *pjdl,
				ns, parent, 0, NULL, NULL, NULL) != 0) {
				errcode = edg_wll_Error(ctx, NULL, &errdesc);
				goto edg_wll_registersubjobsmaster_end;
			}
			pjdl++; psubjob++;
		}
	else if (type == EDG_WLL_REGJOB_FILE_TRANSFER)
		for (i = 0; i < nsubjobs; i++){
	                if (edg_wll_RegisterJobMaster(ctx, logging_flags,
                	        *psubjob, EDG_WLL_REGJOB_FILE_TRANSFER, NULL,
        	                ns, parent, 0, NULL, NULL, NULL) != 0) {
	                        errcode = edg_wll_Error(ctx, NULL, &errdesc);
                        	goto edg_wll_registersubjobsmaster_end;
                	}
        	        psubjob++;
	        }	
	else {
		errcode = 1;
		errdesc = strdup("Unsupported job type.");
		goto edg_wll_registersubjobsmaster_end;
	}

edg_wll_registersubjobsmaster_end:
	edg_wll_SetLoggingJobMaster(ctx, oldctxjob, oldctxseq, NULL, EDG_WLL_SEQ_NORMAL,logging_flags);

	if (errcode) {
                edg_wll_SetError(ctx, errcode, errdesc);
                free(errdesc);
        }
	return edg_wll_Error(ctx, NULL, NULL);
}

/**
 *-----------------------------------------------------------------------
 * Register batch of subjobs with L&B service
 * \note simple wrapper around edg_wll_RegisterSubjobsMaster()
 *-----------------------------------------------------------------------
 */ 
int edg_wll_RegisterSubjobs(
        edg_wll_Context         ctx,
        glite_jobid_const_t     parent,
        char const * const *    jdls,
        const char *            ns, 
        edg_wlc_JobId const *   subjobs)
{
	return edg_wll_RegisterSubjobsMaster(ctx,EDG_WLL_LOGFLAG_LOCAL,
		parent, EDG_WLL_REGJOB_SIMPLE, jdls, ns, 0, subjobs);
}

/**
 *-----------------------------------------------------------------------
 * Register batch of subjobs with L&B Proxy service
 * \note simple wrapper around edg_wll_RegisterSubjobsMaster()
 *-----------------------------------------------------------------------
 */ 
int edg_wll_RegisterSubjobsProxy(
        edg_wll_Context         ctx,
        glite_jobid_const_t     parent,
        char const * const *    jdls,
        const char *            ns, 
        edg_wlc_JobId const *   subjobs)
{
	return edg_wll_RegisterSubjobsMaster(ctx,EDG_WLL_LOGFLAG_PROXY,
		parent, EDG_WLL_REGJOB_SIMPLE, jdls, ns, 0, subjobs);

}

int edg_wll_RegisterFTSubjobs(
        edg_wll_Context         ctx,
        glite_jobid_const_t     parent,
        const char *            ns,
	int			nsubjobs,
        edg_wlc_JobId const *   subjobs)
{
        return edg_wll_RegisterSubjobsMaster(ctx,EDG_WLL_LOGFLAG_LOCAL,
                parent, EDG_WLL_REGJOB_FILE_TRANSFER, NULL, ns, nsubjobs, subjobs);
}

/**
 *-----------------------------------------------------------------------
 * Change ACL for given job
 *-----------------------------------------------------------------------
 */
int edg_wll_ChangeACL(
		edg_wll_Context			ctx,
		glite_jobid_const_t		jobid,
		const char			*user_id,
        	enum edg_wll_ChangeACLUser_id_type      user_id_type,
        	enum edg_wll_ChangeACLPermission        permission,
        	enum edg_wll_ChangeACLPermission_type   permission_type,
        	enum edg_wll_ChangeACLOperation         operation)
{
	if ( edg_wll_SetLoggingJob(ctx, jobid, NULL, EDG_WLL_SEQ_NORMAL) == 0 ) {
		edg_wll_LogEventMaster(ctx, EDG_WLL_LOGFLAG_LOCAL | EDG_WLL_LOGFLAG_SYNC, 
			EDG_WLL_EVENT_CHANGEACL, EDG_WLL_FORMAT_CHANGEACL,
			user_id, user_id_type, permission, permission_type, operation);
	}

	return edg_wll_Error(ctx,NULL,NULL);
}
