#ident "$Header$"

#include "prod_proto.h"
#include "glite/lb/producer.h"
#include "glite/lb/escape.h"
#include "glite/lb/lb_gss.h"
#include "glite/lb/lb_plain_io.h"
#include "glite/lb/il_string.h"

#include <signal.h>
#include <string.h>
#include <sys/types.h>

/*
 *----------------------------------------------------------------------
 * edg_wll_log_proto_handle_gss_failures - handle GSS failures on the client side
 *
 * Returns: errno
 *----------------------------------------------------------------------
 */
int edg_wll_log_proto_handle_gss_failures(edg_wll_Context context, int code, edg_wll_GssStatus *gss_code, const char *text)
{
        static char     err[256];
        int             ret = 0;

	edg_wll_ResetError(context);

	if(code>0)
                return(0);

	switch(code) {
                case EDG_WLL_GSS_ERROR_EOF: 
			snprintf(err, sizeof(err), "%s;; GSS Error: EOF occured;", text);	
			ret = edg_wll_SetError(context,ENOTCONN,err);
			break;
                case EDG_WLL_GSS_ERROR_TIMEOUT: 
			snprintf(err, sizeof(err), "%s;; GSS Error: timeout expired;", text);	
			ret = edg_wll_SetError(context,ENOTCONN,err);
			break;
		case EDG_WLL_GSS_ERROR_ERRNO: 
			snprintf(err, sizeof(err), "%s;; GSS Error: system error occured;", text);	
			ret = edg_wll_SetError(context,ENOTCONN,err);
			break;
                case EDG_WLL_GSS_ERROR_GSS:
			snprintf(err, sizeof(err), "%s;; GSS Error: GSS failure occured", text);
			ret = edg_wll_SetErrorGss(context,err,gss_code);
                        break;
                case EDG_WLL_GSS_ERROR_HERRNO:
                        { 
                                const char *msg1;
                                char *msg2;
                                msg1 = hstrerror(errno);
                                asprintf(&msg2, "%s;; GSS Error: %s", text, msg1);
                                ret = edg_wll_SetError(context,EDG_WLL_ERROR_DNS, msg2);
                                free(msg2);
                        }
                        break;
                default:
			snprintf(err, sizeof(err), "%s;; GSS Error: unknown failure", text);
                        ret = edg_wll_SetError(context,ECONNREFUSED,err);
                        break;
	}
	return ret;
}

/*
 *----------------------------------------------------------------------
 * get_reply_plain, get_reply_gss  - read reply from server
 *
 *  Returns: -1       - error reading message, 
 *           code > 0 - error code from server
 *----------------------------------------------------------------------
 */
static
int
get_reply_plain(edg_wll_Context context, edg_wll_PlainConnection *conn, char **buf, int *code_min)
{
	char buffer[17];
	char *msg, *p;
	int len, code;

	code = 0;
	/* get message header */
	len = edg_wll_plain_read_full(conn, buffer, 17, &context->p_tmp_timeout);
	if(len < 0) {
		edg_wll_SetError(context,LB_PROTO,"get_reply_plain(): error reading message header");
		goto get_reply_plain_end;
	}

	buffer[16] = 0;

	sscanf(buffer, "%d", &len);
	if(len > MAXLEN) {
		edg_wll_SetError(context,LB_PROTO,"get_reply_plain(): error reading message body length");
		goto get_reply_plain_end;
	}

	/* allocate room for message body */
	if((msg = malloc(len)) == NULL) {
		edg_wll_SetError(context,ENOMEM,"get_reply_plain(): no room for message body");
		goto get_reply_plain_end;
	}

	/* read all the data */
	len = edg_wll_plain_read_full(conn, msg, len, &context->p_tmp_timeout);
	if(len < 0) {
		edg_wll_SetError(context,LB_PROTO,"get_reply_plain(): error reading message body");
		goto get_reply_plain_end;
	}

	p = msg;
	p = get_int(p, &code);
	if(p == NULL) {
		edg_wll_SetError(context,LB_PROTO,"get_reply_plain(): error reading result code");
		free(msg);
		goto get_reply_plain_end;
	}
	p = get_int(p, code_min);
	if(p == NULL) {
		edg_wll_SetError(context,LB_PROTO,"get_reply_plain(): error reading result code minor");
		free(msg);
		goto get_reply_plain_end;
	}
	p = get_string(p, buf);
	if(p == NULL) {
		if(*buf) {
			free(*buf);
			*buf = NULL;
		}
		edg_wll_SetError(context,LB_PROTO,"get_reply_plain(): error reading result string");
		free(msg);
		goto get_reply_plain_end;
	}

get_reply_plain_end:
	if(msg) free(msg);
	return edg_wll_Error(context,NULL,NULL);
}

