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


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>
#include <time.h>

#include "glite/jobid/cjobid.h"
#include "glite/security/glite_gss.h"
#include "glite/lb/context-int.h"
#include "glite/lb/mini_http.h"
#include "glite/lb/connpool.h"

#include "consumer.h"
#include "connection.h"


int CloseConnection(edg_wll_Context ctx, int conn_index)
{
	/* close connection and free its structures */
	int cIndex,ret = 0;

        cIndex = conn_index;

	assert(ctx->connections->connOpened);

	if (ctx->connections->connPool[cIndex].gss.sock >= 0)
		ret = edg_wll_gss_close(&ctx->connections->connPool[cIndex].gss, &ctx->p_tmp_timeout);
	if (ctx->connections->connPool[cIndex].gsiCred != NULL) 
		edg_wll_gss_release_cred(&ctx->connections->connPool[cIndex].gsiCred, NULL);
	free(ctx->connections->connPool[cIndex].peerName);
	free(ctx->connections->connPool[cIndex].buf);
	free(ctx->connections->connPool[cIndex].certfile);
	
	memset(ctx->connections->connPool + cIndex, 0, sizeof(edg_wll_ConnPool));
	ctx->connections->connPool[cIndex].gss.sock = -1;
	
	ctx->connections->connOpened--;

	return ret;
}


int CloseConnectionNotif(edg_wll_Context ctx)
{
	/* close connection and free its structures */
	int cIndex,ret = 0;


        cIndex = ctx->connNotif->connToUse;

	assert(ctx->connNotif->connOpened);

	if (ctx->connNotif->connPool[cIndex].gss.sock >= 0)
		edg_wll_gss_close(&ctx->connNotif->connPool[cIndex].gss, &ctx->p_tmp_timeout); // always returns 0
	if (ctx->connNotif->connPool[cIndex].gsiCred != NULL) 
		if ( (ret = edg_wll_gss_release_cred(&ctx->connNotif->connPool[cIndex].gsiCred, NULL)) )
			edg_wll_SetError(ctx,ret,"error in edg_wll_gss_release_cred()");	
	free(ctx->connNotif->connPool[cIndex].peerName);
	free(ctx->connNotif->connPool[cIndex].buf);
	free(ctx->connNotif->connPool[cIndex].bufOut);
	free(ctx->connNotif->connPool[cIndex].certfile);
	
	memset(ctx->connNotif->connPool + cIndex, 0, sizeof(edg_wll_ConnPool));
	ctx->connNotif->connPool[cIndex].gss.sock = -1;
	
	ctx->connNotif->connOpened--;

	return edg_wll_Error(ctx,NULL,NULL);
}



