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


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>

#include "glite/lb/lb_plain_io.h"
#include "glite/lb/il_msg.h"
#include "glite/lb/il_string.h"
#include "glite/lb/connpool.h"

#include "prod_proto.h"
#include "producer.h"
#include "connection.h"

#if defined(FREEBSD) || defined(__FreeBSD__) && !defined(TCP_CORK)
#define TCP_CORK TCP_NOPUSH
#endif

static const char* socket_path="/tmp/lb_proxy_store.sock";

/**
 *----------------------------------------------------------------------
 * Handle GSS failures on the client side
 *----------------------------------------------------------------------
 */
/* TODO: restructuring - move handle_gss_failures to a separate file 
	and use it both in producer (here) and consumer (connection.c)
*/
static
int handle_gss_failures(edg_wll_Context ctx, int code, edg_wll_GssStatus *gss_code, const char *text)
{
        static char     err[256];
        int             myerrno, ret;

	myerrno = errno;
	ret = 0;

	edg_wll_ResetError(ctx);

	if(code>0)
                return(0);

	switch(code) {
                case EDG_WLL_GSS_ERROR_EOF: 
			snprintf(err, sizeof(err), "%s;; GSS Error: EOF occured;", text);	
			ret = edg_wll_SetError(ctx,ENOTCONN,err);
			break;
                case EDG_WLL_GSS_ERROR_TIMEOUT: 
			snprintf(err, sizeof(err), "%s;; GSS Error: timeout expired;", text);	
			ret = edg_wll_SetError(ctx,ENOTCONN,err);
			break;
		case EDG_WLL_GSS_ERROR_ERRNO: 
			{
				const char *msg1;
				char *msg2;
				msg1 = strerror(myerrno);
				asprintf(&msg2, "%s;; System Error: %s", text, msg1);
				ret = edg_wll_SetError(ctx,ENOTCONN,msg2);
				free(msg2);
			}
			break;
                case EDG_WLL_GSS_ERROR_GSS:
			snprintf(err, sizeof(err), "%s;; GSS Error: GSS failure occured", text);
			ret = edg_wll_SetErrorGss(ctx,err,gss_code);
                        break;
                case EDG_WLL_GSS_ERROR_HERRNO:
                        { 
                                const char *msg1;
                                char *msg2;
                                msg1 = hstrerror(myerrno);
                                asprintf(&msg2, "%s;; GSS Error: %s", text, msg1);
                                ret = edg_wll_SetError(ctx,EDG_WLL_ERROR_DNS, msg2);
                                free(msg2);
                        }
                        break;
                default:
			snprintf(err, sizeof(err), "%s;; GSS Error: unknown failure", text);
                        ret = edg_wll_SetError(ctx,ECONNREFUSED,err);
                        break;
	}
	return ret;
}

/* TODO: restructuring - start using the following to provide better error handling when communicating with proxy
	and use it also both in producer and consumer
*/
#if 0	/* unused */
/**
 *----------------------------------------------------------------------
 * Handle UNIX socket failures on the client side
 *----------------------------------------------------------------------
 */
static
int edg_wll_log_proto_handle_plain_failures(edg_wll_Context ctx, int code, const char *text)
{
	return 0;
}
#endif

/*
 *----------------------------------------------------------------------
 * get_reply_plain, get_reply_gss  - read reply from server
 *
 *  Returns: -1       - error reading message, 
 *           code > 0 - error code from server
 *----------------------------------------------------------------------
 */
struct reader_data {
	edg_wll_Context ctx;
	void *conn;
};

static 
int plain_reader(void *user_data, char *buffer, int max_len)
{
	struct reader_data *data = (struct reader_data *)user_data;
	int len;

	len = edg_wll_plain_read_full(data->conn, buffer, max_len, &data->ctx->p_tmp_timeout);
	if(len < 0) {
		edg_wll_SetError(data->ctx, errno, "edg_wll_plain_read_full()");
		edg_wll_UpdateError(data->ctx, EDG_WLL_IL_PROTO, "plain_reader(): error reading message data");
	}

	return(len);
}

