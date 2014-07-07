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


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <assert.h>

#include "glite/security/glite_gss.h"
#include "glite/lbu/escape.h"
#include "glite/lb/events.h"
#include "glite/lb/log_proto.h"
#include "glite/lb/mini_http.h"
#include "glite/lb/context-int.h"
#include "glite/lb/xml_parse.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/il_msg.h"

#include "notification.h"
#include "connection.h"

#define CON_QUEUE               10	/* listen() queue limit */

/* XXX: moving all request_headers to one file would be nice
 */
static const char* const request_headers[] = {
	"Cache-Control: no-cache",
	"Accept: application/x-dglb",
	"User-Agent: edg_wll_Api/" PROTO_VERSION "/" COMP_PROTO,
	"Content-Type: application/x-dglb",
	NULL
};

static int zero = 0;



/* Decrement timeout
 */
static int decrement_timeout(struct timeval *timeout, struct timeval before, struct timeval after)
{
	if (!timeout) 
		return(0);	// wait indefinitely
	
        (*timeout).tv_sec = (*timeout).tv_sec - (after.tv_sec - before.tv_sec);
        (*timeout).tv_usec = (*timeout).tv_usec - (after.tv_usec - before.tv_usec);
        while ( (*timeout).tv_usec < 0) {
                (*timeout).tv_sec--;
                (*timeout).tv_usec += 1000000;
        }
        if ( ((*timeout).tv_sec < 0) || (((*timeout).tv_sec == 0) && ((*timeout).tv_usec == 0)) ) return(1);
        else return(0);
}



/* Split address to name & port
 */
static void get_name_and_port(const char *address, char **name, int *port)
{
	char *n = NULL, *p;
	
	n = strdup(address);
	p = strrchr(n, ':');
	if (p)
	{
		*port = atoi(p+1);
		*p = 0;
	}
	if ((p = strrchr(n, '[')) != NULL) {
		int len = strcspn(p+1, "]");
		*name = (char*)malloc(sizeof(char*) * (len+1));
		strncpy(*name, p+1, len);
	}
	else *name = strdup(n);
	free(n);
}	


static int daemon_listen(edg_wll_Context ctx, const char *name, char *port, int *conn_out) {
	struct	addrinfo *ai;
	struct	addrinfo hints;
	int	conn;
	int 	gaie;

	memset (&hints, '\0', sizeof (hints));
	hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE | AI_ADDRCONFIG;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET6;

	gaie = getaddrinfo (name, port, &hints, &ai);
	if (gaie != 0 || ai == NULL) {
		hints.ai_family = 0;
		gaie = getaddrinfo (NULL, port, &hints, &ai);
	}

	gaie = getaddrinfo (name, port, &hints, &ai);
	if (gaie != 0) {
		return edg_wll_SetError(ctx, EADDRNOTAVAIL, gai_strerror(gaie));
	}
	if (ai == NULL) {
		return edg_wll_SetError(ctx, EADDRNOTAVAIL, "no result from getaddrinfo");
	}

	conn = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if ( conn < 0 ) { 
		freeaddrinfo(ai);
		return edg_wll_SetError(ctx, errno, "socket() failed");
	}
	//setsockopt(conn, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (ai->ai_family == AF_INET6)
		setsockopt(conn, IPPROTO_IPV6, IPV6_V6ONLY, &zero, sizeof(zero));

	if ( bind(conn, ai->ai_addr, ai->ai_addrlen) )
	{
		edg_wll_SetError(ctx, errno, "bind() failed");
		close(conn); 
		freeaddrinfo(ai);
		return edg_wll_Error(ctx, NULL, NULL);
	}

	if ( listen(conn, CON_QUEUE) ) { 
		edg_wll_SetError(ctx, errno, "listen() failed");
		close(conn); 
		return edg_wll_Error(ctx, NULL, NULL);
	}

	freeaddrinfo(ai);

	*conn_out = conn;
	return 0;
}


static int my_bind(edg_wll_Context ctx, const char *name, int port, int *fd)
{
	int		ret;
	char 		*portstr = NULL;

	asprintf(&portstr, "%d", port);
	if (portstr == NULL) {
		return edg_wll_SetError(ctx, ENOMEM, "my_bind(): ENOMEM converting port number");
	}
	ret = daemon_listen(ctx, name, portstr, fd);
	free(portstr);

	return ret;
}
	


