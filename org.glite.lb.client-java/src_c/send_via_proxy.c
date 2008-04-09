#include <jni.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pwd.h>
#include <sys/un.h>
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

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
        int	conn;

        edg_wll_ResetError(ctx);
        memset(&conn,0,sizeof(conn));

	/* connect to local-logger */
	if ((ret = edg_wll_log_connect(ctx,&conn))) {
		fprintf(stderr, "edg_wll_log_connect error");
		goto edg_wll_DoLogEvent_end;
	}

	/* send message */
	if ((ret = edg_wll_log_write(ctx,conn,logline)) == -1) {
		fprintf(stderr, "edg_wll_log_write error");
		goto edg_wll_DoLogEvent_end;
	}

	/* get answer */
	if ((ret = edg_wll_log_read(ctx,conn)) == -1) {
		fprintf(stderr, "edg_wll_log_read error");
	} else {
		answer = edg_wll_Error(ctx, NULL, NULL);
	}

edg_wll_DoLogEvent_end:
	if (ret) edg_wll_log_close(ctx,conn);

	return 0;
}

/**
 *----------------------------------------------------------------------
 * connect to locallogger
 *----------------------------------------------------------------------
 */
int edg_wll_log_connect(edg_wll_Context ctx, int *conn) 
{
	int	ret, answer=0, index;
	char	*my_subject_name = NULL;
	edg_wll_GssStatus	gss_stat;

	//edg_wll_ResetError(ctx);
	//edg_wll_poolLock(); 

	/* check if connection already in pool */
	if ( (index = ConnectionIndex(ctx, ctx->p_destination, ctx->p_dest_port)) == -1 ) {
		if (ctx->connections->connOpened == ctx->connections->poolSize)
			if (ReleaseConnection(ctx, NULL, 0)) 
				goto edg_wll_log_connect_end;
		index = AddConnection(ctx, ctx->p_destination, ctx->p_dest_port);
		if (index < 0) {
                    edg_wll_SetError(ctx,EAGAIN,"connection pool size exceeded");
		    goto edg_wll_log_connect_end;
		}
#if 0
	/* acquire gss credentials */
	ret = edg_wll_gss_acquire_cred_gsi(
	      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename,
	      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_key_filename,
	      &ctx->connections->connPool[index].gsiCred, &my_subject_name, &gss_stat);
	/* give up if unable to acquire prescribed credentials, otherwise go on anonymously */
	if (ret && ctx->p_proxy_filename) {
		edg_wll_SetErrorGss(ctx, "edg_wll_gss_acquire_cred_gsi(): failed to load GSI credentials", &gss_stat);
		goto edg_wll_log_connect_err;
	}
	/* gss_connect */
	if (ctx->connections->connPool[index].gss.context == GSS_C_NO_CONTEXT) {

	/* acquire gss credentials */
	ret = edg_wll_gss_acquire_cred_gsi(
	      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename,
	      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_key_filename,
	      &ctx->connections->connPool[index].gsiCred, &my_subject_name, &gss_stat);
	/* give up if unable to acquire prescribed credentials, otherwise go on anonymously */
	if (ret && ctx->p_proxy_filename) {
		edg_wll_SetErrorGss(ctx, "edg_wll_gss_acquire_cred_gsi(): failed to load GSI credentials", &gss_stat);
		goto edg_wll_log_connect_err;
	}
		if ((answer = edg_wll_gss_connect(
				ctx->connections->connPool[index].gsiCred,
				ctx->connections->connPool[index].peerName,
				ctx->connections->connPool[index].peerPort,
				&ctx->p_tmp_timeout, 
				&ctx->connections->connPool[index].gss,
				&gss_stat)) < 0) {
			answer = handle_gss_failures(ctx,answer,&gss_stat,"edg_wll_gss_connect()");
			goto edg_wll_log_connect_err;
		}
		goto edg_wll_log_connect_end;
	} else goto edg_wll_log_connect_end;

edg_wll_log_connect_err:
	if (index >= 0) CloseConnection(ctx, &index);
	index = -1;

edg_wll_log_connect_end:
	if (index >= 0) edg_wll_connectionTryLock(ctx, index);
	if (my_subject_name) free(my_subject_name);

	edg_wll_poolUnlock();

	*conn = index;
	return answer;
}

/**
 *----------------------------------------------------------------------
 * close connection to locallogger
 *----------------------------------------------------------------------
 */
int edg_wll_log_close(edg_wll_Context ctx, int conn) 
{
	int ret = 0;

	if (conn == -1) return 0;
	ret = CloseConnection(ctx,&conn);
	edg_wll_connectionUnlock(ctx,conn);
	return ret;
}

/**
 *----------------------------------------------------------------------
 * write/send to locallogger
 *----------------------------------------------------------------------
 */
int edg_wll_log_write(edg_wll_Context ctx, int conn, edg_wll_LogLine logline)
{
	char	header[EDG_WLL_LOG_SOCKET_HEADER_LENGTH+1];
	int	err;
	int	answer;
	size_t	count,sent;
	int	size;
	u_int8_t size_end[4];
	edg_wll_GssStatus gss_code;

	errno = err = answer = count = sent = 0;
	size = strlen(logline)+1;
	size_end[0] = size & 0xff; size >>= 8;
	size_end[1] = size & 0xff; size >>= 8;
	size_end[2] = size & 0xff; size >>= 8;
	size_end[3] = size;
	size = strlen(logline)+1;

	edg_wll_ResetError(ctx);

	sprintf(header,"%s",EDG_WLL_LOG_SOCKET_HEADER);
	header[EDG_WLL_LOG_SOCKET_HEADER_LENGTH]='\0';
	if ((err = edg_wll_gss_write_full(&ctx->connections->connPool[conn].gss, header, EDG_WLL_LOG_SOCKET_HEADER_LENGTH, &ctx->p_tmp_timeout, &count, &gss_code)) < 0) {
		switch (answer = handle_gss_failures(ctx,err,&gss_code,"edg_wll_gss_write_full()")) {
		case ENOTCONN:
			edg_wll_log_close(ctx,conn);
			if (edg_wll_log_connect(ctx,&conn) || 
			    edg_wll_gss_write_full(&ctx->connections->connPool[conn].gss, header, EDG_WLL_LOG_SOCKET_HEADER_LENGTH, &ctx->p_tmp_timeout, &count, &gss_code) < 0) {
				edg_wll_UpdateError(ctx,answer,"edg_wll_log_write(): error sending header");
				return -1;
			}
			break;
		case 0:
			break;
		default:
			edg_wll_UpdateError(ctx,answer,"edg_wll_log_write(): error sending header");
			return -1;
		}
	}
	sent += count;

	count = 0;
	if ((err = edg_wll_gss_write_full(&ctx->connections->connPool[conn].gss, size_end, 4, &ctx->p_tmp_timeout, &count, &gss_code)) < 0) {
                switch (answer = handle_gss_failures(ctx,err,&gss_code,"edg_wll_gss_write_full()")) {
		case ENOTCONN:
			edg_wll_log_close(ctx,conn);
			if (edg_wll_log_connect(ctx,&conn) ||
			    edg_wll_gss_write_full(&ctx->connections->connPool[conn].gss, size_end, 4, &ctx->p_tmp_timeout, &count, &gss_code) < 0) {
				edg_wll_UpdateError(ctx,answer,"edg_wll_log_write(): error sending message size");
		                return -1;
			}
			break;
		case 0:
			break;
		default:
			edg_wll_UpdateError(ctx,answer,"edg_wll_log_write(): error sending message size");
			return -1;
		
		}
        }
	sent += count;

	count = 0;
	if (( err = edg_wll_gss_write_full(&ctx->connections->connPool[conn].gss, logline, size, &ctx->p_tmp_timeout, &count, &gss_code)) < 0) {
		switch (answer = handle_gss_failures(ctx,err,&gss_code,"edg_wll_gss_write_full()")) {
		case ENOTCONN:
			edg_wll_log_close(ctx,conn);
			if (edg_wll_log_connect(ctx,&conn) ||
			    edg_wll_gss_write_full(&ctx->connections->connPool[conn].gss, logline, size, &ctx->p_tmp_timeout, &count, &gss_code) < 0) {
				edg_wll_UpdateError(ctx,answer,"edg_wll_log_write(): error sending message");
				return -1;
			}
                        break;
                case 0:
                        break;
                default:
			edg_wll_UpdateError(ctx,answer,"edg_wll_log_write(): error sending message");
			return -1;
		}
	}
	sent += count;

	return sent;
}

/**
 *----------------------------------------------------------------------
 * read/receive from locallogger
 *----------------------------------------------------------------------
 */
int edg_wll_log_read(edg_wll_Context ctx, int conn)
{
	int	err;
	int	answer;
	u_int8_t answer_end[4];
	size_t	count;
	edg_wll_GssStatus gss_code;

	errno = err = answer = count = 0;

	edg_wll_ResetError(ctx);

	count = 0;
	if ((err = edg_wll_gss_read_full(&ctx->connections->connPool[conn].gss, answer_end, 4, &ctx->p_tmp_timeout, &count, &gss_code)) < 0 ) {
		switch (answer = handle_gss_failures(ctx,err,&gss_code,"edg_wll_gss_read_full()")) {
		case ENOTCONN:
                        edg_wll_log_close(ctx,conn);
                        if (edg_wll_log_connect(ctx,&conn) ||
			    edg_wll_gss_read_full(&ctx->connections->connPool[conn].gss, answer_end, 4, &ctx->p_tmp_timeout, &count, &gss_code) < 0 ) {
				edg_wll_UpdateError(ctx,answer,"edg_wll_log_read(): error reading answer from local-logger");
				return -1;
			}
			break;
		case 0:
			break;
		default:
			edg_wll_UpdateError(ctx,answer,"edg_wll_log_read(): error reading answer from local-logger");
			return -1;
		}
	} 
	answer = answer_end[3]; answer <<=8;
	answer |= answer_end[2]; answer <<=8;
	answer |= answer_end[1]; answer <<=8;
	answer |= answer_end[0];
	edg_wll_SetError(ctx,answer,"edg_wll_log_read(): answer read from locallogger");

        return count;
}