static
int get_reply_plain(edg_wll_Context ctx, edg_wll_PlainConnection *conn, char **buf, int *code_maj, int *code_min)
{
	char *msg=NULL;
	int len;
	struct reader_data data;

	data.ctx = ctx;
	data.conn = conn;
	len = read_il_data(&data, &msg, plain_reader);
	if(len < 0) {
		edg_wll_SetError(ctx, EDG_WLL_IL_PROTO, "get_reply_plain(): error reading message");
		goto get_reply_plain_end;
	}

	if(decode_il_reply(code_maj, code_min, buf, msg) < 0) {
		edg_wll_SetError(ctx, EDG_WLL_IL_PROTO, "get_reply_plain(): error decoding message");
		goto get_reply_plain_end;
	}

get_reply_plain_end:
	if(msg) free(msg);
	return edg_wll_Error(ctx,NULL,NULL);
}


static 
int gss_reader(void *user_data, char *buffer, int max_len)
{
	struct reader_data *data = (struct reader_data *)user_data;
	int ret; 
	size_t len;
	edg_wll_GssStatus gss_code;

	ret = edg_wll_gss_read_full(data->conn, buffer, max_len, &data->ctx->p_tmp_timeout,
				    &len, &gss_code);
	if(ret < 0) {
		handle_gss_failures(data->ctx, ret, &gss_code, "edg_wll_gss_read_full");
		edg_wll_UpdateError(data->ctx, EDG_WLL_IL_PROTO, "gss_reader(): error reading message");
	}

	return(ret);
}


static
int get_reply_gss(edg_wll_Context ctx, edg_wll_GssConnection *conn, char **buf, int *code_maj, int *code_min)
{
	char *msg = NULL;
	int code;
	struct reader_data data;

	data.ctx = ctx;
	data.conn = conn;
	code = read_il_data(&data, &msg, gss_reader);
	if(code < 0) {
		edg_wll_UpdateError(ctx, EDG_WLL_IL_PROTO, "get_reply_gss(): error reading reply");
		goto get_reply_gss_end;
	}

	if(decode_il_reply(code_maj, code_min, buf, msg) < 0) {
		char *et;
		asprintf(&et,"get_reply_gss(): error decoding reply \"%s\"", msg);
		edg_wll_UpdateError(ctx, EDG_WLL_IL_PROTO, et);
		if (et) free(et);
		goto get_reply_gss_end;
	}

get_reply_gss_end:
	if(msg) free(msg);
	return edg_wll_Error(ctx,NULL,NULL);
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

	edg_wll_ResetError(ctx);
	edg_wll_poolLock(); 

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_connect: setting connection to local-logger %s:%d (remaining timeout %d.%06d sec)\n",
		ctx->p_destination,ctx->p_dest_port,
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
	/* check if connection already in pool */
	if ( (index = ConnectionIndex(ctx, ctx->p_destination, ctx->p_dest_port)) == -1 ) {
		if (ctx->connections->connOpened == ctx->connections->poolSize)
			if (ReleaseConnection(ctx, NULL, 0)) {
                    		answer = edg_wll_SetError(ctx,EAGAIN,"cannot release connection (pool size exceeded)");
				edg_wll_poolUnlock();
				goto edg_wll_log_connect_end;
			}
		index = AddConnection(ctx, ctx->p_destination, ctx->p_dest_port);
		if (index < 0) {
                    answer = edg_wll_SetError(ctx,EAGAIN,"cannot add connection to pool");
		    edg_wll_poolUnlock();
		    goto edg_wll_log_connect_end;
		}
#ifdef EDG_WLL_LOG_STUB	
		fprintf(stderr,"edg_wll_log_connect: connection to local-logger %s:%d added as No. %d in the pool\n",
			ctx->connections->connPool[index].peerName,
			ctx->connections->connPool[index].peerPort,index);
#endif
	}
#ifdef EDG_WLL_LOG_STUB   
	else fprintf(stderr,"edg_wll_log_connect: connection to %s:%d exists (No. %d in the pool) - reusing\n",
			ctx->connections->connPool[index].peerName,
			ctx->connections->connPool[index].peerPort,index);
#endif  

	// Unlock the pool here but lock the connection in question
	edg_wll_connectionTryLock(ctx, index);
	edg_wll_poolUnlock();

	/* acquire gss credentials */
	ret = edg_wll_gss_acquire_cred_gsi(
	      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename,
	      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_key_filename,
	      &ctx->connections->connPool[index].gsiCred, &gss_stat);
	/* give up if unable to acquire prescribed credentials, otherwise go on anonymously */
	if (ret && ctx->p_proxy_filename) {
		answer = edg_wll_SetErrorGss(ctx, "edg_wll_gss_acquire_cred_gsi(): failed to load GSI credentials", &gss_stat);
		goto edg_wll_log_connect_err;
	}
	if (ctx->connections->connPool[index].gsiCred)
		my_subject_name = ctx->connections->connPool[index].gsiCred->name;
        
#ifdef EDG_WLL_LOG_STUB
	if (my_subject_name != NULL) {
		fprintf(stderr,"edg_wll_log_connect: using certificate: %s\n",my_subject_name);
	} else {
		fprintf(stderr,"edg_wll_log_connect: going on anonymously!\n");
	}
#endif

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_connect: opening connection to local-logger %s:%d\n",
			ctx->connections->connPool[index].peerName,
			ctx->connections->connPool[index].peerPort);