static int set_server_name_and_port(edg_wll_Context ctx) 
{

	if (!ctx->p_notif_server)
		return(edg_wll_SetError(ctx, (edg_wll_ErrorCode) EINVAL, 
				"Hostname of server to notif is not set"));
	else {
		free(ctx->srvName);
		ctx->srvName = strdup(ctx->p_notif_server);
	}
	if (!ctx->p_notif_server_port)
		return(edg_wll_SetError(ctx, (edg_wll_ErrorCode) EINVAL, 
				"Port of server to notif is not set"));
	else ctx->srvPort = ctx->p_notif_server_port;

	return edg_wll_Error(ctx,NULL,NULL);
}



static int get_client_address(
		edg_wll_Context ctx, 
		int fd,
		const char *address_override,
		char **address) 
	
{
	struct  sockaddr_storage	a;
	socklen_t		alen = sizeof(a);
	int			e;
	char 			*name = NULL;
	int			port = 0;
	struct addrinfo *ai;
	struct addrinfo hints;
	char 		hostnum[64], hostnum1[64], portnum[16];
	
	memset (&hints, '\0', sizeof (hints));
	hints.ai_flags = AI_ADDRCONFIG;
	hints.ai_socktype = SOCK_STREAM;
	
	if (address_override) {
		// complete dest URL ==> pass the URL furhter without listening
		if (strstr(address_override, "//")) {
			// close the socket if opened
			if (ctx->notifSock >= 0) {
				close(ctx->notifSock);
				ctx->notifSock = -1;
			}
		} else {

		get_name_and_port(address_override, &name, &port);
		
		e = getaddrinfo((const char *) name, NULL, &hints, & ai);
		if (e) {
			edg_wll_SetError(ctx, EADDRNOTAVAIL, "getaddrinfo() failed");
			goto err;
		}

		e = getnameinfo ((struct sockaddr *) ai->ai_addr, ai->ai_addrlen,
                	hostnum, sizeof(hostnum), NULL, 0, NI_NUMERICHOST );
    		if (e) {
			edg_wll_SetError(ctx, EADDRNOTAVAIL, "getnameinfo() failed");
			goto err;
    		}
		freeaddrinfo(ai);

		e = getaddrinfo((const char *) hostnum, NULL, &hints, & ai);
		if (e) {
			edg_wll_SetError(ctx, EADDRNOTAVAIL, "getaddrinfo() failed");
			goto err;
		}

		/* Test whether open sockket represents the same address as address_override
		 * if not, close ctx->notifSock and bind to new socket corresponding to 
		 * address_override
		 */
		if (ctx->notifSock >= 0) {
			if (getsockname(ctx->notifSock, (struct sockaddr *) &a, &alen)) 
				return edg_wll_SetError(ctx, errno, "getsockname() failed");
			
			e = getnameinfo ((struct sockaddr *) &a, alen,
                		NULL, 0, portnum, sizeof(portnum), NI_NUMERICSERV );
    			if (e) {
				edg_wll_SetError(ctx, EADDRNOTAVAIL, "getnameinfo() failed");
				goto err;
    			}
			e = getnameinfo ((struct sockaddr *) &a, alen,
                		hostnum1, sizeof(hostnum1), NULL, 0, NI_NUMERICHOST );
    			if (e) {
				edg_wll_SetError(ctx, EADDRNOTAVAIL, "getnameinfo() failed");
				goto err;
    			}
			if ( (strcmp(hostnum1, hostnum)) || (atoi(portnum) != port) ) {
				
				if (close(ctx->notifSock)) {
					edg_wll_SetError(ctx, errno, "close() failed");
					goto err;
				}
				ctx->notifSock = -1;
				
				if (my_bind(ctx, name, port, &(ctx->notifSock))) 
					goto err;
			}
		}
		else {	// create new socket 
			if (my_bind(ctx, name, port, &(ctx->notifSock))) 
				goto err;
		}
		} // complete URL

		*address = strdup(address_override);
	}
	else {	// address_override == NULL
		
		if (fd == -1) {
			if (ctx->notifSock == -1) 	
				// create new socket
				if (my_bind(ctx, NULL, 0, &(ctx->notifSock)))
						goto err;
			// else: resue socket
		}
	
		if (getsockname(fd == -1 ? ctx->notifSock : fd,(struct sockaddr *)  &a, &alen)) 
			return edg_wll_SetError(ctx, errno, "getsockname() failed");

		e = getnameinfo ((struct sockaddr *) &a, alen,
                	hostnum, sizeof(hostnum), portnum, sizeof(portnum), NI_NUMERICSERV );
    		if (e) {
			edg_wll_SetError(ctx, EADDRNOTAVAIL, "getnameinfo() failed");
			goto err;
    		}
		asprintf(address,"%s:%s", hostnum, portnum);
	}

err:
	free(name);
	
	return edg_wll_Error(ctx,NULL,NULL);
}



