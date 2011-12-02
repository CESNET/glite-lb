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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <netdb.h>

#include "glite/lbu/escape.h"
#include "glite/lbu/log.h"
#include "glite/lb/context-int.h"
#include "glite/lb/events_parse.h"

#include "logd_proto.h"

#define CON_QUEUE 5

static const int zero = 0;
static const int one = 1;

extern char* socket_path;

int glite_common_log_priority_security;
int glite_common_log_priority_access;
int glite_common_log_priority_control;

#define tv_sub(a,b) {\
	(a).tv_usec -= (b).tv_usec;\
	(a).tv_sec -= (b).tv_sec;\
	if ((a).tv_usec < 0) {\
		(a).tv_sec--;\
		(a).tv_usec += 1000000;\
	}\
}

/*
 *----------------------------------------------------------------------
 *
 * handle_gss_failures - handle GSS failures on the server side
 *
 * Returns: errno
 *
 *----------------------------------------------------------------------
 */
static int handle_gss_failures(int code, edg_wll_GssStatus *gss_code, const char *text)
{
	const char	*func = "edg_wll_log_proto_server()";
        int             ret = 0;

	if(code>0) {
                return(0);
	}
	switch(code) {
		case EDG_WLL_GSS_ERROR_EOF: 
			glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN,"%s: %s, EOF occured\n", func, text);	
			ret = EAGAIN;
			break;
		case EDG_WLL_GSS_ERROR_TIMEOUT: 
			glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN,"%s: %s, timeout expired\n", func, text);	
			ret = EAGAIN;
			break;
		case EDG_WLL_GSS_ERROR_ERRNO: 
			glite_common_log_SYS_ERROR(func);
			glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN,"%s: %s, system error occured\n", func, text);	
			ret = EAGAIN;
			break;
		case EDG_WLL_GSS_ERROR_GSS:
			{
			   char *gss_err;

			   edg_wll_gss_get_error(gss_code, "GSS error occured", &gss_err);
			   glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_WARN,"%s: %s, %s\n", func, text, gss_err);
			   free(gss_err);
			   ret = EAGAIN;
			   break;
			}
		default:
			glite_common_log(LOG_CATEGORY_SECURITY,LOG_PRIORITY_ERROR,"%s: %s, unknown error occured\n", func, text);
			break;
	}
	return ret;
}

/*
 *----------------------------------------------------------------------
 *
 * send_answer_back - 
 *                     
 *----------------------------------------------------------------------
 */
static int send_answer_back(edg_wll_GssConnection *con, int answer, struct timeval *timeout) {
	size_t count = 0;
	int err = 0;
	int ans = answer;
	u_int8_t ans_end[4];
	edg_wll_GssStatus	gss_stat;

        ans_end[0] = ans & 0xff; ans >>= 8;
        ans_end[1] = ans & 0xff; ans >>= 8;
        ans_end[2] = ans & 0xff; ans >>= 8;
        ans_end[3] = ans;
	if ((err = edg_wll_gss_write_full(con,ans_end,4,timeout,&count, &gss_stat)) < 0 ) {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_INFO,"Error sending answer \"%d\" back to client.\n",answer);
		return handle_gss_failures(err,&gss_stat,"Error sending answer");
	} else {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Answer \"%d\" succesfully sent back to client.\n",answer);
		return 0;
	}
}

static int daemon_listen(const char *name, char *port, int *conn_out) {
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
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "getaddrinfo: %s", gai_strerror (gaie));
		return 1;
	}
	if (ai == NULL) {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "getaddrinfo: no result");
		return 1;
	}

	conn = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if ( conn < 0 ) { 
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "socket(): %s", strerror(errno));
		freeaddrinfo(ai);
		return 1; 
	}
	setsockopt(conn, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (ai->ai_family == AF_INET6)
		setsockopt(conn, IPPROTO_IPV6, IPV6_V6ONLY, &zero, sizeof(zero));

	if ( bind(conn, ai->ai_addr, ai->ai_addrlen) )
	{
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "bind(%s): %s", port, strerror(errno));
		close(conn);
		freeaddrinfo(ai);
		return 1;
	}

	if ( listen(conn, CON_QUEUE) ) { 
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_FATAL, "listen(): %s", strerror(errno));
		close(conn);
		freeaddrinfo(ai);
		return 1; 
	}

	freeaddrinfo(ai);

	*conn_out = conn;
	return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * wait_for_confirmation -
 *
 * Args:    timeout - number of seconds to wait, 0 => wait indefinitely
 *
 * Returns:  1 => OK, *code contains error code sent by interlogger
 *           0 => timeout expired before anything interesting happened
 *          -1 => some error (see errno for details)
 *
 *----------------------------------------------------------------------
 */
