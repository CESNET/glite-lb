#ident "$Header$"

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
			if (getsockname(ctx->notifSock, (struct sockaddr *) &a, &alen)) 
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
	
		if (getsockname(ctx->notifSock,(struct sockaddr *)  &a, &alen)) 
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
			EDG_WLL_NOTIF_NOOP, *valid, conditions, &send_mess)) )
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
			EDG_WLL_NOTIF_NOOP, *valid, NULL, &send_mess)) 
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
			op, -1, conditions, &send_mess)) 
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
			EDG_WLL_NOTIF_NOOP, *valid, NULL, &send_mess)) 
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
			EDG_WLL_NOTIF_NOOP, -1, NULL, &send_mess)) 
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



static int gss_reader(void *user_data, char *buffer, int max_len)
{
  edg_wll_GssStatus gss_code;
  edg_wll_Context tmp_ctx = (edg_wll_Context)user_data;
  int ret;
  size_t	len;

  ret = edg_wll_gss_read_full(&tmp_ctx->connPoolNotif[0].gss,
			      buffer, max_len,
			      &tmp_ctx->p_tmp_timeout,
			      &len, &gss_code);
  if(ret < 0)
    switch(ret) {
    case EDG_WLL_GSS_ERROR_TIMEOUT:
      edg_wll_SetError(tmp_ctx, ETIMEDOUT, "read message");
      break;
    case EDG_WLL_GSS_ERROR_EOF:
      edg_wll_SetError(tmp_ctx, ENOTCONN, NULL);
      break;
    default:
      edg_wll_SetError(tmp_ctx, EDG_WLL_ERROR_GSS, "read message");
      break;
    }

  return(ret);
}


static int recv_notif(edg_wll_Context ctx)
{
	int 	len;
	
	if (ctx->connPoolNotif[0].buf) {
		free(ctx->connPoolNotif[0].buf);
		ctx->connPoolNotif[0].buf = NULL;
	}
	ctx->connPoolNotif[0].bufUse = 0;
	ctx->connPoolNotif[0].bufSize = 0;
	
	len = read_il_data(ctx, &ctx->connPoolNotif[0].buf, gss_reader);
	if(len < 0)
	  return(len);
	
	ctx->connPoolNotif[0].bufSize = len+1;
	ctx->connPoolNotif[0].bufUse = len+1;

	
	return edg_wll_Error(ctx,NULL,NULL);
}



static int send_reply(const edg_wll_Context ctx)
{
	int	ret, len, err_code, err_code_min = 0;
	char	*buf, *err_msg = NULL;
	size_t  total;
	edg_wll_GssStatus  gss_code;

	
	err_code = edg_wll_Error(ctx,NULL,&err_msg);

	if (!err_msg) err_msg=strdup("OK");
	
	len = encode_il_reply(&buf, err_code, err_code_min, err_msg);
	if(len < 0) {
		edg_wll_SetError(ctx,E2BIG,"create_reply()");
		goto err;
	}

	ret = edg_wll_gss_write_full(&ctx->connPoolNotif[0].gss,
	                             buf,len,&ctx->p_tmp_timeout,&total, &gss_code);
	if (ret < 0) {
		edg_wll_SetError(ctx,
				ret == EDG_WLL_GSS_ERROR_TIMEOUT ? 
					ETIMEDOUT : EDG_WLL_ERROR_GSS,
					"write reply");
		goto err;
	}
	
err:
	if(buf) free(buf);
	free(err_msg);
	return edg_wll_Error(ctx,NULL,NULL);
}




int edg_wll_NotifReceive(
        edg_wll_Context         ctx,
        int                     fd,
        const struct timeval    *timeout,
        edg_wll_JobStat         *state_out,
        edg_wll_NotifId         *id_out)