int edg_wll_NotifNew(
        edg_wll_Context         ctx,
        edg_wll_QueryRec        const * const *conditions,
	int			flags,
        int                     fd,
        const char              *address_override,
        edg_wll_NotifId         *id_out,
        time_t                  *valid) 
{
	edg_wll_NotifId		notifId = NULL;
	char			*address = NULL, *send_mess = NULL,
				*recv_mess = NULL, *response = NULL;
	int			ret;	
	

	edg_wll_ResetError(ctx);

	if ( (ret = set_server_name_and_port(ctx)) ) 
		goto err;
		
	if ( (ret = edg_wll_NotifIdCreate(ctx->srvName,ctx->srvPort,&notifId)) )
		goto err;

	if ( (ret = get_client_address(ctx, fd, address_override, &address)) )
		goto err;
	
	if ( (ret = edg_wll_NotifRequestToXML(ctx, "New", notifId, address, 
			EDG_WLL_NOTIF_NOOP, *valid, conditions, flags, &send_mess)) )
		goto err;

	ctx->p_tmp_timeout = ctx->p_notif_timeout;
	
	if ( (ret = edg_wll_http_send_recv(ctx, "POST /notifRequest HTTP/1.1",
			request_headers,send_mess,
			&response,NULL,&recv_mess)) )
		goto err;
	
	if ( (ret = http_check_status(ctx,response)) )
		goto err;

	ret = edg_wll_ParseNotifResult(ctx, recv_mess, valid);


err:
	if (ret != 0) {
		if (notifId) edg_wll_NotifIdFree(notifId);
		*id_out = NULL;
		*valid = -1;
	}
	else
		*id_out = notifId;
		
	free(address);
	free(recv_mess);
	free(send_mess);
	free(response);
	return edg_wll_Error(ctx,NULL,NULL);
}



int edg_wll_NotifBind(
        edg_wll_Context         ctx,
        const edg_wll_NotifId   id,
        int                     fd,
        const char              *address_override,
        time_t                  *valid)
{
	char	*address = NULL, *send_mess = NULL,
       		*recv_mess = NULL, *response = NULL;
	
	
	edg_wll_ResetError(ctx);

	
/* XXX - ??? this should not be there - if fd = -1 then semantic is 'create new socket or _reuse_ old'
	// if a local listening socket active, close it
	if (ctx->notifSock >= 0) {
		if (close(ctx->notifSock)) 
			return edg_wll_SetError(ctx, errno, "close() failed");
		else
			ctx->notifSock = -1;
	}
*/

	if (set_server_name_and_port(ctx)) 
		goto err;
		
	if (get_client_address(ctx, fd, address_override, &address))
		goto err;

	if (edg_wll_NotifRequestToXML(ctx, "Bind", id, address, 
			EDG_WLL_NOTIF_NOOP, *valid, NULL, 0, &send_mess)) 
		goto err;

	ctx->p_tmp_timeout = ctx->p_notif_timeout;
	
	if (edg_wll_http_send_recv(ctx, "POST /notifRequest HTTP/1.1",
			request_headers,send_mess,
			&response,NULL,&recv_mess)) 
		goto err;
	
	if (http_check_status(ctx,response))
		goto err;

	edg_wll_ParseNotifResult(ctx, recv_mess, valid);

err:
	free(address);
	free(recv_mess);
	free(send_mess);
	free(response);
	return edg_wll_Error(ctx,NULL,NULL);
}