int confirm_sock;
char confirm_sock_name[256];

static
int init_confirmation()
{
	struct sockaddr_un saddr;

	/* create socket */
	if((confirm_sock=socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		glite_common_log_SYS_ERROR("socket");
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_ERROR,"init_confirmation(): error creating socket\n");
		return(-1);
	}

	/* set the socket parameters */
	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	strcpy(saddr.sun_path, confirm_sock_name);

	/* bind the socket */
	if(bind(confirm_sock, (struct sockaddr *)&saddr, sizeof(saddr.sun_path)) < 0) {
		glite_common_log_SYS_ERROR("bind");
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_ERROR,"init_confirmation(): error binding socket\n");
		close(confirm_sock);
		unlink(confirm_sock_name);
		return(-1);
	}

	/* and listen */
	if(listen(confirm_sock, 5) < 0) {
		glite_common_log_SYS_ERROR("listen");
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_ERROR,"init_confirmation(): error listening on socket\n");
		close(confirm_sock);
		unlink(confirm_sock_name);
		return(-1);
	}

	return(0);
}


int wait_for_confirmation(struct timeval *timeout, int *code)
{
	fd_set fds;
	struct timeval  to,before,after;
	int ret = 0, tmp = 0;

	*code = 0;

	FD_ZERO(&fds);
	FD_SET(confirm_sock, &fds);

	/* set timeout */
	if (timeout) {
		memcpy(&to,timeout,sizeof to);
		gettimeofday(&before,NULL);
	}

	/* wait for confirmation at most timeout seconds */
	if ((tmp=select(confirm_sock+1, &fds, NULL, NULL, timeout?&to:NULL)) < 0) {
		glite_common_log_SYS_ERROR("select");
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_ERROR,"wait_for_confirmation(): error selecting socket\n");
		ret = -1;
	} else {
		if (tmp == 0)
			ret = 0;
		else {
			int nsd = accept(confirm_sock, NULL, NULL);
			ret = 1;
			if(nsd < 0) {
				glite_common_log_SYS_ERROR("accept");
				glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_ERROR,"wait_for_confirmation(): error accepting a connection on a socket\n");
				ret = -1;
			} else {
				if(recv(nsd, code, sizeof(*code), MSG_NOSIGNAL) < 0) {
					glite_common_log_SYS_ERROR("recv");
					glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_ERROR,"wait_for_confirmation(): error receiving a message from a socket\n");
					ret = -1;
				}
				close(nsd);
			}
		}
	}
	close(confirm_sock);
	unlink(confirm_sock_name);
        if (timeout) {
           gettimeofday(&after,NULL);
           tv_sub(after,before);
           tv_sub(*timeout,after);
           if (timeout->tv_sec < 0) {
                timeout->tv_sec = 0;
                timeout->tv_usec = 0;
           }
	}
	return ret;
}

/*
 *----------------------------------------------------------------------
 *
 * do_listen - listen on given port
 *
 * Returns: socket handle or -1 if something fails
 *
 * Calls: socket, bind, listen
 *
 * Algorithm:
 *
 *----------------------------------------------------------------------
 */
int do_listen(int port)
{
	int                ret;
	int                sock;
	char 		*portstr = NULL;

	asprintf(&portstr, "%d", port);
	if (portstr == NULL) {
		glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_FATAL,"do_listen(): ENOMEM converting port number\n");
		return -1;
	}

	if (daemon_listen(NULL, portstr, &sock) != 0)
		return -1;

	return sock;
}

/*
 *----------------------------------------------------------------------
 *
 * edg_wll_log_proto_server - handle incoming data
 *
 * Returns: 0 if done properly or errno
 *
 * Calls:
 *
 * Algorithm:
 *
 *----------------------------------------------------------------------
 */