/* pullup from INFN, support multiple messages from interlogger */
#if 0
{
	fd_set 			fds;	
	struct sockaddr_in	a;
	int 			recv_sock, alen;
	edg_wll_Event 		*event = NULL;
	struct timeval 		start_time,check_time,tv;
	char 			*p = NULL, *ucs = NULL,
       				*event_char = NULL, *jobstat_char = NULL;
	int			ret;
	edg_wll_GssStatus	gss_code;
	
	
	edg_wll_ResetError(ctx);

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

	ret = edg_wll_gss_accept(ctx->connPool[ctx->connToUse].gsiCred, recv_sock,
				 &tv, &ctx->connPool[ctx->connToUse].gss, &gss_code);

	if (ret) {
		edg_wll_SetError(ctx, errno, "GSS authentication failed.");
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
	
	/****************************************************************/
	/* end of  notif-interlogger message exchange			*/
	/****************************************************************/
	
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
	
	if ( id_out ) {
		*id_out = event->notification.notifId;
		event->notification.notifId = NULL;
	}
	
	
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
	/* Dan: ??? */
	edg_wll_gss_close(&ctx->connPool[ctx->connToUse].gss, NULL);
	
	return edg_wll_Error(ctx,NULL,NULL);
}
#endif
/* NotifReceive */
{
	struct pollfd		pollfds[1];
	struct sockaddr_in	a;
	int 			recv_sock;
	socklen_t			alen;
	edg_wll_Event 		*event = NULL;
	struct timeval 		start_time,check_time,tv;
	char 			*event_char = NULL, *jobstat_char = NULL;
	edg_wll_GssStatus	gss_code;
	
	
	edg_wll_ResetError(ctx);

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
	
	pollfds[0].fd = fd;
	pollfds[0].events = POLLIN;
	tv.tv_sec = timeout->tv_sec;
	tv.tv_usec = timeout->tv_usec;
	
	
select:
	/* XXX - index 0 is used because of absence of connection management	*/
       	/* 	 to use it, support in client/connection.c needed		*/
	/*  	 it is better to separate it from ctx->connPool, which is used  */
	/*	 for outgouing messages to server				*/
	/*	 In future it should be in context, so one could use:		*/
	/*	 ctx->connPoolNotif[ctx->connPoolNotifToUse]			*/
	/*	 notif_send() & notif_receive() should then migrate to		*/
	/*	 client/connection.c and use connPool management f-cions	*/

	/* XXX: long-lived contexts may have problems, TODO implement credential reload */

	if (!ctx->connPoolNotif[0].gsiCred) {
		if (edg_wll_gss_acquire_cred_gsi(
			ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_cert_filename,
			ctx->p_proxy_filename ? ctx->p_proxy_filename : ctx->p_key_filename,
			&ctx->connPoolNotif[0].gsiCred,&gss_code))
		{
			edg_wll_SetErrorGss(ctx,"failed aquiring credentials",&gss_code);
			goto err;
		}
	}
	
	if (ctx->connPoolNotif[0].gss.context == NULL) 
	{	
		int 	ret;
		switch(poll(pollfds, 1, tv.tv_sec*1000+tv.tv_usec/1000)) {
			case -1:
				edg_wll_SetError(ctx, errno, "edg_wll_NotifReceive: poll() failed");
				goto err;
			case 0:
				edg_wll_SetError(ctx, ETIMEDOUT, "edg_wll_NotifReceive: poll() timed out");
				goto err;
			default:
				if (!(pollfds[0].revents & POLLIN)) {
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
		start_time = check_time;
		
		alen=sizeof(a);
		recv_sock = accept(fd,(struct sockaddr *)&a,&alen);
		if (recv_sock <0) {
			edg_wll_SetError(ctx, errno, "accept() failed");
			goto err;
		}

		ret = edg_wll_gss_accept(ctx->connPoolNotif[0].gsiCred, recv_sock,
				&tv, &ctx->connPoolNotif[0].gss,&gss_code);

		switch (ret) {
			case EDG_WLL_GSS_OK:
				break;
			case EDG_WLL_GSS_ERROR_ERRNO:
				edg_wll_SetError(ctx,errno,"failed to receive notification");
				goto err;
			case EDG_WLL_GSS_ERROR_GSS:
				edg_wll_SetErrorGss(ctx, "failed to authenticate sender", &gss_code);
				goto err;
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

		/* check time */
		gettimeofday(&check_time,0);
		if (decrement_timeout(&tv, start_time, check_time)) {
			edg_wll_SetError(ctx, ETIMEDOUT, "edg_wll_NotifReceive()");
			goto err;
		}
		start_time = check_time;
	}	

	
	ctx->p_tmp_timeout = tv;
		
	/****************************************************************/
	/* Communication with notif-interlogger				*/
	/****************************************************************/
	
	if (recv_notif(ctx)) {
		if (ctx->errCode == ENOTCONN) {
			/* other side (interlogger-notif) probably closed connection */
			edg_wll_ResetError(ctx);
			
			edg_wll_gss_close(&ctx->connPoolNotif[0].gss,NULL);
			// buffer is freed in recv_notif()
		
			goto select;
		}				
		else {
			goto err;	/* error set in recv_notif() */
		}
	}	

	if (send_reply(ctx)) {
		goto err;		/* error set in send_reply() */
	}
	
	{
		il_octet_string_t ev;

		if(decode_il_msg(&ev, ctx->connPoolNotif[0].buf) < 0)
			return edg_wll_SetError(ctx, EDG_WLL_IL_PROTO, "decoding event string");
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
	start_time = check_time;
		
	event = edg_wll_InitEvent(EDG_WLL_EVENT_NOTIFICATION);
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
		//free(event);
	}

	free(ctx->connPoolNotif[0].buf);
	ctx->connPoolNotif[0].buf = NULL;
	ctx->connPoolNotif[0].bufUse = 0;
	ctx->connPoolNotif[0].bufSize = 0;
	
	free(event_char);
	free(jobstat_char);

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
		if (ctx->connPoolNotif[0].gss.context != NULL) {
			edg_wll_gss_close(&ctx->connPoolNotif[0].gss, NULL);
		}
		err = close(ctx->notifSock);
		ctx->notifSock = -1;
		
		if (err) 
			return edg_wll_SetError(ctx, errno, "close() failed");
	}

	return edg_wll_Error(ctx,NULL,NULL);
}