int edg_wll_NotifChange(
        edg_wll_Context         ctx,
        const edg_wll_NotifId   id,
        edg_wll_QueryRec        const * const * conditions,
        edg_wll_NotifChangeOp   op)
{
	char	*send_mess = NULL, *recv_mess = NULL, *response = NULL;
	time_t 	not_used;
	
	
	edg_wll_ResetError(ctx);

	
	if (set_server_name_and_port(ctx)) 
		goto err;
		
	if (edg_wll_NotifRequestToXML(ctx, "Change", id, NULL, 
			op, -1, conditions, 0, &send_mess)) 
		goto err;

	ctx->p_tmp_timeout = ctx->p_notif_timeout;
	
	if (edg_wll_http_send_recv(ctx, "POST /notifRequest HTTP/1.1",
			request_headers,send_mess,
			&response,NULL,&recv_mess)) 
		goto err;
	
	if (http_check_status(ctx,response))
		goto err;

	edg_wll_ParseNotifResult(ctx, recv_mess, &not_used);

err:
	free(recv_mess);
	free(send_mess);
	free(response);
	return edg_wll_Error(ctx,NULL,NULL);
}



int edg_wll_NotifRefresh(
        edg_wll_Context         ctx,
        const edg_wll_NotifId   id,
        time_t                  *valid)
{
	char	*send_mess = NULL, *recv_mess = NULL, *response = NULL;
	
	
	edg_wll_ResetError(ctx);

	
	if (set_server_name_and_port(ctx)) 
		goto err;
		
	if (edg_wll_NotifRequestToXML(ctx, "Refresh", id, NULL, 
			EDG_WLL_NOTIF_NOOP, *valid, NULL, 0, &send_mess)) 
		goto err;

	ctx->p_tmp_timeout = ctx->p_notif_timeout;
	
	if (edg_wll_http_send_recv(ctx, "POST /notifRequest HTTP/1.1",
			request_headers,send_mess,
			&response,NULL,&recv_mess)) 
		goto err;
	
	if (http_check_status(ctx,response))
		goto err;

	edg_wll_ParseNotifResult(ctx, recv_mess, valid);

err:
	free(recv_mess);
	free(send_mess);
	free(response);
	return edg_wll_Error(ctx,NULL,NULL);
}



int edg_wll_NotifDrop(
        edg_wll_Context         ctx,
        edg_wll_NotifId         id)
{
	char	*send_mess = NULL, *recv_mess = NULL, *response = NULL;
	time_t 	not_used;
	
	
	edg_wll_ResetError(ctx);

	
	if (set_server_name_and_port(ctx)) 
		goto err;
		
	if (edg_wll_NotifRequestToXML(ctx, "Drop", id, NULL, 
			EDG_WLL_NOTIF_NOOP, -1, NULL, 0, &send_mess)) 
		goto err;

	ctx->p_tmp_timeout = ctx->p_notif_timeout;
	
	if (edg_wll_http_send_recv(ctx, "POST /notifRequest HTTP/1.1",
			request_headers,send_mess,
			&response,NULL,&recv_mess)) 
		goto err;
	
	if (http_check_status(ctx,response))
		goto err;

	edg_wll_ParseNotifResult(ctx, recv_mess, &not_used);

err:
	free(recv_mess);
	free(send_mess);
	free(response);
	return edg_wll_Error(ctx,NULL,NULL);
}


/* read all data available on active notif connection */
static int read_data(edg_wll_Context ctx)
{
	edg_wll_GssStatus	gss_code;
	int			ret;
	edg_wll_ConnPool	*notif = &(ctx->connNotif->connPool[ctx->connNotif->connToUse]);


	do {
		if (notif->bufUse == notif->bufSize) {
			notif->bufSize += NOTIF_POOL_BUFFER_BLOCK_SIZE;
			notif->buf = realloc(notif->buf, notif->bufSize);
			if (!notif->buf) 
				return edg_wll_SetError(ctx, ENOMEM, "read_data()");
		}

		ret = edg_wll_gss_read(&(notif->gss), notif->buf + notif->bufUse, 
			notif->bufSize - notif->bufUse, &ctx->p_tmp_timeout, &gss_code);

		if (ret < 0) {
			switch(ret) {
				case EDG_WLL_GSS_ERROR_TIMEOUT:
					edg_wll_SetError(ctx, ETIMEDOUT, "read message");
					break;
				case EDG_WLL_GSS_ERROR_EOF:
					edg_wll_SetError(ctx, ENOTCONN, NULL);
					break;
				default:
					edg_wll_SetError(ctx, EDG_WLL_ERROR_GSS, "read message");
					break;
			}
			goto err;
		}	
		notif->bufUse += ret;

	} while (notif->bufSize == notif->bufUse); 

err:
	return edg_wll_Error(ctx, NULL, NULL);
}