int edg_wll_log_proto_server(edg_wll_GssConnection *con, struct timeval *timeout, char *name, char *prefix, int noipc, int noparse)
{
	char	*buf,*dglllid,*dguser,*jobId,*name_esc;
	char	header[EDG_WLL_LOG_SOCKET_HEADER_LENGTH+1];
	char	outfilename[FILENAME_MAX];
	size_t	count;
	int	count_total,size;
	u_int8_t size_end[4];
	size_t  msg_size,dglllid_size,dguser_size;
	int	i,answer,answer_sent;
	int	msg_sock;
	char    *msg,*msg_begin;
	int	filedesc,filelock_status;
	long	filepos;
	int	priority;
	long	lllid;
	int	unique;
	int	err;
	edg_wll_Context	context;
	edg_wll_Event	*event;
	edg_wlc_JobId	j;
	edg_wll_GssStatus	gss_stat;
                
	errno = i = answer = answer_sent = size = msg_size = dglllid_size = dguser_size = count = count_total = msg_sock = filedesc = filelock_status = /* priority */ unique = err = 0;     
        buf = dglllid = dguser = jobId = name_esc = msg = msg_begin = NULL;
	event = NULL;

	/* init */
	if (edg_wll_InitContext(&context) != 0) {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_ERROR,"edg_wll_InitContex(): error.\n");
		answer = ENOMEM; 
		goto edg_wll_log_proto_server_end; 
	}
	if (edg_wll_ResetError(context) != 0) { 
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_ERROR,"edg_wll_ResetError(): error.\n");
		answer = ENOMEM; 
		goto edg_wll_log_proto_server_end;
	}

	/* look for the unique unused long local-logger id (LLLID) */
	lllid = 1000*getpid();
	for (i=0; (i<1000)&&(!unique); i++) {
		struct stat statbuf;
		lllid += i;
		snprintf(confirm_sock_name, sizeof(confirm_sock_name), "/tmp/dglogd_sock_%ld", lllid);
		if ((filedesc = stat(confirm_sock_name,&statbuf)) == 0) {
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"Warning: LLLID %ld already in use.\n",lllid);
		} else {
			if (errno == ENOENT) {
				unique = 1;
			} else {
				glite_common_log_SYS_ERROR("stat");
			}
		}
	}
	if (!unique) {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_ERROR,"Cannot determine the unique long local-logger id (LLLID %ld)!\n",lllid);
		return EAGAIN;
	}
	glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_INFO,"Long local-logger id (LLLID): %ld ... [ok]\n",lllid);

	/* receive socket header */
	memset(header, 0, EDG_WLL_LOG_SOCKET_HEADER_LENGTH+1);
	if ((err = edg_wll_gss_read_full(con, header, EDG_WLL_LOG_SOCKET_HEADER_LENGTH, timeout, &count, &gss_stat)) < 0) {
		if (err == EDG_WLL_GSS_ERROR_EOF) {
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"Reading socket header - no data available.\n");
			answer = err;
			answer_sent = 1; /* i.e. do not try to send answer back */
		} else {
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"Error reading socket header.\n");
			answer = handle_gss_failures(err,&gss_stat,"Error reading socket header");
		}
		goto edg_wll_log_proto_server_end;
	} else {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Reading socket header... [ok]\n");
	}

	/* Check socket header */
	header[EDG_WLL_LOG_SOCKET_HEADER_LENGTH] = '\0';
	if (strncmp(header,EDG_WLL_LOG_SOCKET_HEADER,EDG_WLL_LOG_SOCKET_HEADER_LENGTH)) {
		/* not the proper socket header text */
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"edg_wll_log_proto_server(): invalid socket header\n");
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"edg_wll_log_proto_server(): read header '%s' instead of '%s'\n",
				header,EDG_WLL_LOG_SOCKET_HEADER);
		answer = EINVAL;
		goto edg_wll_log_proto_server_end;
	} else {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Read socket header: \"%s\" [ok]\n",header);
	}