#endif
	/* gss_connect */
	if (ctx->connections->connPool[index].gss.context == NULL) {
		int	opt;

	/* acquire gss credentials */
	ret = edg_wll_gss_acquire_cred_gsi(
	      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename,
	      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_key_filename,
	      &ctx->connections->connPool[index].gsiCred, &gss_stat);

	/* give up if unable to acquire prescribed credentials, otherwise go on anonymously */
	if (ret && ctx->p_proxy_filename) {
		answer = edg_wll_SetErrorGss(ctx, "edg_wll_gss_acquire_cred_gsi(): failed to load GSI credentials", &gss_stat);
		goto edg_wll_log_connect_err;
	}
	if (ctx->connections->connPool[index].gsiCred)
		my_subject_name = ctx->connections->connPool[index].gsiCred->name;

#ifdef EDG_WLL_LOG_STUB
	if (my_subject_name != NULL) {
		fprintf(stderr,"edg_wll_log_connect: using certificate: %s\n",my_subject_name);
	} else {
		fprintf(stderr,"edg_wll_log_connect: going on anonymously!\n");
	}
#endif
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
		opt = 0;
		setsockopt(ctx->connections->connPool[index].gss.sock,IPPROTO_TCP,TCP_CORK,(const void *) &opt,sizeof opt);
		opt = 1;
		setsockopt(ctx->connections->connPool[index].gss.sock,IPPROTO_TCP,TCP_NODELAY,(const void *) &opt,sizeof opt);
		goto edg_wll_log_connect_end;
	} else goto edg_wll_log_connect_end;

edg_wll_log_connect_err:
	if (index >= 0) {
		CloseConnection(ctx, index);
		edg_wll_connectionUnlock(ctx, index);
	}
	index = -1;

edg_wll_log_connect_end:
	if (index >= 0) edg_wll_connectionTryLock(ctx, index);

//	edg_wll_poolUnlock();
//      ZS, 2 Feb 2010: Overall pool lock replaced with a connection-specific 
//      lock for the most part