/* copies bytes_to_copy chars from pool buffer to given _allocated_ working buffer
 */
static int buffer_reader(void *user_data, char *buffer, const int bytes_to_copy)
{
	edg_wll_Context tmp_ctx = (edg_wll_Context)user_data;


	if (tmp_ctx->connNotif->connPool[tmp_ctx->connNotif->connToUse].bufUse < bytes_to_copy) 
		return(-1);

	memcpy(buffer,tmp_ctx->connNotif->connPool[tmp_ctx->connNotif->connToUse].buf + 
		tmp_ctx->connNotif->connPool[tmp_ctx->connNotif->connToUse].bufPtr, bytes_to_copy);
	tmp_ctx->connNotif->connPool[tmp_ctx->connNotif->connToUse].bufPtr += bytes_to_copy;

	return(bytes_to_copy);
}


static int recv_notif(edg_wll_Context ctx, char **message)
{
	int 	len;
	

	len = read_il_data(ctx, message, buffer_reader);
	if(len < 0)
	  return(len);
	
	return edg_wll_Error(ctx,NULL,NULL);
}


static int prepare_reply(const edg_wll_Context ctx)
{
	int	len, err_code, err_code_min = 0;
	char	*err_msg = NULL;

	
	err_code = edg_wll_Error(ctx,NULL,&err_msg);

	if (!err_msg) err_msg=strdup("OK");
	
	len = encode_il_reply(&(ctx->connNotif->connPool[ctx->connNotif->connToUse].bufOut),
		 err_code, err_code_min, err_msg);

	if(len < 0) {
		edg_wll_SetError(ctx,E2BIG,"create_reply()");
		goto err;
	}

	ctx->connNotif->connPool[ctx->connNotif->connToUse].bufSizeOut = len;
err:
	free(err_msg);
	return edg_wll_Error(ctx,NULL,NULL);
}


// send pool output buffer nonblockingly
// XXX: missing partial-sent logic (gss_write does not return written bytes count)
static int send_reply(const edg_wll_Context ctx)
{
	int	ret;
	edg_wll_GssStatus	gss_code;


	ret = edg_wll_gss_write(&ctx->connNotif->connPool[ctx->connNotif->connToUse].gss,
		ctx->connNotif->connPool[ctx->connNotif->connToUse].bufOut, 
		ctx->connNotif->connPool[ctx->connNotif->connToUse].bufSizeOut, &ctx->p_tmp_timeout, &gss_code);

	if (ret < 0) {
		// edg_wll_gss_write does not return how many bytes were written, so
		// any error is fatal for us and we can close connection
		edg_wll_SetError(ctx,
				ret == EDG_WLL_GSS_ERROR_TIMEOUT ? 
					ETIMEDOUT : EDG_WLL_ERROR_GSS,
					"send_reply()");
		goto err;
	}
	
err:
	return edg_wll_Error(ctx,NULL,NULL);
}




int edg_wll_NotifReceive(
        edg_wll_Context         ctx,
        int                     fd,
        const struct timeval    *timeout,
        edg_wll_JobStat         *state_out,
        edg_wll_NotifId         *id_out)