static
int
get_reply_gss(edg_wll_Context context, edg_wll_GssConnection *conn, char **buf, int *code_min)
{
	char buffer[17];
	char *msg, *p;
	int len, code, answer;
	edg_wll_GssStatus gss_code;

	code = len = answer = 0;

	/* get message header */
	code = edg_wll_gss_read_full(conn, buffer, 17, &context->p_tmp_timeout, &len, &gss_code);
	if(code < 0) {
		answer = edg_wll_log_proto_handle_gss_failures(context,code,&gss_code,"edg_wll_gss_read_full()");
		edg_wll_UpdateError(context,LB_PROTO,"get_reply_gss(): error reading message header");
		goto get_reply_gss_end;
	}

	buffer[16] = 0;

	sscanf(buffer, "%d", &len);
	if(len > MAXLEN) {
		edg_wll_SetError(context,LB_PROTO,"get_reply_gss(): error reading message body length");
		goto get_reply_gss_end;
	}

	/* allocate room for message body */
	if((msg = malloc(len)) == NULL) {
		edg_wll_SetError(context,ENOMEM,"get_reply_gss(): no room for message body");
		goto get_reply_gss_end;
	}

	/* read all the data */
	code = edg_wll_gss_read_full(conn, msg, len, &context->p_tmp_timeout, &len, &gss_code);
	if(code < 0) {
		answer = edg_wll_log_proto_handle_gss_failures(context,code,&gss_code,"edg_wll_gss_read_full()");
		edg_wll_SetError(context,LB_PROTO,"get_reply_gss(): error reading message body");
		goto get_reply_gss_end;
	}

	p = msg;
	p = get_int(p, &code);
	if(p == NULL) {
		edg_wll_SetError(context,LB_PROTO,"get_reply_gss(): error reading result code");
		free(msg);
		goto get_reply_gss_end;
	}
	p = get_int(p, code_min);
	if(p == NULL) {
		edg_wll_SetError(context,LB_PROTO,"get_reply_gss(): error reading result code minor");
		free(msg);
		goto get_reply_gss_end;
	}
	p = get_string(p, buf);
	if(p == NULL) {
		if(*buf) {
			free(*buf);
			*buf = NULL;
		}
		edg_wll_SetError(context,LB_PROTO,"get_reply_gss(): error reading result string");
		free(msg);
		goto get_reply_gss_end;
	}

get_reply_gss_end:
	if(msg) free(msg);
	return edg_wll_Error(context,NULL,NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * edg_wll_log_proto_client - client part of the logging protocol
 *   used when sending messages to local logger
 *
 * Returns: 0 if done properly or errno
 *
 * Calls:
 *
 * Algorithm:
 *
 *----------------------------------------------------------------------
 */
int edg_wll_log_proto_client(edg_wll_Context context, edg_wll_GssConnection *con, edg_wll_LogLine logline)
{
	char	header[EDG_WLL_LOG_SOCKET_HEADER_LENGTH+1];
	int	err;
	int	answer;
	u_int8_t answer_end[4];
	size_t	count;
	int	size;
	u_int8_t size_end[4];
	edg_wll_GssStatus gss_code;

	errno = err = answer = count = 0;
	size = strlen(logline)+1;
	size_end[0] = size & 0xff; size >>= 8;
	size_end[1] = size & 0xff; size >>= 8;
	size_end[2] = size & 0xff; size >>= 8;
	size_end[3] = size;
	size = strlen(logline)+1;
	edg_wll_ResetError(context);

	/* send header */
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"log_proto_client: sending header...\n");
#endif
	sprintf(header,"%s",EDG_WLL_LOG_SOCKET_HEADER);
	header[EDG_WLL_LOG_SOCKET_HEADER_LENGTH]='\0';
	if ((err = edg_wll_gss_write_full(con, header, EDG_WLL_LOG_SOCKET_HEADER_LENGTH, &context->p_tmp_timeout, &count, &gss_code)) < 0) {
		answer = edg_wll_log_proto_handle_gss_failures(context,err,&gss_code,"edg_wll_gss_write_full(}");
		edg_wll_UpdateError(context,answer,"edg_wll_log_proto_client(): error sending header");
		goto edg_wll_log_proto_client_end;
	}

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"log_proto_client: sending message size...\n");
#endif
	count = 0;
	if ((err = edg_wll_gss_write_full(con, size_end, 4, &context->p_tmp_timeout, &count, &gss_code)) < 0) {
                answer = edg_wll_log_proto_handle_gss_failures(context,err,&gss_code,"edg_wll_gss_write_full()");
                edg_wll_UpdateError(context,answer,"edg_wll_log_proto_client(): error sending message size");
                goto edg_wll_log_proto_client_end;
        }

	/* send message */
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"log_proto_client: sending message...\n");
#endif
	count = 0;
	if (( err = edg_wll_gss_write_full(con, logline, size, &context->p_tmp_timeout, &count, &gss_code)) < 0) {
		answer = edg_wll_log_proto_handle_gss_failures(context,err,&gss_code,"edg_wll_gss_write_full()");
		edg_wll_UpdateError(context,answer,"edg_wll_log_proto_client(): error sending message");
		goto edg_wll_log_proto_client_end;
	}

	/* get answer */
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"log_proto_client: reading answer from server...\n");
#endif
	count = 0;
	if ((err = edg_wll_gss_read_full(con, answer_end, 4, &context->p_tmp_timeout, &count, &gss_code)) < 0 ) {
		answer = edg_wll_log_proto_handle_gss_failures(context,err,&gss_code,"edg_wll_gss_read_full()");
/* FIXME: update the answer (in context?) to EAGAIN or not?
		answer = EAGAIN;
*/
		edg_wll_UpdateError(context,answer,"edg_wll_log_proto_client(): error getting answer");
	} else {
		answer = answer_end[3]; answer <<=8;
		answer |= answer_end[2]; answer <<=8;
		answer |= answer_end[1]; answer <<=8;
		answer |= answer_end[0];
#ifdef EDG_WLL_LOG_STUB
		fprintf(stderr,"log_proto_client: read answer \"%d\"\n",answer);
#endif
		edg_wll_SetError(context,answer,"answer read from locallogger");
	}