#ifdef EDG_WLL_LOG_STUB
	if (answer) {
		fprintf(stderr,"edg_wll_log_connect: error (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
	} else {
		fprintf(stderr,"edg_wll_log_connect: done o.k. (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
	}
#endif
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
	ret = CloseConnection(ctx,conn);
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

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_write: sending header\n");
#endif
	sprintf(header,"%s",EDG_WLL_LOG_SOCKET_HEADER);
	header[EDG_WLL_LOG_SOCKET_HEADER_LENGTH]='\0';
	if ((err = edg_wll_gss_write_full(&ctx->connections->connPool[conn].gss, header, EDG_WLL_LOG_SOCKET_HEADER_LENGTH, &ctx->p_tmp_timeout, &count, &gss_code)) < 0) {
		answer = handle_gss_failures(ctx,err,&gss_code,"edg_wll_gss_write_full()");
		edg_wll_UpdateError(ctx,answer,"edg_wll_log_write(): error sending header");
		return -1;
	}
	sent += count;

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_write: sending message size\n");
#endif
	count = 0;
	if ((err = edg_wll_gss_write_full(&ctx->connections->connPool[conn].gss, size_end, 4, &ctx->p_tmp_timeout, &count, &gss_code)) < 0) {
                answer = handle_gss_failures(ctx,err,&gss_code,"edg_wll_gss_write_full()");
                edg_wll_UpdateError(ctx,answer,"edg_wll_log_write(): error sending message size");
                return -1;
        }
	sent += count;

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_write: sending message...\n");
#endif
	count = 0;
	if (( err = edg_wll_gss_write_full(&ctx->connections->connPool[conn].gss, logline, size, &ctx->p_tmp_timeout, &count, &gss_code)) < 0) {
		answer = handle_gss_failures(ctx,err,&gss_code,"edg_wll_gss_write_full()");
		edg_wll_UpdateError(ctx,answer,"edg_wll_log_write(): error sending message");
		return -1;
	}
	sent += count;

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_write: done (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
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

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_read: reading answer from local-logger\n");
#endif
	count = 0;
	if ((err = edg_wll_gss_read_full(&ctx->connections->connPool[conn].gss, answer_end, 4, &ctx->p_tmp_timeout, &count, &gss_code)) < 0 ) {
		answer = handle_gss_failures(ctx,err,&gss_code,"edg_wll_gss_read_full()");
		edg_wll_UpdateError(ctx,answer,"edg_wll_log_read(): error reading answer from local-logger");
		return -1;
	} else {
		answer = answer_end[3]; answer <<=8;
		answer |= answer_end[2]; answer <<=8;
		answer |= answer_end[1]; answer <<=8;
		answer |= answer_end[0];
#ifdef EDG_WLL_LOG_STUB
		fprintf(stderr,"edg_wll_log_read: read answer \"%d\"\n",answer);
#endif
		edg_wll_SetError(ctx,answer,"edg_wll_log_read(): answer read from locallogger");
	}

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_read: done (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
        return count;
}


/**
 *----------------------------------------------------------------------
 * connect to lbproxy
 *----------------------------------------------------------------------
 */
int edg_wll_log_proxy_connect(edg_wll_Context ctx, edg_wll_PlainConnection *conn)
{
	int	answer = 0, retries;
	int 	flags;
	struct sockaddr_un saddr;

	edg_wll_ResetError(ctx);

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_proxy_connect: setting connection to lbroxy (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
	conn->sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (conn->sock < 0) {
		edg_wll_SetError(ctx,answer = errno,"edg_wll_log_proxy_connect(): socket() error");
		goto edg_wll_log_proxy_connect_end;
	}
	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	strcpy(saddr.sun_path, ctx->p_lbproxy_store_sock?
				ctx->p_lbproxy_store_sock: socket_path);
	if ((flags = fcntl(conn->sock, F_GETFL, 0)) < 0 || fcntl(conn->sock, F_SETFL, flags | O_NONBLOCK) < 0) {
		edg_wll_SetError(ctx,answer = errno,"edg_wll_log_proxy_connect(): fcntl() error");
		close(conn->sock); conn->sock = -1;
		goto edg_wll_log_proxy_connect_end;
	}
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_proxy_connect: opening connection to lbproxy (socket '%s')\n",
		ctx->p_lbproxy_store_sock? ctx->p_lbproxy_store_sock: socket_path);
#endif
	retries = 0;
	while ((answer = connect(conn->sock, (struct sockaddr *)&saddr, sizeof(saddr))) < 0 &&
			errno == EAGAIN &&
			ctx->p_tmp_timeout.tv_sec >= 0 && ctx->p_tmp_timeout.tv_usec >= 0 &&
			!(ctx->p_tmp_timeout.tv_sec == 0 && ctx->p_tmp_timeout.tv_usec == 0)
			)
	{
		struct timespec ns = { 0, PROXY_CONNECT_RETRY * 1000000 /* 10 ms */ },rem;

		nanosleep(&ns,&rem);

		ctx->p_tmp_timeout.tv_usec -= ns.tv_nsec/1000;
		ctx->p_tmp_timeout.tv_usec += rem.tv_nsec/1000;

		ctx->p_tmp_timeout.tv_sec -= ns.tv_sec;
		ctx->p_tmp_timeout.tv_sec += rem.tv_sec;

		if (ctx->p_tmp_timeout.tv_usec < 0) {
			ctx->p_tmp_timeout.tv_usec += 1000000;
			ctx->p_tmp_timeout.tv_sec--;
		}
		retries++;
	}

	if (answer) {
		edg_wll_SetError(ctx,answer = (errno == EAGAIN ? ETIMEDOUT : errno),"edg_wll_log_proxy_connect()");
		close(conn->sock); conn->sock = -1;
	}

#ifdef EDG_WLL_LOG_STUB
	if (retries) fprintf(stderr,"edg_wll_log_proxy_connect: there were %d connect retries\n",retries);
#endif

edg_wll_log_proxy_connect_end:
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_proxy_connect: done (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
	return answer;
}

/**
 *----------------------------------------------------------------------
 * close connection to lbproxy
 *----------------------------------------------------------------------
 */
int edg_wll_log_proxy_close(edg_wll_Context ctx, edg_wll_PlainConnection *conn)
{
	return edg_wll_plain_close(conn);
}

/**
 *----------------------------------------------------------------------
 * write/send to lbproxy
 *----------------------------------------------------------------------
 */
int edg_wll_log_proxy_write(edg_wll_Context ctx, edg_wll_PlainConnection *conn, edg_wll_LogLine logline)
{
	int  len,count = 0;
	char *buffer;

	edg_wll_ResetError(ctx);

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_proxy_write: encoding message\n");
#endif
	{ 
		il_octet_string_t ll;

		ll.len = strlen(logline);
		ll.data = logline;
		len = encode_il_msg(&buffer, &ll);
	}
	if(len < 0) {
		edg_wll_SetError(ctx,errno,"encode_il_msg()");
		edg_wll_UpdateError(ctx,ENOMEM,"edg_wll_log_proto_client_proxy(): error encoding message");
		return -1;
	}

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_proxy_write: sending message\n");
#endif
	if ((count = edg_wll_plain_write_full(conn, buffer, len, &ctx->p_tmp_timeout)) < 0) {
		edg_wll_SetError(ctx, errno, "edg_wll_plain_write_full()");
		edg_wll_UpdateError(ctx, EDG_WLL_IL_PROTO,"edg_wll_log_proto_client_proxy(): error sending message to socket");
		return -1;
	}

	if (buffer) free(buffer);

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_proxy_write: done (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
	return count;
}

/**
 *----------------------------------------------------------------------
 * read/receive from lbproxy
 *----------------------------------------------------------------------
 */
int edg_wll_log_proxy_read(edg_wll_Context ctx, edg_wll_PlainConnection *conn)
{
	char *answer = NULL;
	static char et[256];
	int	err;
	int	code;
	int	lbproto_code;
	int	count;

	errno = err = code = count = 0;
	lbproto_code = 0;

	edg_wll_ResetError(ctx);

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_proxy_read: reading answer from lbproxy\n");
#endif
	if ((err = get_reply_plain(ctx, conn, &answer, &lbproto_code, &code)) != 0 ) {
		edg_wll_UpdateError(ctx, EDG_WLL_IL_PROTO,"edg_wll_log_proxy_read(): error reading answer from lbproxy");
		return -1;
	} else {
#ifdef EDG_WLL_LOG_STUB
		fprintf(stderr,"edg_wll_log_proxy_read: read answer \"%d:%d: %s\"\n",lbproto_code,code,answer);
#endif
		switch (lbproto_code) {
			case LB_OK: break;
			case LB_NOMEM: 
				edg_wll_SetError(ctx, ENOMEM, "edg_wll_log_proxy_read(): proxy out of memory"); 
				break;
			case LB_PROTO:
				edg_wll_SetError(ctx, EDG_WLL_IL_PROTO, "edg_wll_log_proxy_read(): received protocol error response"); 
				break;
			case LB_DBERR:
				snprintf(et, sizeof(et), "edg_wll_log_proxy_read(): error details from L&B Proxy server: %s", answer);
				edg_wll_SetError(ctx, code, et);
				break;
			default:
				edg_wll_SetError(ctx, EDG_WLL_IL_PROTO, "edg_wll_log_proxy_read(): received unknown protocol response"); 
				break;
		}
	}

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_proxy_read: done (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
	return 0;
}


/** 
 *----------------------------------------------------------------------
 * connect to bkserver
 *----------------------------------------------------------------------
 */
int edg_wll_log_direct_connect(edg_wll_Context ctx, edg_wll_GssConnection *conn) 
{
	int	ret,answer;
	edg_wll_GssStatus	gss_stat;
	edg_wll_GssCred	cred = NULL;
	char	*host;
	unsigned int	port;

	ret = answer = 0;

	edg_wll_ResetError(ctx);

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_direct_connect: setting connection to bkserver (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
	/* get bkserver location: */
	edg_wlc_JobIdGetServerParts(ctx->p_jobid,&host,&port);
	port +=1;
	/* acquire gss credentials */
	ret = edg_wll_gss_acquire_cred_gsi(
	      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename,
	      ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_key_filename,
	      &cred, &gss_stat);
	/* give up if unable to acquire prescribed credentials, otherwise go on anonymously */
	if (ret && ctx->p_proxy_filename) {
		answer = edg_wll_SetErrorGss(ctx, "edg_wll_gss_acquire_cred_gsi(): failed to load GSI credentials", &gss_stat);
		goto edg_wll_log_direct_connect_end;
	}
#ifdef EDG_WLL_LOG_STUB
	if (cred && cred->name) {
/* TODO: merge - shouldn't be probably ctx->p_user_lbproxy but some new parameter, eg. ctx->p_user
	related to the change in producer.c
*/
		edg_wll_SetParamString(ctx, EDG_WLL_PARAM_LBPROXY_USER, cred->name);
		fprintf(stderr,"edg_wll_log_direct_connect: using certificate: %s\n", cred->name);
	} else {
		fprintf(stderr,"edg_wll_log_direct_connect: going on anonymously\n");
	}
	fprintf(stderr,"edg_wll_log_direct_connect: opening connection to bkserver (host '%s', port '%d')\n", host, port);
#endif
	if ((answer = edg_wll_gss_connect(cred,host,port,
			&ctx->p_tmp_timeout, conn, &gss_stat)) < 0) {
		answer = handle_gss_failures(ctx,answer,&gss_stat,"edg_wll_gss_connect()");
		goto edg_wll_log_direct_connect_end;
	}

edg_wll_log_direct_connect_end:
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_direct_connect: done (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
	if (cred != NULL)
		edg_wll_gss_release_cred(&cred, NULL);
	if (host) free(host);

	return answer;
}

/**
 *----------------------------------------------------------------------
 * close connection to bkserver
 *----------------------------------------------------------------------
 */
int edg_wll_log_direct_close(edg_wll_Context ctx, edg_wll_GssConnection *conn) 
{
	return edg_wll_gss_close(conn,&ctx->p_tmp_timeout);
}

/**
 *----------------------------------------------------------------------
 * write/send to bkserver
 *----------------------------------------------------------------------
 */ 
int edg_wll_log_direct_write(edg_wll_Context ctx, edg_wll_GssConnection *conn, edg_wll_LogLine logline)
{
	size_t  len,count=0;
	int	err;
	char *buffer;
	edg_wll_GssStatus gss_code;

	edg_wll_ResetError(ctx);

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_direct_write: encoding message\n");
#endif
	{ 
		il_octet_string_t ll;
		
		ll.len=strlen(logline); ll.data=logline;
		len = encode_il_msg(&buffer, &ll);
	}
	if(len < 0) {
		edg_wll_SetError(ctx, errno, "encode_il_msg()");
		edg_wll_UpdateError(ctx, ENOMEM, "edg_wll_log_proto_client_direct(): error encoding message");
		return -1;
	}
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_direct_write: sending message\n");
#endif
	count = 0;
	if (( err = edg_wll_gss_write_full(conn, buffer, len, &ctx->p_tmp_timeout, &count, &gss_code)) < 0) {
		handle_gss_failures(ctx,err,&gss_code,"edg_wll_gss_write_full()");
		edg_wll_UpdateError(ctx, EDG_WLL_IL_PROTO,"edg_wll_log_direct_write(): error sending message");
		return -1;
	}
	if (buffer) free(buffer);
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_direct_write: done (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
	return count;
}

/**
 *----------------------------------------------------------------------
 * read/receive from bkserver
 *----------------------------------------------------------------------
 */
int edg_wll_log_direct_read(edg_wll_Context ctx, edg_wll_GssConnection *con)
{
	char *answer = NULL;
	static char et[256];
	int	err;
	int	code, lbproto_code;
	int	count;

	errno = err = code = count = 0;

	edg_wll_ResetError(ctx);

#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_direct_read: reading answer from bkserver\n");
#endif
	if ((err = get_reply_gss(ctx, con, &answer, &lbproto_code, &code)) != 0 ) {
		edg_wll_UpdateError(ctx, EDG_WLL_IL_PROTO,"edg_wll_log_proto_client_direct(): error reading answer from L&B direct server");
		if (answer) free(answer);
		return -1;
	} else {
#ifdef EDG_WLL_LOG_STUB
		fprintf(stderr,"edg_wll_log_direct_read: read answer \"%d:%d: %s\"\n",lbproto_code,code,answer);
#endif
		switch (lbproto_code) {
			case LB_OK: break;
			case LB_NOMEM: 
				edg_wll_SetError(ctx, ENOMEM, "edg_wll_log_direct_read(): server out of memory"); 
				break;
			case LB_PROTO:
				edg_wll_SetError(ctx, EDG_WLL_IL_PROTO, "edg_wll_log_direct_read(): received protocol error response"); 
				break;
			case LB_DBERR:
				snprintf(et, sizeof(et), "edg_wll_log_direct_read: error details from L&B server: %s", answer);
				edg_wll_SetError(ctx, code, et);
				break;
			default:
				edg_wll_SetError(ctx, EDG_WLL_IL_PROTO, "edg_wll_log_direct_read(): received unknown protocol response"); 
				break;
		}
	}
#ifdef EDG_WLL_LOG_STUB
	fprintf(stderr,"edg_wll_log_direct_read: done (remaining timeout %d.%06d sec)\n",
		(int) ctx->p_tmp_timeout.tv_sec, (int) ctx->p_tmp_timeout.tv_usec);
#endif
	return 0;
}