/* XXX: obsolete
	count = 0;
	if ((err = edg_wll_gss_read_full(con, &priority, sizeof(priority), timeout, &count, &gss_stat)) < 0) {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"Error reading message priority.\n");
		answer = handle_gss_failures(err,&gss_stat,"Error receiving message priority");
                goto edg_wll_log_proto_server_end;
        } else {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Read message priority: %d [ok]\n",priority);
	}
*/

        /* read message size */
	count = 0;
	if ((err = edg_wll_gss_read_full(con, size_end, 4, timeout, &count,&gss_stat)) < 0) {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Error reading message size.\n");
		answer = handle_gss_failures(err,&gss_stat,"Error reading message size");
                goto edg_wll_log_proto_server_end;
	} else {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Reading message size... [ok]\n");
	}
	size = size_end[3]; size <<=8; 
	size |= size_end[2]; size <<=8; 
	size |= size_end[1]; size <<=8; 
	size |= size_end[0];
	if (size <= 0) {
		/* probably wrong size in the header or nothing to read */
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"edg_wll_log_proto_server(): invalid size read from socket header\n");
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Read message size '%d' [error].\n",size);
		answer = EINVAL;
		goto edg_wll_log_proto_server_end;
	} else {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Read message size: %d bytes [ok].\n",size);
	}

	/* format the DG.LLLID string */
	if (asprintf(&dglllid,"DG.LLLID=%ld ",lllid) == -1) {
		glite_common_log_SYS_ERROR("asprintf");
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"edg_wll_log_proto_server(): nomem for DG.LLLID\n");
		answer = ENOMEM;
		goto edg_wll_log_proto_server_end;
	}
	dglllid_size = strlen(dglllid);

	/* format the DG.USER string */
	name_esc = glite_lbu_EscapeULM(name);
	if (asprintf(&dguser,"DG.USER=\"%s\" ",name_esc) == -1) {
		glite_common_log_SYS_ERROR("asprintf");
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"edg_wll_log_proto_server(): nomem for DG.USER\n");
		answer = ENOMEM;
		goto edg_wll_log_proto_server_end;
	}
	dguser_size = strlen(dguser);

	/* allocate enough memory for all data */
	msg_size = dglllid_size + dguser_size + size + 1;
	if ((msg = malloc(msg_size)) == NULL) {
		glite_common_log_SYS_ERROR("malloc");
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"edg_wll_log_proto_server(): out of memory for allocating message\n");
		answer = ENOMEM;
		goto edg_wll_log_proto_server_end;
	}
	strncpy(msg,dglllid,dglllid_size);
	msg_begin = msg + dglllid_size; // this is the "official" beginning of the message
	strncpy(msg_begin,dguser,dguser_size);

	/* receive message */
	buf = msg_begin + dguser_size;
	count = 0;
	if ((err = edg_wll_gss_read_full(con, buf, size, timeout, &count, &gss_stat)) < 0) {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"Error reading message from socket.\n");
		answer = handle_gss_failures(err,&gss_stat,"Error reading message from socket.");
		goto edg_wll_log_proto_server_end;
	} else {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Reading message... [ok]\n");
		// glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_TRACE,"Read message: \"%s\"\n",msg);
	}       

	buf[count] = '\0';

	/* parse message and get jobId and priority from it */
	if (!noparse && strstr(msg, "DG.TYPE=\"command\"") == NULL) {
		if (edg_wll_ParseEvent(context,msg_begin,&event) != 0) { 
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"edg_wll_log_proto_server(): edg_wll_ParseEvent error\n");
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"edg_wll_ParseEvent(): %s\n",context->errDesc);
			answer = edg_wll_Error(context,NULL,NULL);
			goto edg_wll_log_proto_server_end;
		} else {
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Parsing message for correctness...[ok]\n");
		}
		jobId = edg_wlc_JobIdGetUnique(event->any.jobId);
		priority = event->any.priority;
		edg_wll_FreeEvent(event);
		event->any.priority = priority;
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Getting jobId from message...[ok]\n");
	} else {
		if ((event = edg_wll_InitEvent(EDG_WLL_EVENT_UNDEF)) == NULL) {
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN, "edg_wll_InitEvent(): out of memory\n");
			answer = ENOMEM;
			goto edg_wll_log_proto_server_end;
		}
		jobId = edg_wll_GetJobId(msg);
		if (!jobId || edg_wlc_JobIdParse(jobId,&j)) {
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"Error getting jobId from message.\n");
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"edg_wlc_JobIdParse(%s)\n",jobId?jobId:"NULL");
			answer = EINVAL;
			goto edg_wll_log_proto_server_end;
		} else {
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Getting jobId from message...[ok]\n");
		}
		free(jobId);
		jobId = edg_wlc_JobIdGetUnique(j);
		edg_wlc_JobIdFree(j);