edg_wll_log_proto_client_end:

	return edg_wll_Error(context,NULL,NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * edg_wll_log_proto_client_proxy - client part of the logging protocol
 *   used when sending messages to L&B Proxy
 *
 * Returns: 0 if done properly or errno
 *
 * Calls:
 *
 * Algorithm:
 *
 *----------------------------------------------------------------------
 */
int edg_wll_log_proto_client_proxy(edg_wll_Context context, edg_wll_PlainConnection *conn, edg_wll_LogLine logline)
{
	char *p;  int  len;
	char *ucs = "honik6";
	char *buffer,*answer = NULL;
	static char et[256];
	int	err;
	int	code;
	int	count;

	errno = err = code = count = 0;
	edg_wll_ResetError(context);

	/* allocate enough room to hold the message */
	len = 17 + len_string(ucs) + len_string(logline);
	if((buffer = malloc(len)) == NULL) {
		edg_wll_SetError(context,ENOMEM,"edg_wll_log_proto_client_proxy(): no room for message body");
		goto edg_wll_log_proto_client_proxy_end;
	}
	p = buffer;

	/* write header */
	sprintf(p, "%16d\n", len - 17);
	p += 17;

	/* write rest of the message */
	p = put_string(p, ucs);
	p = put_string(p, logline);

	/* send message */
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"log_proto_client_proxy: sending message...\n");
#endif
	if (( count = edg_wll_plain_write_full(conn, buffer, len, &context->p_tmp_timeout)) < 0) {
		edg_wll_SetError(context,LB_PROTO,"edg_wll_log_proto_client_proxy(): error sending message to socket");
		goto edg_wll_log_proto_client_proxy_end;
	}

	/* get answer */
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"log_proto_client_proxy: reading answer from server...\n");
#endif
	if ((err = get_reply_plain(context, conn, &answer, &code)) < 0 ) {
		edg_wll_SetError(context,LB_PROTO,"edg_wll_log_proto_client_proxy(): error reading answer from L&B Proxy server");
	} else {
#ifdef EDG_WLL_LOG_STUB
		fprintf(stderr,"log_proto_client_proxy: read answer \"%d: %s\"\n",code,answer);
#endif
		snprintf(et, sizeof(et), "answer read from L&B Proxy server: %s", answer);
		edg_wll_SetError(context,code,et);
	}