/* NotifReceive */
{
	int 			i, j, ret, fd_num = ctx->connNotif->connOpened + 1;
	struct _fd_map {
		struct pollfd	pollfds[fd_num];
		int		index[fd_num];
	}			fd_map;
	edg_wll_Event 		*event = NULL;
	struct timeval 		start_time,check_time;
	char 			*event_char = NULL, *jobstat_char = NULL, *message = NULL;
	struct timeval		tv = *timeout;
	

	edg_wll_ResetError(ctx);

	/* start timer */
	gettimeofday(&start_time,0);

	if (fd == -1) {
		if (ctx->notifSock == -1) {
			return edg_wll_SetError(ctx, EINVAL, "No client socket opened.");
		}
		else {
			fd = ctx->notifSock;
		}
	}

start:
	fd_num = ctx->connNotif->connOpened + 1;
	fd_map.pollfds[0].fd = fd;
	fd_map.pollfds[0].events = POLLIN;
	fd_map.index[0] = -1;
	
	j = 1;
	for (i=0; i < ctx->connNotif->poolSize; i++) {
		if (ctx->connNotif->connPool[i].gss.sock != -1) {
			fd_map.pollfds[j].fd = ctx->connNotif->connPool[i].gss.sock;
			fd_map.pollfds[j].events = POLLIN;
			fd_map.index[j] = i;
			j++;
		}
	}
	assert(j == fd_num);
	
	/* XXX	 notif_send() & notif_receive() should then migrate to		*/
	/*	 client/connection.c and use connPool management f-cions	*/

	/* XXX: long-lived contexts may have problems, TODO implement credential reload */

	switch(poll(fd_map.pollfds, fd_num, tv.tv_sec*1000+tv.tv_usec/1000)) {
		case -1:
			edg_wll_SetError(ctx, errno, "edg_wll_NotifReceive: poll() failed");
			goto err;
		case 0:
			edg_wll_SetError(ctx, ETIMEDOUT, "edg_wll_NotifReceive: poll() timed out");
			goto err;
		default:
			for (i=0; i < fd_num; i++) {
				if ((fd_map.pollfds[i].revents & POLLIN)) {
					break;
				}
			}
			if (fd_num == i) { 
				/* no events on any socket */
				edg_wll_SetError(ctx, errno, "edg_wll_NotifReceive: error on filedescriptor");
				goto err;
			}
			break;
	}

	/* check time */
	gettimeofday(&check_time,0);
	if (decrement_timeout(&tv, start_time, check_time)) {
		edg_wll_SetError(ctx, ETIMEDOUT, "edg_wll_NotifReceive()");
		goto err;
	}
	ctx->p_tmp_timeout = tv;

	// there is some incomming connection(s)
	// XXX: what has higher priority? new connection, or data on active connection ?

	if (fd_map.pollfds[0].revents & POLLIN) {	/* new connection */
		start_time = check_time;
		
		if (edg_wll_accept(ctx,fd)) goto err;
	
		/* check time */
		gettimeofday(&check_time,0);
		if (decrement_timeout(&tv, start_time, check_time)) {
			edg_wll_SetError(ctx, ETIMEDOUT, "edg_wll_NotifReceive()");
			goto err;
		}
		ctx->p_tmp_timeout = tv;
	}
	else {	/* data on some of active (NotifPool) connections */
		// XXX: if data arrives for more connections, which one serve first?
		//      poor-man solution - take first one on which data arrived
		for (i=1; i < fd_num; i++) {
			if (fd_map.pollfds[i].revents & POLLIN) { 
				ctx->connNotif->connToUse = fd_map.index[i];
				break;
			}
		}
		assert(i < fd_num);
	}
	
	/****************************************************************/
	/* Communication with notif-interlogger				*/
	/****************************************************************/
	
	start_time = check_time;

	if ( (ret = read_data(ctx)) ) {
		ctx->connNotif->connPool[ctx->connNotif->connToUse].bufPtr = 0;

		if ( ret == ENOTCONN ) {
			int	broken = (ctx->connNotif->connPool[ctx->connNotif->connToUse].bufUse != 0);
			/* IL closed connection; remove this connection from pool and go to poll if timeout > 0 */

			CloseConnectionNotif(ctx);

			/* check time */
			gettimeofday(&check_time,0);
			if (decrement_timeout(&tv, start_time, check_time)) {
				edg_wll_SetError(ctx, ETIMEDOUT, "edg_wll_NotifReceive()");
				goto err;
			}
			ctx->p_tmp_timeout = tv;
			start_time = check_time;

			if (broken) { 
				edg_wll_SetError(ctx,ENOTCONN,"IL connection broken in middle of message");
				goto err;
			}

			edg_wll_ResetError(ctx);
			goto start;
		}

		goto err;
	}

	/* check time */
	gettimeofday(&check_time,0);
	if (decrement_timeout(&tv, start_time, check_time)) {
		edg_wll_SetError(ctx, ETIMEDOUT, "edg_wll_NotifReceive()");
		goto err;
	}
	ctx->p_tmp_timeout = tv;
	start_time = check_time;

	if (recv_notif(ctx, &message) < 0) {
		ctx->connNotif->connPool[ctx->connNotif->connToUse].bufPtr = 0;
		goto err;
	}

	/* receive message complete, free input buffer */
	free(ctx->connNotif->connPool[ctx->connNotif->connToUse].buf);
	ctx->connNotif->connPool[ctx->connNotif->connToUse].buf = NULL;
	ctx->connNotif->connPool[ctx->connNotif->connToUse].bufPtr = 0;
	ctx->connNotif->connPool[ctx->connNotif->connToUse].bufUse = 0;
	ctx->connNotif->connPool[ctx->connNotif->connToUse].bufSize = 0;
	

	if (prepare_reply(ctx) || send_reply(ctx)) { 
		// fatal error
		// XXX: output buffer not used between edg_wll_NotifReceive() calls
		CloseConnectionNotif(ctx);	
		goto err;	
	}

	/* check time */
	gettimeofday(&check_time,0);
	if (decrement_timeout(&tv, start_time, check_time)) {
		edg_wll_SetError(ctx, ETIMEDOUT, "edg_wll_NotifReceive()");
		goto err;
	}
	ctx->p_tmp_timeout = tv;
	start_time = check_time;

	/* response sent, free output buffer */
	free(ctx->connNotif->connPool[ctx->connNotif->connToUse].bufOut);
	ctx->connNotif->connPool[ctx->connNotif->connToUse].bufOut = NULL;
	ctx->connNotif->connPool[ctx->connNotif->connToUse].bufUseOut = 0;
	ctx->connNotif->connPool[ctx->connNotif->connToUse].bufSizeOut = 0;
	ctx->connNotif->connPool[ctx->connNotif->connToUse].bufPtrOut = 0;

	{
		il_octet_string_t ev;

                if ( decode_il_msg(&ev, message) < 0 ) {
			CloseConnectionNotif(ctx);
			edg_wll_SetError(ctx, EDG_WLL_IL_PROTO, "decoding event string");
			goto err;
		}
		event_char = ev.data;
	}
	/****************************************************************/
	/* end of  notif-interlogger message exchange			*/
	/****************************************************************/
	
	/* check time */
	gettimeofday(&check_time,0);
	if (decrement_timeout(&tv, start_time, check_time)) {
		edg_wll_SetError(ctx, ETIMEDOUT, "edg_wll_NotifReceive()");
		goto err;
	}
	ctx->p_tmp_timeout = tv;
	start_time = check_time;
		
	if (edg_wll_ParseNotifEvent(ctx, event_char, &event)) {
		goto err;
	}

	jobstat_char = glite_lbu_UnescapeXML((const char *) event->notification.jobstat);
	if (jobstat_char == NULL) {
		edg_wll_SetError(ctx, EINVAL, "glite_lbu_UnescapeXML()");
		goto err;
	}
	
	/* fill in return values
	 */
	if ( edg_wll_ParseJobStat(ctx, jobstat_char, 
				strlen(jobstat_char), state_out)) {
		goto err;
	}

	if (id_out) { 
		*id_out = event->notification.notifId;
		event->notification.notifId = NULL;
	}
	
err:
	if (event) { 
		edg_wll_FreeEvent(event);
		// XXX - konzultovat s honikem; podle meho by to free 
		// mel delat uz edg_wll_FreeEvent
		free(event);
	}

	free(event_char);
	free(jobstat_char);
	free(message);
	
	// XXX: decrement or not to decrement :)
	//*timeout = tv;

	return edg_wll_Error(ctx,NULL,NULL);
}


int edg_wll_NotifGetFd(
        edg_wll_Context         ctx)
{
	if (ctx->notifSock == -1) {
		edg_wll_SetError(ctx, EBADF, "Client socket is not opened.");
		return -1;
	}
	
	return ctx->notifSock;
}


int edg_wll_NotifCloseFd(
        edg_wll_Context         ctx)
{
	int err;
	
	if (ctx->notifSock >= 0) {
		err = close(ctx->notifSock);
		ctx->notifSock = -1;
		
		if (err) 
			return edg_wll_SetError(ctx, errno, "close() failed");
	}

	return edg_wll_Error(ctx,NULL,NULL);
}

int edg_wll_NotifClosePool(
        edg_wll_Context         ctx)
{
	if (ctx->connNotif->connOpened) {
		for (ctx->connNotif->connToUse=0; ctx->connNotif->connToUse < ctx->connNotif->poolSize; 
				ctx->connNotif->connToUse++) {
			if (ctx->connNotif->connPool[ctx->connNotif->connToUse].gss.sock != -1) CloseConnectionNotif(ctx);
			
		}
	}

	return edg_wll_Error(ctx,NULL,NULL);
}