/* TODO: get the priority from message some better way */
		if (strstr(msg, "DG.PRIORITY=1") != NULL)
			event->any.priority = 1;
		else event->any.priority = 0;
	}

	/* if not command, save message to file */
	if(strstr(msg, "DG.TYPE=\"command\"") == NULL) {
		/* compose the name of the log file */
		count = strlen(prefix);
		strncpy(outfilename,prefix,count); count_total=count;
		strncpy(outfilename+count_total,".",1); count_total+=1; count=strlen(jobId);
		strncpy(outfilename+count_total,jobId,count); count_total+=count;
		outfilename[count_total]='\0';
//		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Composing filename from prefix \"%s\" and unique jobId \"%s\"...[ok]",prefix,jobId);

		/* fopen and properly handle the filelock */
#ifdef LOGD_NOFILE
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_INFO,"NOT writing message to \"%s\".\n",outfilename);
		filepos = 0;
#else
		if ( edg_wll_log_event_write(context, outfilename, msg, FCNTL_ATTEMPTS, FCNTL_TIMEOUT, &filepos) ) {
			char *errd;
			// FIXME: there is probably not a correct errno
			glite_common_log_SYS_ERROR("edg_wll_log_event_write");
			answer = edg_wll_Error(context, NULL, &errd);
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"edg_wll_log_event_write error: %s\n",errd);
			free(errd);
			goto edg_wll_log_proto_server_end;
		} else glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_INFO,"Writing message to \"%s\"... [ok]",outfilename);
#endif
	} else {
		filepos = 0;
	}

#ifdef LB_PERF
	glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_INFO,"Calling perftest\n");
	glite_wll_perftest_consumeEventString(msg);
	glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_INFO,"Calling perftest... [done]\n");
#endif

	/* if not priority send now the answer back to client */
	if (!(event->any.priority & (EDG_WLL_LOGFLAG_SYNC|EDG_WLL_LOGFLAG_SYNC_COMPAT))) {
		if (!send_answer_back(con,answer,timeout)) { 
			answer_sent = 1;
		}
		if(init_confirmation() < 0) { 
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"Error initializing 2nd UNIX socket (%s) for priority messages confirmation.\n",confirm_sock_name); 
			answer = errno; 
			goto edg_wll_log_proto_server_end; 
		} else {
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Initializing 2nd UNIX socket (%s) for priority messages confirmation...[ok]\n",confirm_sock_name);
		}
	} 

	/* send message via IPC (UNIX socket) */
	if (!noipc) {

		if ( edg_wll_log_event_send(context, socket_path, filepos, msg, msg_size, CONNECT_ATTEMPTS, timeout) ) {
			char *errd;
			// XXX: probably not a SYSTEM ERROR
			// glite_common_log_SYS_ERROR("edg_wll_log_event_send");
			answer = edg_wll_Error(context, NULL, &errd);
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"edg_wll_log_event_send error: %s\n",errd);
			free(errd);
			goto edg_wll_log_proto_server_end_1;
		} else glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,
			"Sending via IPC (UNIX socket \"%s\")\n\t"
			"the message position %ld (%ld bytes)... [ok]",
			socket_path, filepos, sizeof(filepos));

	} else {
		glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"NOT sending via IPC.\n");
	}

	if (event->any.priority & (EDG_WLL_LOGFLAG_SYNC|EDG_WLL_LOGFLAG_SYNC_COMPAT)) {
		if ((count = wait_for_confirmation(timeout, &answer)) < 0) {
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_WARN,"Error waiting for confirmation.\n");
			answer = errno;
		} else {
			glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Waiting for confirmation... [ok].\n");
			if (count == 0) {
				glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Waking up, timeout expired.\n");
				answer = EAGAIN;
			} else {
				glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_DEBUG,"Confirmation received, waking up.\n");
			}
		}
	}

edg_wll_log_proto_server_end:
	/* if not sent already, send the answer back to client */
	if (!answer_sent) {
		answer = send_answer_back(con,answer,timeout);
	} 
	/* clean */
	edg_wll_FreeContext(context);
	if (name_esc) free(name_esc);
	if (dglllid) free(dglllid);
	if (dguser) free(dguser);
	if (jobId) free(jobId);
	if (msg) free(msg);
	if (event) free(event);

//	glite_common_log(LOG_CATEGORY_ACCESS,LOG_PRIORITY_INFO,"Done.\n");

	return answer;

edg_wll_log_proto_server_end_1:
	if (event->any.priority) {
		close(confirm_sock);
		unlink(confirm_sock_name);
	}	
	goto edg_wll_log_proto_server_end;
}