edg_wll_log_proto_client_proxy_end:

	if (buffer) free(buffer);
	if (answer) free(answer);
	return edg_wll_Error(context,NULL,NULL);
}
/*
 *----------------------------------------------------------------------
 *
 * edg_wll_log_proto_client_direct - client part of the logging protocol
 *   used when sending messages directly to bkserver
 *
 * Returns: 0 if done properly or errno                                                                                 *
 * Calls:
 *
 * Algorithm:
 *
 *----------------------------------------------------------------------
 */
int edg_wll_log_proto_client_direct(edg_wll_Context context, edg_wll_GssConnection *con, edg_wll_LogLine logline)
{       
	char *p;  int  len;
	char *ucs = "honik6";
	char *buffer,*answer = NULL;
	static char et[256];
	int	err;
	int	code;
	int	count;
	edg_wll_GssStatus gss_code;

	errno = err = code = count = 0;
	edg_wll_ResetError(context);

	/* allocate enough room to hold the message */
	len = 17 + len_string(ucs) + len_string(logline);
	if((buffer = malloc(len)) == NULL) {
		edg_wll_SetError(context,ENOMEM,"edg_wll_log_proto_client_direct(): no room for message body");
		goto edg_wll_log_proto_client_direct_end;
	}
	p = buffer;

	/* write header */
	sprintf(p, "%16d\n", len - 17);
	p += 17;

	/* write rest of the message */
	p = put_string(p, ucs);
	p = put_string(p, logline);

	/* send message */
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"log_proto_client_direct: sending message...\n");
#endif
	count = 0;
	if (( err = edg_wll_gss_write_full(con, buffer, len, &context->p_tmp_timeout,  &count, &gss_code)) < 0) {
		edg_wll_log_proto_handle_gss_failures(context,err,&gss_code,"edg_wll_gss_write_full()");
		edg_wll_UpdateError(context,LB_PROTO,"edg_wll_log_proto_client_direct(): error sending message");
		goto edg_wll_log_proto_client_direct_end;
	}

	/* get answer */
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"log_proto_client_direct: reading answer from server...\n");
#endif
	if ((err = get_reply_gss(context, con, &answer, &code)) < 0 ) {
		edg_wll_SetError(context,LB_PROTO,"edg_wll_log_proto_client_direct(): error reading answer from L&B direct server");
	} else {
#ifdef EDG_WLL_LOG_STUB
		fprintf(stderr,"log_proto_client_direct: read answer \"%d: %s\"\n",code,answer);
#endif
		snprintf(et, sizeof(et), "answer read from bkserver: %s", answer);
		edg_wll_SetError(context,code,et);
	}

edg_wll_log_proto_client_direct_end:

	if (buffer) free(buffer);
	if (answer) free(answer);
	return edg_wll_Error(context,NULL,NULL);
}