int ConnectionIndex(edg_wll_Context ctx, const char *name, int port)
{
	int i;
	struct stat statinfo;
	int using_certfile = 0;

	if (ctx->p_proxy_filename || ctx->p_cert_filename) {
		stat(ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename, &statinfo);
		using_certfile = 1;
	}

        for (i=0; i<ctx->connections->poolSize;i++) { 
//		printf("*** Testing connection %d: peerName = %s, peerPort = %d, file = %s\n", i, ctx->connections->connPool[i].peerName != NULL ? ctx->connections->connPool[i].peerName : "NULL", ctx->connections->connPool[i].peerPort, ctx->connections->connPool[i].file);
		if ((ctx->connections->connPool[i].peerName != NULL) &&		// Conn Pool record must exist
                    !strcmp(name, ctx->connections->connPool[i].peerName) &&	// Server names must be equal
		   (port == ctx->connections->connPool[i].peerPort) && 		// Ports must be equal
			(!using_certfile ||					// we are either using the default cert file
				((ctx->connections->connPool[i].certfile) &&	// or checking which file
				 (ctx->connections->connPool[i].certfile->st_ino == statinfo.st_ino) &&	 // this conn
				 (ctx->connections->connPool[i].certfile->st_dev == statinfo.st_dev)))) { // uses to auth.


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


int AddConnection(edg_wll_Context ctx, char *name, int port)
{
	int i,index = -1;

        for (i = 0; i < ctx->connections->poolSize; i++) {
            if (ctx->connections->connPool[i].peerName == NULL) {
                if (!edg_wll_connectionTryLock(ctx, i)) {
		    index = i;	// This connection is free and was not locked. We may lock it and use it.
		    break;
                }
	    }
	}

	if (index < 0) return -1;

	free(ctx->connections->connPool[index].peerName);	// should be empty; just to be sure
	ctx->connections->connPool[index].peerName = strdup(name);
	ctx->connections->connPool[index].peerPort = port;
	ctx->connections->connPool[index].gsiCred = NULL; // initial value
	ctx->connections->connPool[index].certfile = NULL;
	ctx->connections->connOpened++;

	return index;
}


int SetFreeConnectionIndexNotif(edg_wll_Context ctx)
{
	int i;


	ctx->connNotif->connToUse = -1;

        for (i = 0; i < ctx->connNotif->poolSize; i++) 
        	if (ctx->connNotif->connPool[i].gss.sock == -1) {
			ctx->connNotif->connToUse = i;
			ctx->connNotif->connOpened++;
			assert(!ctx->connNotif->connPool[i].buf);
			break;
		}

	return ctx->connNotif->connToUse;
}



int ReleaseConnection(edg_wll_Context ctx, char *name, int port)
{
	int i, index = 0, foundConnToDrop = 0;
	long min;


	edg_wll_ResetError(ctx);
	if (ctx->connections->connOpened == 0) return 0;	/* nothing to release */
	
	if (name) {
		if ((index = ConnectionIndex(ctx, name, port)) >= 0)
			CloseConnection(ctx, index);
	}
	else {					/* free the oldest (unlocked) connection */
		for (i=0; i<ctx->connections->poolSize; i++) {
			assert(ctx->connections->connPool[i].peerName); // Full pool expected - accept non-NULL values only
			if (!edg_wll_connectionTryLock(ctx, i)) {
				edg_wll_connectionUnlock(ctx, i);	// Connection unlocked. Consider releasing it
				if (foundConnToDrop) {		// This is not the first unlocked connection
					if (ctx->connections->connPool[i].lastUsed.tv_sec < min) {
						min = ctx->connections->connPool[i].lastUsed.tv_sec;
						index = i;
						foundConnToDrop++;
					}
				}
				else {	// This is the first unlocked connection we have found.
					foundConnToDrop++;
					index = i;
					min = ctx->connections->connPool[i].lastUsed.tv_sec;
				}
			}
		}
		if (!foundConnToDrop) return edg_wll_SetError(ctx,EAGAIN,"all connections in the connection pool are locked");
		CloseConnection(ctx, index);
	}
	return edg_wll_Error(ctx,NULL,NULL);
}
			

int ReleaseConnectionNotif(edg_wll_Context ctx)
{
	int i, index = 0;
	long min;


	edg_wll_ResetError(ctx);
	if (ctx->connNotif->connOpened == 0) return 0;	/* nothing to release */
	
	min = ctx->connNotif->connPool[0].lastUsed.tv_sec;

	/* free the oldest (unlocked) connection */
	for (i=0; i<ctx->connNotif->poolSize; i++) {
		assert(ctx->connNotif->connPool[i].gss.sock > -1); // Full pool expected - accept non-NULL values only

		if (ctx->connections->connPool[i].lastUsed.tv_sec < min) {
			min = ctx->connections->connPool[i].lastUsed.tv_sec;
			index = i;
		}
	}	

	ctx->connNotif->connToUse = index;
	CloseConnectionNotif(ctx);

	return edg_wll_Error(ctx,NULL,NULL);
}
			


int edg_wll_close(edg_wll_Context ctx, int* connToUse)
{
	edg_wll_ResetError(ctx);
	if (*connToUse == -1) return 0;

	CloseConnection(ctx, *connToUse);

        edg_wll_connectionUnlock(ctx, *connToUse); /* Forgetting the conn. Unlocking is safe. */
		
	*connToUse = -1;
	return edg_wll_Error(ctx,NULL,NULL);
}



int edg_wll_close_proxy(edg_wll_Context ctx)
{
	edg_wll_plain_close(&ctx->connProxy->conn);

	return edg_wll_ResetError(ctx);
}



int edg_wll_open(edg_wll_Context ctx, int* connToUse)
{
	int index;
	edg_wll_GssStatus gss_stat;
	time_t lifetime = 0;
	struct stat statinfo;
	int acquire_cred = 0;
	

	edg_wll_ResetError(ctx);

        edg_wll_poolLock(); /* We are going to search the pool, it has better be locked */

        /* July 12, 2007 - ZS - Searching the pool for srvName/srvPort is not enough.
        we also need to check the user identity so that there may be several connections
        open to the same server using different identities. */

	if ( (index = ConnectionIndex(ctx, ctx->srvName, ctx->srvPort)) == -1 ) {
		/* no such open connection in pool */
		if (ctx->connections->connOpened == ctx->connections->poolSize)
			if(ReleaseConnection(ctx, NULL, 0)) goto end;
		
		index = AddConnection(ctx, ctx->srvName, ctx->srvPort);
		if (index < 0) {
                    edg_wll_SetError(ctx,EAGAIN,"connection pool size exceeded");
		    goto end;
		}

                #ifdef EDG_WLL_CONNPOOL_DEBUG	
                    printf("Connection to %s:%d opened as No. %d in the pool\n",ctx->srvName,ctx->srvPort,index);
                #endif
		
	}
	/* else - there is cached open connection, reuse it */
        #ifdef EDG_WLL_CONNPOOL_DEBUG	
            else printf("Connection to %s:%d exists (No. %d in the pool) - reusing\n",ctx->srvName,ctx->srvPort,index);
        #endif

	*connToUse = index;

	//Lock the select connection, unlock the rest of the pool 
	edg_wll_connectionTryLock(ctx, index);
	edg_wll_poolUnlock(); 
	
	/* Old Comment: support anonymous connections, perhaps add a flag to the connPool
	 * struct specifying whether or not this connection shall be authenticated
	 * to prevent from repeated calls to edg_wll_gss_acquire_cred_gsi() */

	// In case of using a specifically given cert file, stat it and check for the need to reauthenticate
	if (ctx->p_proxy_filename || ctx->p_cert_filename) {
		if (ctx->connections->connPool[index].certfile)	{	// Has the file been stated before?
			stat(ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename, &statinfo);
			if (ctx->connections->connPool[index].certfile->st_mtime != statinfo.st_mtime)
				acquire_cred = 1;	// File has been modified. Need to acquire new creds.
		}
		else acquire_cred = 1; 
	}
		
	// Check if credentials exist. If so, check validity
	if (ctx->connections->connPool[index].gsiCred) {
		lifetime = ctx->connections->connPool[index].gsiCred->lifetime;
        	#ifdef EDG_WLL_CONNPOOL_DEBUG	
			printf ("Credential exists, lifetime: %d\n", lifetime);
		#endif
		if (!lifetime) acquire_cred = 1;	// Credentials exist and lifetime is OK. No need to authenticate.
	}
	else {
			acquire_cred = 1;	// No credentials exist so far, acquire. 
	}


	if (acquire_cred) {
		edg_wll_GssCred newcred = NULL;
		if (edg_wll_gss_acquire_cred_gsi(
	        	ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename,
		       ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_key_filename,
		       &newcred, &gss_stat)) {
		    edg_wll_SetErrorGss(ctx, "failed to load GSI credentials", &gss_stat);
		    goto err;
		} else {
			if (ctx->connections->connPool[index].gsiCred != NULL)
      				edg_wll_gss_release_cred(&ctx->connections->connPool[index].gsiCred,&gss_stat);
		       	ctx->connections->connPool[index].gsiCred = newcred;
			newcred = NULL;

			// Credentials Acquired successfully. Storing file identification.
        		#ifdef EDG_WLL_CONNPOOL_DEBUG	
				printf("Cert file: %s\n", ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename);
			#endif

			if (ctx->p_proxy_filename || ctx->p_cert_filename) {
				if (!ctx->connections->connPool[index].certfile) // Allocate space for certfile stats
					ctx->connections->connPool[index].certfile = 
						(struct stat*)calloc(1, sizeof(struct stat));
				stat(ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename, ctx->connections->connPool[index].certfile);
			}
		}
	}

	if (acquire_cred && ctx->connections->connPool[index].gss.context != NULL) {
		edg_wll_gss_close(&ctx->connections->connPool[index].gss, &ctx->p_tmp_timeout);
	}
	if (ctx->connections->connPool[index].gss.context == NULL) {	
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
	if (index >= 0) {
		CloseConnection(ctx, index);
		edg_wll_connectionUnlock(ctx, index);
	}
	*connToUse = -1;
ok:	

        if (*connToUse>-1) edg_wll_connectionTryLock(ctx, *connToUse); /* Just to be sure we have not forgotten to lock it */

end:

//	edg_wll_poolUnlock(); /* One way or the other, there are no more pool-wide operations */
//	ZS, 2 Feb 2010: Overall pool lock replaced with a connection-specific 
//	lock for the most part

//	xxxxx

	return edg_wll_Error(ctx,NULL,NULL);
}



int edg_wll_open_proxy(edg_wll_Context ctx)
{
	struct sockaddr_un	saddr;
	int			flags;
	int	err;
	char	*ed = NULL;
	int	retries = 0;

	edg_wll_ResetError(ctx);

	edg_wll_ResetError(ctx);

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

	while ((err = connect(ctx->connProxy->conn.sock, (struct sockaddr *)&saddr, sizeof(saddr))) < 0 &&
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

	/* printf("retries %d\n",retries); */

	if (err) {
		if (errno == EAGAIN) edg_wll_SetError(ctx,ETIMEDOUT, "edg_wll_open_proxy()");
		else edg_wll_SetError(ctx, errno, "connect()");
		goto err;
	}

	return 0;
	
err:
	/* some error occured; close created connection */

	err = edg_wll_Error(ctx,NULL,&ed);
	edg_wll_close_proxy(ctx);
	edg_wll_SetError(ctx,err,ed);
	free(ed);
		
	return err;
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
		case HTTP_ACCEPTED:
			edg_wll_SetError(ctx,EDG_WLL_ERROR_ACCEPTED_OK,response+len);
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
 
        edg_wll_connectionUnlock(ctx, connToUse);

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
	int	err;
	char	*et = NULL;

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

	/* XXX: workaround for bug #25153, don't keep proxy connection at all
	 * May have slight performance impact, it would be nice to cover proxy
	 * connections in the pool too.
	 */

	err = edg_wll_Error(ctx,NULL,&et);
	edg_wll_close_proxy(ctx);
	if (err) {
		edg_wll_SetError(ctx,err,et);
		free(et);
	}
	
	return edg_wll_Error(ctx,NULL,NULL);
}


int edg_wll_accept(edg_wll_Context ctx, int fd)
{
	int recv_sock;
	edg_wll_GssStatus gss_stat;
	time_t lifetime = 0;
	struct stat statinfo;
	int acquire_cred = 0;
        struct sockaddr_storage	a;
        socklen_t		alen;
	edg_wll_GssStatus	gss_code;

	
	edg_wll_ResetError(ctx);
	assert(fd > 0);

	alen=sizeof(a);
	recv_sock = accept(fd,(struct sockaddr *)&a,&alen);
	if (recv_sock <0) {
		edg_wll_SetError(ctx, errno, "accept() failed");
		goto err;
	}

	if (ctx->connNotif->connOpened == ctx->connNotif->poolSize)	
		if (ReleaseConnectionNotif(ctx)) goto err;
	
	if (SetFreeConnectionIndexNotif(ctx) < 0) {
		edg_wll_SetError(ctx,EAGAIN,"connection pool size exceeded");
		goto err; 
	}

        #ifdef EDG_WLL_CONNPOOL_DEBUG	
        	printf("Connection with fd %d accepted. %d in the pool\n",>srvName,ctx->srvPort,ctx->connNotif->connToUse);
        #endif
		

	// In case of using a specifically given cert file, stat it and check for the need to reauthenticate
	if (ctx->p_proxy_filename || ctx->p_cert_filename) {
		if (ctx->connNotif->connPool[ctx->connNotif->connToUse].certfile)	{	// Has the file been stated before?
			stat(ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename, &statinfo);
			if (ctx->connNotif->connPool[ctx->connNotif->connToUse].certfile->st_mtime != statinfo.st_mtime)
				acquire_cred = 1;	// File has been modified. Need to acquire new creds.
		}
		else acquire_cred = 1; 
	}
		
	// Check if credentials exist. If so, check validity
	if (ctx->connNotif->connPool[ctx->connNotif->connToUse].gsiCred) {
		lifetime = ctx->connNotif->connPool[ctx->connNotif->connToUse].gsiCred->lifetime;
        	#ifdef EDG_WLL_CONNPOOL_DEBUG	
			printf ("Credential exists, lifetime: %d\n", lifetime);
		#endif
		if (!lifetime) acquire_cred = 1;	// Credentials exist and lifetime is OK. No need to authenticate.
	}
	else {
			acquire_cred = 1;	// No credentials exist so far, acquire. 
	}


	if (acquire_cred) {
		edg_wll_GssCred newcred = NULL;
		if (edg_wll_gss_acquire_cred_gsi(
	        	ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename,
		       ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_key_filename,
		       &newcred, &gss_stat)) {
		    edg_wll_SetErrorGss(ctx, "failed to load GSI credentials", &gss_stat);
		    goto err;
		} else {
			if (ctx->connNotif->connPool[ctx->connNotif->connToUse].gsiCred != NULL)
      				edg_wll_gss_release_cred(&ctx->connNotif->connPool[ctx->connNotif->connToUse].gsiCred,&gss_stat);
		       	ctx->connNotif->connPool[ctx->connNotif->connToUse].gsiCred = newcred;
			newcred = NULL;

			// Credentials Acquired successfully. Storing file identification.
        		#ifdef EDG_WLL_CONNPOOL_DEBUG	
				printf("Cert file: %s\n", ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename);
			#endif

			if (ctx->p_proxy_filename || ctx->p_cert_filename) {
				if (!ctx->connNotif->connPool[ctx->connNotif->connToUse].certfile) // Allocate space for certfile stats
					ctx->connNotif->connPool[ctx->connNotif->connToUse].certfile = 
						(struct stat*)calloc(1, sizeof(struct stat));
				stat(ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename, ctx->connNotif->connPool[ctx->connNotif->connToUse].certfile);
			}
		}
	}

	assert(ctx->connNotif->connPool[ctx->connNotif->connToUse].gss.context == NULL);

	switch ( edg_wll_gss_accept(ctx->connNotif->connPool[ctx->connNotif->connToUse].gsiCred, recv_sock,
			&ctx->p_tmp_timeout, &ctx->connNotif->connPool[ctx->connNotif->connToUse].gss,&gss_code)) {

		case EDG_WLL_GSS_OK:
			break;
		case EDG_WLL_GSS_ERROR_ERRNO:
			edg_wll_SetError(ctx,errno,"failed to receive notification");
			goto err;
		case EDG_WLL_GSS_ERROR_GSS:
			edg_wll_SetErrorGss(ctx, "failed to authenticate sender", &gss_code);
			goto err;
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
			edg_wll_SetError(ctx,ECONNREFUSED,"sender closed the connection");
			goto err;
		case EDG_WLL_GSS_ERROR_TIMEOUT:
			edg_wll_SetError(ctx,ETIMEDOUT,"accepting notification");
			goto err;
		default:
			edg_wll_SetError(ctx, ENOTCONN, "failed to accept notification");
			goto err;
	}

	return edg_wll_Error(ctx,NULL,NULL);


err:
	/* some error occured; close created connection
	 * and free all fields in connPool[ctx->connNotif->connToUse] */
	if (ctx->connNotif->connToUse >= 0) {
		CloseConnectionNotif(ctx);
	}


	return edg_wll_Error(ctx,NULL,NULL);
}

