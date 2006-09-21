#ident "$Header$"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>

#include "glite/security/glite_gss.h"
#include "glite/lb/consumer.h"
#include "glite/lb/context-int.h"
#include "glite/lb/mini_http.h"
#include "glite/lb/connpool.h"


static void CloseConnection(edg_wll_Context ctx, int* conn_index)
{
	/* close connection ad free its structures */
	OM_uint32 min_stat;
	int cIndex;

        cIndex = conn_index[0];

	assert(ctx->connections->connOpened);
	assert(cIndex < ctx->connections->connOpened);

	edg_wll_gss_close(&ctx->connections->connPool[cIndex].gss, &ctx->p_tmp_timeout);
	if (ctx->connections->connPool[cIndex].gsiCred) 
		gss_release_cred(&min_stat, &ctx->connections->connPool[cIndex].gsiCred);
	free(ctx->connections->connPool[cIndex].peerName);
	free(ctx->connections->connPool[cIndex].buf);
	
	memset(ctx->connections->connPool + cIndex, 0, sizeof(edg_wll_ConnPool));
	
	/* if deleted conn was not the last one -> there is a 'hole' and then 	*/
	/* 'shake' together connections in pool, no holes are allowed		*/
	if (cIndex < ctx->connections->connOpened - 1) {	
		ctx->connections->connPool[cIndex] = ctx->connections->connPool[ctx->connections->connOpened - 1];
		memset(ctx->connections->connPool + ctx->connections->connOpened  - 1 , 0, sizeof(edg_wll_ConnPool));
	}
	ctx->connections->connOpened--;

        conn_index[0] = cIndex;
}



static int ConnectionIndex(edg_wll_Context ctx, const char *name, int port)
{
	int i;

        for (i=0; i<ctx->connections->connOpened;i++) { 
		if (!strcmp(name, ctx->connections->connPool[i].peerName) &&
		   (port == ctx->connections->connPool[i].peerPort)) {

			/* TryLock (next line) is in fact used only 
			   to check the mutex status */
			switch (edg_wll_connectionTryLock(ctx, i)) {
			case 0: 
				/* Connection was not locked but now it is. Since we do not
				   really know wheter we are interested in that connection, we
				   are simply unlocking it now. */
				edg_wll_connectionUnlock(ctx, i);
				return i;

			case EBUSY:
				/* Connection locked. Do not consider it */
				// try to find another free connection
				break;
			default:
				/* Some obscure error occured. Need inspection */
				perror("ConnectionIndex() - locking problem \n");
				assert(0);
			}
		}
	}
	
	return -1;
}



static int AddConnection(edg_wll_Context ctx, char *name, int port)
{
	int index = ctx->connections->connOpened;
	
	free(ctx->connections->connPool[index].peerName);	// should be empty; just to be sure
	ctx->connections->connPool[index].peerName = strdup(ctx->srvName);
	ctx->connections->connPool[index].peerPort = ctx->srvPort;
	ctx->connections->connOpened++;

	return index;
}



static void ReleaseConnection(edg_wll_Context ctx, char *name, int port)
{
	int i, index = 0;
	long min;


	if (ctx->connections->connOpened == 0) return;	/* nothing to release */
	
	if (name) {
		if ((index = ConnectionIndex(ctx, name, port)) >= 0)
			CloseConnection(ctx, &index);
	}
	else {					/* free the oldest connection*/
		min = ctx->connections->connPool[0].lastUsed.tv_sec;
		for (i=0; i<ctx->connections->connOpened; i++) {
			if (ctx->connections->connPool[i].lastUsed.tv_sec < min) {
				min = ctx->connections->connPool[i].lastUsed.tv_sec;
				index = i;
			}
		}
		CloseConnection(ctx, &index);
	}
}
			


int edg_wll_close(edg_wll_Context ctx, int* connToUse)
{
	edg_wll_ResetError(ctx);
	if (*connToUse == -1) return 0;

	CloseConnection(ctx, connToUse);

        edg_wll_connectionUnlock(ctx, *connToUse); /* Forgetting the conn. Unlocking is safe. */
		
	*connToUse = -1;
	return edg_wll_Error(ctx,NULL,NULL);
}



int edg_wll_close_proxy(edg_wll_Context ctx)
{
	edg_wll_plain_close(&ctx->connProxy->conn);

	return edg_wll_Error(ctx, NULL, NULL);
}



