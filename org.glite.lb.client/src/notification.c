#ident "$Header"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#include "glite/lb/notification.h"
#include "glite/lb/events.h"
#include "glite/lb/log_proto.h"
#include "glite/lb/mini_http.h"
#include "glite/lb/context-int.h"
#include "glite/lb/xml_parse.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/il_string.h"
#include "glite/lb/escape.h"

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
	p = strchr(n, ':');
	if (p)
	{
		*port = atoi(p+1);
		*p = 0;
	}
	*name = strdup(n);
	free(n);
}	


static int my_bind(edg_wll_Context ctx, const char *name, int port, int *fd)
{
	struct  sockaddr_in     a;
	socklen_t               alen = sizeof(a);
	int			sock;
		
	sock = socket(PF_INET,SOCK_STREAM,0);
	if (sock<0) 
		return  edg_wll_SetError(ctx, errno, "socket() failed");

	a.sin_family = AF_INET;
	a.sin_port = htons(port);
	a.sin_addr.s_addr = name? inet_addr(name): htonl(INADDR_ANY);

//	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (bind(sock,(struct sockaddr *)&a,alen))
		return  edg_wll_SetError(ctx, errno, "bind() failed");

	
	if (listen(sock,CON_QUEUE)) 
		return edg_wll_SetError(ctx, errno, "listen() failed");
	
	*fd = sock;
	
	return edg_wll_Error(ctx,NULL,NULL);
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
	struct  sockaddr_in	a;
	socklen_t		alen = sizeof(a);
	struct hostent 		*he;
	char 			*name = NULL;
	int			port = 0;
	
	
	if (address_override) {
		struct in_addr in;
		
		get_name_and_port(address_override, &name, &port);
		
		if ( (he = gethostbyname((const char *) name)) == NULL) {
			edg_wll_SetError(ctx, errno, "gethostbyname() failed");
			goto err;
		}
		
		free(name);
	       	
		memmove(&in.s_addr, he->h_addr_list[0], sizeof(in.s_addr));
		name = strdup(inet_ntoa(in));
	
		if ( (he = gethostbyname((const char *) name)) == NULL) {
			edg_wll_SetError(ctx, errno, "gethostbyname() failed");
			goto err;
		}
	
		/* Test whether open sockket represents the same address as address_override
		 * if not, close ctx->notifSock and bind to new socket corresponding to 
		 * address_override
		 */
		if (ctx->notifSock >= 0) {
			if (getsockname(ctx->notifSock, &a, &alen)) 
				return edg_wll_SetError(ctx, errno, "getsockname() failed");
			
			if ( (strcmp(inet_ntoa(a.sin_addr), name)) || (ntohs(a.sin_port) != port) ) {
				
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
		else	
			// used supplied socket
			ctx->notifSock = fd;
	
		if (getsockname(ctx->notifSock, &a, &alen)) 
			return edg_wll_SetError(ctx, errno, "getsockname() failed");

		if (a.sin_addr.s_addr == INADDR_ANY)
			asprintf(address,"0.0.0.0:%d", ntohs(a.sin_port));
		else
			asprintf(address,"%s:%d", inet_ntoa(a.sin_addr), ntohs(a.sin_port));
	}

err:
	free(name);
	
	return edg_wll_Error(ctx,NULL,NULL);
}



int edg_wll_NotifNew(
        edg_wll_Context         ctx,
        edg_wll_QueryRec        const * const *conditions,
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
			EDG_WLL_NOTIF_NOOP, conditions, &send_mess)) )
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

	
	// if a local listening socket active, close it
	if (ctx->notifSock >= 0) {
		if (close(ctx->notifSock)) 
			return edg_wll_SetError(ctx, errno, "close() failed");
		else
			ctx->notifSock = -1;
	}

	if (set_server_name_and_port(ctx)) 
		goto err;
		
	if (get_client_address(ctx, fd, address_override, &address))
		goto err;

	if (edg_wll_NotifRequestToXML(ctx, "Bind", id, address, 
			EDG_WLL_NOTIF_NOOP, NULL, &send_mess)) 
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
			op, conditions, &send_mess)) 
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
			EDG_WLL_NOTIF_NOOP, NULL, &send_mess)) 
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
        edg_wll_NotifId         *id)
{
	char	*send_mess = NULL, *recv_mess = NULL, *response = NULL;
	time_t 	not_used;
	
	
	edg_wll_ResetError(ctx);

	
	if (set_server_name_and_port(ctx)) 
		goto err;
		
	if (edg_wll_NotifRequestToXML(ctx, "Drop", id, NULL, 
			EDG_WLL_NOTIF_NOOP, NULL, &send_mess)) 
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



static int recv_notif(edg_wll_Context ctx)
{
	int 	ret, len;
	char 	fbuf[17];
	size_t  total;

	
	if (ctx->connPool[ctx->connToUse].buf) {
		free(ctx->connPool[ctx->connToUse].buf);
		ctx->connPool[ctx->connToUse].buf = NULL;
	}
	ctx->connPool[ctx->connToUse].bufUse = 0;
	ctx->connPool[ctx->connToUse].bufSize = 0;

	
	if ((ret=edg_wll_ssl_read_full(ctx->connPool[ctx->connToUse].ssl,
					fbuf,17,&ctx->p_tmp_timeout,&total)) < 0) 
		switch (ret) {
			case EDG_WLL_SSL_ERROR_TIMEOUT: 
				return edg_wll_SetError(ctx,ETIMEDOUT,"read message header");
			case EDG_WLL_SSL_ERROR_EOF: 
				return edg_wll_SetError(ctx,ENOTCONN,NULL);
			default: 
				return edg_wll_SetError(ctx,EDG_WLL_ERROR_SSL,"read message header");
	}

	if ((len = atoi(fbuf)) <= 0) {
		return edg_wll_SetError(ctx,EINVAL,"message length");
	}

	ctx->connPool[ctx->connToUse].bufSize = len+1;

	ctx->connPool[ctx->connToUse].buf = (char *) malloc(
		ctx->connPool[ctx->connToUse].bufSize);
	
	if (!ctx->connPool[ctx->connToUse].buf) {
		return edg_wll_SetError(ctx, ENOMEM, "recv_notif()");
	}
	

	if ((ret=edg_wll_ssl_read_full(ctx->connPool[ctx->connToUse].ssl,
					ctx->connPool[ctx->connToUse].buf,
					len,
					&ctx->p_tmp_timeout,&total)) < 0) {
		free(ctx->connPool[ctx->connToUse].buf);
		ctx->connPool[ctx->connToUse].bufUse = 0;
		ctx->connPool[ctx->connToUse].bufSize = 0;
		return edg_wll_SetError(ctx,
			ret == EDG_WLL_SSL_ERROR_TIMEOUT ?
				ETIMEDOUT : EDG_WLL_ERROR_SSL,
			"read message");
	}


	ctx->connPool[ctx->connToUse].buf[len] = 0;
	ctx->connPool[ctx->connToUse].bufUse = len+1;

	
	return edg_wll_Error(ctx,NULL,NULL);
}



static int send_reply(const edg_wll_Context ctx)
{
	int	ret, len, err_code, err_code_min = 0, max_len = 256;
	char	*p, *err_msg = NULL, buf[max_len];
	size_t  total;

	
	err_code = edg_wll_Error(ctx,NULL,&err_msg);

	if (!err_msg) err_msg=strdup("OK");
	
	len = 17 + len_int(err_code) + len_int(err_code_min) +len_string(err_msg);
	if(len > max_len) {
		edg_wll_SetError(ctx,E2BIG,"create_reply()");
		goto err;
	}

	snprintf(buf, max_len, "%16d\n", len - 17);
	p = buf + 17;
	p = put_int(p, err_code);
	p = put_int(p, err_code_min);
	p = put_string(p, err_msg);

	
	if ((ret = edg_wll_ssl_write_full(ctx->connPool[ctx->connToUse].ssl,
					buf,len,&ctx->p_tmp_timeout,&total)) < 0) {
		edg_wll_SetError(ctx,
				ret == EDG_WLL_SSL_ERROR_TIMEOUT ? 
					ETIMEDOUT : EDG_WLL_ERROR_SSL,
					"write reply");
		goto err;
	}
	
err:
	free(err_msg);
	return edg_wll_Error(ctx,NULL,NULL);
}



int edg_wll_NotifReceive(
        edg_wll_Context         ctx,
        int                     fd,
        const struct timeval    *timeout,
        edg_wll_JobStat         *state_out,
        edg_wll_NotifId         *id_out)
{
	fd_set 			fds;	
	struct sockaddr_in	a;
	int 			recv_sock, alen;
	edg_wll_Event 		*event = NULL;
	struct timeval 		start_time,check_time,tv;
	char 			*p = NULL, *ucs = NULL,
       				*event_char = NULL, *jobstat_char = NULL;
	
	

/* start timer */
	gettimeofday(&start_time,0);
	
	if (fd == -1) {
		if (ctx->notifSock == -1) {
			edg_wll_SetError(ctx, EINVAL, "No client socket opened.");
			goto err;
		}
		else {
			fd = ctx->notifSock;
		}
	}

	FD_ZERO(&fds);
	FD_SET(fd,&fds);
	
	tv.tv_sec = timeout->tv_sec;
	tv.tv_usec = timeout->tv_usec;
	
	switch(select(fd+1, &fds, NULL, NULL, &tv)) {
		case -1:
			edg_wll_SetError(ctx, errno, "select() failed");
			goto err;
		case 0:
			edg_wll_SetError(ctx, ETIMEDOUT, "select() timeouted");
			goto err;
		default:
			break;
	}

/* check time */
	gettimeofday(&check_time,0);
	if (decrement_timeout(&tv, start_time, check_time)) {
		edg_wll_SetError(ctx, ETIMEDOUT, "edg_wll_NotifReceive()");
		goto err;
	}
	
	start_time = check_time;
	
	alen=sizeof(a);
	recv_sock = accept(fd,&a,&alen);
	if (recv_sock <0) {
		edg_wll_SetError(ctx, errno, "accept() failed");
		goto err;
	}


	ctx->connPool[ctx->connToUse].ssl =
		edg_wll_ssl_accept(ctx->connPool[ctx->connToUse].gsiCred,recv_sock,&tv);
	
	if (ctx->connPool[ctx->connToUse].ssl == NULL) {
		edg_wll_SetError(ctx, errno, "SSL hanshake failed.");
		goto err;	
	}
	
/* check time */
	gettimeofday(&check_time,0);
	if (decrement_timeout(&tv, start_time, check_time)) {
		edg_wll_SetError(ctx, ETIMEDOUT, "edg_wll_NotifReceive()");
		goto err;
	}
	
	start_time = check_time;
	

	ctx->p_tmp_timeout = tv;
		
	if (recv_notif(ctx)) {
		/* error set in recv_notif() */
		goto err;
	}	

	if (send_reply(ctx)) {
		/* error set in send_reply() */
		goto err;
	}	
	
	p = ctx->connPool[ctx->connToUse].buf;
	p = get_string(p, &ucs);
	if (p == NULL) return edg_wll_SetError(ctx,EDG_WLL_IL_PROTO,"reading UCS");
	free(ucs);

	p = get_string(p, &event_char);
	if (p == NULL) {
		free(ucs);
		return edg_wll_SetError(ctx,EDG_WLL_IL_PROTO,"reading event string");;
	}
	
/* check time */
	gettimeofday(&check_time,0);
	if (decrement_timeout(&tv, start_time, check_time)) {
		edg_wll_SetError(ctx, ETIMEDOUT, "edg_wll_NotifReceive()");
		goto err;
	}
	
	start_time = check_time;
		
	event = edg_wll_InitEvent(EDG_WLL_EVENT_NOTIFICATION);
	if (edg_wll_ParseNotifEvent(ctx, event_char, &event)) {
		goto err;
	}

	jobstat_char = edg_wll_UnescapeXML((const char *) event->notification.jobstat);
	if (jobstat_char == NULL) {
		edg_wll_SetError(ctx, EINVAL, "edg_wll_UnescapeXML()");
		goto err;
	}
		
	/* fill in return values
	 */
	if ( edg_wll_ParseJobStat(ctx, jobstat_char, 
				strlen(jobstat_char), state_out)) {
		goto err;
	}
	
	*id_out = event->notification.notifId;
	event->notification.notifId = NULL;
	
	
err:
	if (event) { 
		edg_wll_FreeEvent(event);
		// XXX - konzultovat s honikem; podle meho by to free 
		// mel delat uz edg_wll_FreeEvent
		//free(event);
	}

	free(ctx->connPool[ctx->connToUse].buf);
	ctx->connPool[ctx->connToUse].buf = NULL;
	ctx->connPool[ctx->connToUse].bufUse = 0;
	ctx->connPool[ctx->connToUse].bufSize = 0;
	
	free(event_char);
	free(jobstat_char);

	// XXX
	// konzultovat s Danem
	edg_wll_ssl_close(ctx->connPool[ctx->connToUse].ssl);
	ctx->connPool[ctx->connToUse].ssl = NULL;
	
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
		if (ctx->connPool[ctx->connToUse].ssl) {
			edg_wll_ssl_close(ctx->connPool[ctx->connToUse].ssl);
			ctx->connPool[ctx->connToUse].ssl = NULL;
		}
		err = close(ctx->notifSock);
		ctx->notifSock = -1;
		
		if (err) 
			return edg_wll_SetError(ctx, errno, "close() failed");
	}

	return edg_wll_Error(ctx,NULL,NULL);
}
