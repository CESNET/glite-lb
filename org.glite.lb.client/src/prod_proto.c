#ident "$Header$"

#include "prod_proto.h"
#include "glite/lb/producer.h"
#include "glite/lb/escape.h"
#include "glite/lb/lb_gss.h"

#include <signal.h>
#include <string.h>
#include <sys/types.h>

/*
 *----------------------------------------------------------------------
 *
 * edg_wll_log_proto_client - handle outgoing data
 *
 * Returns: 0 if done properly or errno
 *
 * Calls:
 *
 * Algorithm:
 *
 *----------------------------------------------------------------------
 */
int edg_wll_log_proto_client(edg_wll_Context context, edg_wll_GssConnection *con, edg_wll_LogLine logline/*, int priority,*/)
{
	char	header[EDG_WLL_LOG_SOCKET_HEADER_LENGTH+1];
	int	err;
	int	answer;
	u_int8_t answer_end[4];
	int	count;
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
	fprintf(stderr,"Sending socket header...\n");
#endif
	sprintf(header,"%s",EDG_WLL_LOG_SOCKET_HEADER);
	header[EDG_WLL_LOG_SOCKET_HEADER_LENGTH]='\0';
	if ((err = edg_wll_gss_write_full(con, header, EDG_WLL_LOG_SOCKET_HEADER_LENGTH, &context->p_tmp_timeout, &count, &gss_code)) < 0) {
		answer = edg_wll_log_proto_client_failure(context,err,&gss_code,"send header");
		goto edg_wll_log_proto_client_end;
	}

/* XXX: obsolete
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"Sending message priority...\n");
#endif
	count = 0;
        if ((err = edg_wll_gss_write_full(con, &priority, sizeof(priority), &context->p_tmp_timeout, &count, &gss_code)) < 0) {
                answer = edg_wll_log_proto_client_failure(context,err,&gss_code,"send message priority");
                goto edg_wll_log_proto_client_end;
        }
*/

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"Sending message size...\n");
#endif
	count = 0;
	if ((err = edg_wll_gss_write_full(con, size_end, 4, &context->p_tmp_timeout, &count, &gss_code)) < 0) {
                answer = edg_wll_log_proto_client_failure(context,err,&gss_code,"send message size");
                goto edg_wll_log_proto_client_end;
        }

	/* send message */
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"Sending message to socket...\n");
#endif
	count = 0;
	if (( err = edg_wll_gss_write_full(con, logline, size, &context->p_tmp_timeout, &count, &gss_code)) < 0) {
		answer = edg_wll_log_proto_client_failure(context,err,&gss_code,"send message");
		goto edg_wll_log_proto_client_end;
	}

	/* get answer */
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"Reading answer from server...\n");
#endif
	count = 0;
	if ((err = edg_wll_gss_read_full(con, answer_end, 4, &context->p_tmp_timeout, &count, &gss_code)) < 0 ) {
		answer = edg_wll_log_proto_client_failure(context,err,&gss_code,"get answer");
/* FIXME: update the answer (in context?) to EAGAIN or not?
		answer = EAGAIN;
*/
	} else {
		answer = answer_end[3]; answer <<=8;
		answer |= answer_end[2]; answer <<=8;
		answer |= answer_end[1]; answer <<=8;
		answer |= answer_end[0];
#ifdef EDG_WLL_LOG_STUB
		fprintf(stderr,"Read answer \"%d\"\n",answer);
#endif
		edg_wll_SetError(context,answer,"answer read from locallogger");
	}

edg_wll_log_proto_client_end:

	return edg_wll_Error(context,NULL,NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * edg_wll_log_proto_client_failure - handle protocol failures on the client side
 *
 * Returns: errno
 *
 *----------------------------------------------------------------------
 */
int edg_wll_log_proto_client_failure(edg_wll_Context context, int code, edg_wll_GssStatus *gss_code, const char *text)
{
	const char	*func="edg_wll_log_proto_client()";
        static char     err[256];
        int             ret = 0;
	char		*gss_err;

	edg_wll_ResetError(context);

	if(code>0)
                return(0);

	switch(code) {
                case EDG_WLL_GSS_ERROR_EOF: 
			snprintf(err, sizeof(err), "%s: Error %s, EOF occured;", func, text);	
			ret = edg_wll_SetError(context,ENOTCONN,err);
			break;
                case EDG_WLL_GSS_ERROR_TIMEOUT: 
			snprintf(err, sizeof(err), "%s: Error %s, timeout expired;", func, text);	
			ret = edg_wll_SetError(context,ENOTCONN,err);
			break;
		case EDG_WLL_GSS_ERROR_ERRNO: // XXX: perror("edg_wll_ssl_read()"); break;
			snprintf(err, sizeof(err), "%s: Error %s, system error occured;", func, text);	
			ret = edg_wll_SetError(context,ENOTCONN,err);
			break;
                case EDG_WLL_GSS_ERROR_GSS:
			snprintf(err, sizeof(err), "%s: Error %s, GSS error occured", func, text);
			edg_wll_gss_get_error(gss_code, err, &gss_err);
			ret = edg_wll_SetError(context,ENOTCONN,gss_err);
			free(gss_err);
                        break;
		default:
			break;
	}
	return ret;
}