int edg_wll_open(edg_wll_Context ctx, int* connToUse)
{
	int index;
	edg_wll_GssStatus gss_stat;
	

	edg_wll_ResetError(ctx);

        edg_wll_poolLock(); /* We are going to search the pool, it has better be locked */

	if ( (index = ConnectionIndex(ctx, ctx->srvName, ctx->srvPort)) == -1 ) {
		/* no such open connection in pool */
		if (ctx->connections->connOpened == ctx->connections->poolSize)
			ReleaseConnection(ctx, NULL, 0);
		
		index = AddConnection(ctx, ctx->srvName, ctx->srvPort);
		
	}
	/* else - there is cached open connection, reuse it */
	
	*connToUse = index;
	
	/* XXX support anonymous connections, perhaps add a flag to the connPool
	 * struct specifying whether or not this connection shall be authenticated
	 * to prevent from repeated calls to edg_wll_gss_acquire_cred_gsi() */
	if (!ctx->connections->connPool[index].gsiCred && 
	    edg_wll_gss_acquire_cred_gsi(
	       ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename,
	       ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_key_filename,
	       &ctx->connections->connPool[index].gsiCred, NULL, &gss_stat)) {
	    edg_wll_SetErrorGss(ctx, "failed to load GSI credentials", &gss_stat);
	    goto err;
	}

	if (ctx->connections->connPool[index].gss.context == GSS_C_NO_CONTEXT) {	
		switch (edg_wll_gss_connect(ctx->connections->connPool[index].gsiCred,
				ctx->connections->connPool[index].peerName, ctx->connections->connPool[index].peerPort,
				&ctx->p_tmp_timeout,&ctx->connections->connPool[index].gss,
				&gss_stat)) {
		
			case EDG_WLL_GSS_OK: 
				goto ok;
			case EDG_WLL_GSS_ERROR_ERRNO:
				edg_wll_SetError(ctx,errno,"edg_wll_gss_connect()");
				break;
			case EDG_WLL_GSS_ERROR_GSS:
				edg_wll_SetErrorGss(ctx, "failed to authenticate to server", &gss_stat);
				break;
			case EDG_WLL_GSS_ERROR_HERRNO:
        	                { const char *msg1;
                	          char *msg2;
                        	  msg1 = hstrerror(errno);
	                          asprintf(&msg2, "edg_wll_gss_connect(): %s", msg1);
        	                  edg_wll_SetError(ctx,EDG_WLL_ERROR_DNS, msg2);
                	          free(msg2);
                        	}
				break;
			case EDG_WLL_GSS_ERROR_EOF:
				edg_wll_SetError(ctx,ECONNREFUSED,"edg_wll_gss_connect():"
					       " server closed the connection, probably due to overload");
				break;
			case EDG_WLL_GSS_ERROR_TIMEOUT:
				edg_wll_SetError(ctx,ETIMEDOUT,"edg_wll_gss_connect()");
				break;
		}
	}
	else goto ok;

err:
	/* some error occured; close created connection
	 * and free all fields in connPool[index] */
	CloseConnection(ctx, &index);
	*connToUse = -1;
ok:	

        if (*connToUse>-1) edg_wll_connectionLock(ctx, *connToUse); /* Lock the connection */

	edg_wll_poolUnlock(); /* One way or the other, there are no more pool-wide operations */

//	xxxxx

	return edg_wll_Error(ctx,NULL,NULL);
}



int edg_wll_open_proxy(edg_wll_Context ctx)
{
	struct sockaddr_un	saddr;
	int			flags;
	

	if (ctx->connProxy->conn.sock > -1) {
		// XXX: test path socket here?
		return edg_wll_ResetError(ctx);
	}
	ctx->connProxy->conn.sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (ctx->connProxy->conn.sock < 0) {
		edg_wll_SetError(ctx, errno, "socket() error");
		goto err;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	if (!ctx->p_lbproxy_serve_sock) {
		edg_wll_SetError(ctx, EINVAL, "Proxy socket path not set!");
		goto err;
	}
	
	if (strlen(ctx->p_lbproxy_serve_sock) > 108) {	// UNIX_PATH_MAX (def. in linux/un.h)
							// but not defined in sys/un.h
		 edg_wll_SetError(ctx, EINVAL, "proxy_filename too long!");
		 goto err;
	}
	strcpy(saddr.sun_path, ctx->p_lbproxy_serve_sock);

	if ((flags = fcntl(ctx->connProxy->conn.sock, F_GETFL, 0)) < 0 || 
			fcntl(ctx->connProxy->conn.sock, F_SETFL, flags | O_NONBLOCK) < 0) {
		edg_wll_SetError(ctx, errno, "fcntl()");
		goto err;
	}

	if (connect(ctx->connProxy->conn.sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		edg_wll_SetError(ctx, errno, "connect()");
		goto err;
	}

	return edg_wll_Error(ctx,NULL,NULL);	
	
err:
	/* some error occured; close created connection */

	edg_wll_close_proxy(ctx);
		
	return edg_wll_Error(ctx,NULL,NULL);
}
	


/* transform HTTP error code to ours */
int http_check_status(
	edg_wll_Context ctx,
	char *response)

{
	int	code = HTTP_INTERNAL,len = 0;

	edg_wll_ResetError(ctx);
	sscanf(response,"HTTP/%*f %n%d",&len,&code);
	switch (code) {
		case HTTP_OK: 
			break;
		/* soft errors - some useful data may be returned too */
		case HTTP_UNAUTH: /* EPERM */
		case HTTP_NOTFOUND: /* ENOENT */
		case HTTP_NOTIMPL: /* ENOSYS */
		case HTTP_UNAVAIL: /* EAGAIN */
		case HTTP_INVALID: /* EINVAL */
			break;
		case EDG_WLL_GSS_ERROR_HERRNO:
                        { const char *msg1;
                          char *msg2;
                          msg1 = hstrerror(errno);
                          asprintf(&msg2, "edg_wll_gss_connect(): %s", msg1);
                          edg_wll_SetError(ctx,EDG_WLL_ERROR_DNS, msg2);
                          free(msg2);
                        }
			break;
		case HTTP_NOTALLOWED:
			edg_wll_SetError(ctx, ENXIO, "Method Not Allowed");
			break;
		case HTTP_UNSUPPORTED:
			edg_wll_SetError(ctx, ENOTSUP, "Protocol versions incompatible");
			break;								
		case HTTP_INTERNAL:
			/* fall through */
		default: 
			edg_wll_SetError(ctx,EDG_WLL_ERROR_SERVER_RESPONSE,response+len);
	}
	return edg_wll_Error(ctx,NULL,NULL);
}



int edg_wll_http_send_recv(
	edg_wll_Context ctx,
	char *request,
	const char * const *req_head,
	char *req_body,
	char **response,
	char ***resp_head,
	char **resp_body)
{
	int	ec;
	char	*ed = NULL;
	int	connToUse = -1; //Index of the connection to use. Used to be a context member.

	if (edg_wll_open(ctx,&connToUse)) return edg_wll_Error(ctx,NULL,NULL);
	
	switch (edg_wll_http_send(ctx,request,req_head,req_body,&ctx->connections->connPool[connToUse])) {
		case ENOTCONN:
			edg_wll_close(ctx,&connToUse);
			if (edg_wll_open(ctx,&connToUse)
				|| edg_wll_http_send(ctx,request,req_head,req_body,&ctx->connections->connPool[connToUse]))
					goto err;
			/* fallthrough */
		case 0: break;
		default: goto err;
	}

	switch (edg_wll_http_recv(ctx,response,resp_head,resp_body,&ctx->connections->connPool[connToUse])) {
		case ENOTCONN:
			edg_wll_close(ctx,&connToUse);
			if (edg_wll_open(ctx,&connToUse)
				|| edg_wll_http_send(ctx,request,req_head,req_body,&ctx->connections->connPool[connToUse])
				|| edg_wll_http_recv(ctx,response,resp_head,resp_body,&ctx->connections->connPool[connToUse]))
					goto err;
			/* fallthrough */
		case 0: break;
		default: goto err;
	}
	
	assert(connToUse >= 0);
	gettimeofday(&ctx->connections->connPool[connToUse].lastUsed, NULL);
	return 0;

err:
	ec = edg_wll_Error(ctx,NULL,&ed);
	edg_wll_close(ctx,&connToUse);
	edg_wll_SetError(ctx,ec,ed);
	free(ed);
	return ec;
}



int edg_wll_http_send_recv_proxy(
	edg_wll_Context ctx,
	char *request,
	const char * const *req_head,
	char *req_body,
	char **response,
	char ***resp_head,
	char **resp_body)
{
	if (edg_wll_open_proxy(ctx)) return edg_wll_Error(ctx,NULL,NULL);
	
	switch (edg_wll_http_send_proxy(ctx,request,req_head,req_body)) {
		case ENOTCONN:
			edg_wll_close_proxy(ctx);
			if (edg_wll_open_proxy(ctx)
				|| edg_wll_http_send_proxy(ctx,request,req_head,req_body))
					return edg_wll_Error(ctx,NULL,NULL);
			/* fallthrough */
		case 0: break;
		default: return edg_wll_Error(ctx,NULL,NULL);
	}

	if (edg_wll_http_recv_proxy(ctx,response,resp_head,resp_body) == ENOTCONN) {
		edg_wll_close_proxy(ctx);
		(void) (edg_wll_open_proxy(ctx)
			|| edg_wll_http_send_proxy(ctx,request,req_head,req_body)
			|| edg_wll_http_recv_proxy(ctx,response,resp_head,resp_body));
	}
	
	return edg_wll_Error(ctx,NULL,NULL);
}
