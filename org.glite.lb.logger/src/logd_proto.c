#ident "$Header$"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>

#include "logd_proto.h"
#include "glite/lb/context-int.h"
#include "glite/lb/escape.h"
#include "glite/lb/events_parse.h"

static const int one = 1;

extern char* socket_path;

int edg_wll_ll_log_level;

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
 * send_answer_back - 
 *                     
 *----------------------------------------------------------------------
 */
static int send_answer_back(edg_wll_GssConnection *con, int answer, struct timeval *timeout) {
	int count = 0;
	int err = 0;
	int ans = answer;
	u_int8_t ans_end[4];
	edg_wll_GssStatus	gss_stat;

	edg_wll_ll_log(LOG_INFO,"Sending answer \"%d\" back to client...",answer);
        ans_end[0] = ans & 0xff; ans >>= 8;
        ans_end[1] = ans & 0xff; ans >>= 8;
        ans_end[2] = ans & 0xff; ans >>= 8;
        ans_end[3] = ans;
	if ((err = edg_wll_gss_write_full(con,ans_end,4,timeout,&count, &gss_stat)) < 0 ) {
		edg_wll_ll_log(LOG_INFO,"error.\n");
		return edg_wll_log_proto_server_failure(err,&gss_stat,"Error sending answer");
	} else {
		edg_wll_ll_log(LOG_INFO,"o.k.\n");
		return 0;
	}
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
		edg_wll_ll_log(LOG_ERR,"init_confirmation(): error creating socket\n");
		SYSTEM_ERROR("socket");
		return(-1);
	}

	/* set the socket parameters */
	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	strcpy(saddr.sun_path, confirm_sock_name);

	/* bind the socket */
	if(bind(confirm_sock, (struct sockaddr *)&saddr, sizeof(saddr.sun_path)) < 0) {
		edg_wll_ll_log(LOG_ERR,"init_confirmation(): error binding socket\n");
		SYSTEM_ERROR("bind");
		close(confirm_sock);
		unlink(confirm_sock_name);
		return(-1);
	}

	/* and listen */
	if(listen(confirm_sock, 5) < 0) {
		edg_wll_ll_log(LOG_ERR,"init_confirmation(): error listening on socket\n");
		SYSTEM_ERROR("listen");
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
		edg_wll_ll_log(LOG_ERR,"wait_for_confirmation(): error selecting socket\n");
		SYSTEM_ERROR("select");
		ret = -1;
	} else {
		if (tmp == 0)
			ret = 0;
		else {
			int nsd = accept(confirm_sock, NULL, NULL);
			ret = 1;
			if(nsd < 0) {
				edg_wll_ll_log(LOG_ERR,"wait_for_confirmation(): error accepting a connection on a socket\n");
				SYSTEM_ERROR("accept");
				ret = -1;
			} else {
				if(recv(nsd, code, sizeof(*code), MSG_NOSIGNAL) < 0) {
					edg_wll_ll_log(LOG_ERR,"wait_for_confirmation(): error receiving a message from a socket\n");
					SYSTEM_ERROR("recv");
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
	struct sockaddr_in my_addr;

	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(port);

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == -1) { 
		edg_wll_ll_log(LOG_ERR,"do_listen(): error creating socket\n");
		SYSTEM_ERROR("socket"); 
		return -1; 
	}

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	ret = bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr));
	if (ret == -1) { 
		edg_wll_ll_log(LOG_ERR,"do_listen(): error binding socket\n");
		SYSTEM_ERROR("bind"); 
		return -1; 
	}

	ret = listen(sock, 5);
	if (ret == -1) { 
		edg_wll_ll_log(LOG_ERR,"do_listen(): error listening on socket\n");
		SYSTEM_ERROR("listen"); 
		close(sock); 
		return -1; 
	}

	return sock;
}

/*!
 *----------------------------------------------------------------------
 * Write to socket
 * Needn't write entire buffer. Timeout is applicable only for non-blocking
 * connections 
 * \param sock IN: connection to work with
 * \param buf IN: buffer
 * \param bufsize IN: max size to write
 * \param timeout INOUT: max time allowed for operation, remaining time on return
 * \retval bytes written (>0) on success
 * \retval -1 on write error
 *----------------------------------------------------------------------
 */
static ssize_t edg_wll_socket_write(int sock,const void *buf,size_t bufsize,struct timeval *timeout)
{ 
        ssize_t	len = 0, ret = 0;
	fd_set	fds;
	struct timeval	to,before,after;

	if (timeout) {
		memcpy(&to,timeout,sizeof to);
		gettimeofday(&before,NULL);
	}
        len = write(sock,buf,bufsize);
        while (len <= 0) {
		FD_ZERO(&fds);
		FD_SET(sock,&fds);
		if ((ret=select(sock+1,&fds,NULL,NULL,timeout?&to:NULL)) < 0) {
			edg_wll_ll_log(LOG_ERR,"edg_wll_socket_write(): error selecting socket\n");
			SYSTEM_ERROR("select"); 
			break;
		}
                len = write(sock,buf,bufsize);
        }
        if (timeout) {
           gettimeofday(&after,NULL);
           tv_sub(after,before);
           tv_sub(*timeout,after);
           if (timeout->tv_sec < 0) {
                timeout->tv_sec = 0;
                timeout->tv_usec = 0;
           }
	}
        return len;
} 

/*!
 *----------------------------------------------------------------------
 * Write specified amount of data to socket
 * Attempts to call edg_wll_socket_write() untill the entire request is satisfied
 * (or times out).
 * \param sock IN: connection to work with
 * \param buf IN: buffer
 * \param bufsize IN: max size to write
 * \param timeout INOUT: max time allowed for operation, remaining time on return
 * \param total OUT: bytes actually written
 * \retval bytes written (>0) on success
 * \retval -1 on write error
 *----------------------------------------------------------------------
 */
static ssize_t edg_wll_socket_write_full(int sock,void *buf,size_t bufsize,struct timeval *timeout,ssize_t *total)
{
        ssize_t	len;
        *total = 0;

        while (*total < bufsize) {
                len = edg_wll_socket_write(sock,buf+*total,bufsize-*total,timeout);
                if (len < 0) return len;
                *total += len;
        }
        return 0;
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
int edg_wll_log_proto_server(edg_wll_GssConnection *con, char *name, char *prefix, int noipc, int noparse)
{
	char	*buf,*dglllid,*dguser,*jobId,*name_esc;
	char	header[EDG_WLL_LOG_SOCKET_HEADER_LENGTH+1];
	char	outfilename[FILENAME_MAX];
	int	count,count_total,size;
	u_int8_t size_end[4];
	size_t  msg_size,dglllid_size,dguser_size;
	int	i,answer,answer_sent;
	int	msg_sock;
	char    *msg,*msg_begin;
	FILE	*outfile;
	int	filedesc,filelock_status,flags;
	long	filepos;
	struct flock filelock;
	int	priority;
	long	lllid;
	int	unique;
	struct timeval timeout;
	int	err;
	edg_wll_Context	context;
	edg_wll_Event	*event;
	edg_wll_GssStatus	gss_stat;
                
	errno = i = answer = answer_sent = size = msg_size = dglllid_size = dguser_size = count = count_total = msg_sock = filedesc = filelock_status = /* priority */ unique = err = 0;     
        buf = dglllid = dguser = jobId = name_esc = msg = msg_begin = NULL;
	event = NULL;
	if (EDG_WLL_LOG_TIMEOUT_MAX > EDG_WLL_LOG_SYNC_TIMEOUT_MAX) timeout.tv_sec = EDG_WLL_LOG_TIMEOUT_MAX;
	else timeout.tv_sec = EDG_WLL_LOG_SYNC_TIMEOUT_MAX;
        timeout.tv_usec = 0;
	if (edg_wll_InitContext(&context) != 0) {
		edg_wll_ll_log(LOG_ERR,"edg_wll_InitContex(): error.\n");
		answer = ENOMEM; 
		goto edg_wll_log_proto_server_end; 
	}
	if (edg_wll_ResetError(context) != 0) { 
		edg_wll_ll_log(LOG_ERR,"edg_wll_ResetError(): error.\n");
		answer = ENOMEM; 
		goto edg_wll_log_proto_server_end;
	}

	/* look for the unique unused long local-logger id (LLLID) */
	lllid = 1000*getpid();
	for (i=0; (i<1000)&&(!unique); i++) {
		lllid += i;
		snprintf(confirm_sock_name, sizeof(confirm_sock_name), "/tmp/dglogd_sock_%ld", lllid);
		if ((filedesc = open(confirm_sock_name,O_CREAT)) == -1) {
			if (errno == EEXIST) {
				edg_wll_ll_log(LOG_WARNING,"Warning: LLLID %ld already in use.\n",lllid);
			} else {
				SYSTEM_ERROR("open");
			}
		} else {
			unique = 1;
			close(filedesc); filedesc = 0;
			unlink(confirm_sock_name);
		}
	}
	if (!unique) {
		edg_wll_ll_log(LOG_ERR,"Cannot determine the unique long local-logger id (LLLID)!\n",lllid);
		return EAGAIN;
	}
	edg_wll_ll_log(LOG_INFO,"Long local-logger id (LLLID): %ld\n",lllid);

	/* receive socket header */
	edg_wll_ll_log(LOG_INFO,"Reading socket header...");
	memset(header, 0, EDG_WLL_LOG_SOCKET_HEADER_LENGTH+1);
	if ((err = edg_wll_gss_read_full(con, header, EDG_WLL_LOG_SOCKET_HEADER_LENGTH, &timeout, &count, &gss_stat)) < 0) {
		edg_wll_ll_log(LOG_INFO,"error.\n");
		answer = edg_wll_log_proto_server_failure(err,&gss_stat,"Error receiving header");
		goto edg_wll_log_proto_server_end;
	} else {
		edg_wll_ll_log(LOG_INFO,"o.k.\n");
	}
	edg_wll_ll_log(LOG_DEBUG,"Checking socket header...");
	header[EDG_WLL_LOG_SOCKET_HEADER_LENGTH] = '\0';
	if (strncmp(header,EDG_WLL_LOG_SOCKET_HEADER,EDG_WLL_LOG_SOCKET_HEADER_LENGTH)) {
		/* not the proper socket header text */
		edg_wll_ll_log(LOG_DEBUG,"error.\n");
		edg_wll_ll_log(LOG_ERR,"edg_wll_log_proto_server(): invalid socket header\n");
		edg_wll_ll_log(LOG_DEBUG,"edg_wll_log_proto_server(): read header '%s' instead of '%s'\n",
				header,EDG_WLL_LOG_SOCKET_HEADER);
		answer = EINVAL;
		goto edg_wll_log_proto_server_end;
	} else {
		edg_wll_ll_log(LOG_DEBUG,"o.k.\n");
	}

/*
	edg_wll_ll_log(LOG_DEBUG,"Reading message priority...");
	count = 0;
	if ((err = edg_wll_gss_read_full(con, &priority, sizeof(priority), &timeout, &count, &gss_stat)) < 0) {
		edg_wll_ll_log(LOG_DEBUG,"error.\n");
		answer = edg_wll_log_proto_server_failure(err,&gss_stat,"Error receiving message priority");
                goto edg_wll_log_proto_server_end;
        } else {
		edg_wll_ll_log(LOG_DEBUG,"o.k.\n");
	}
*/

        edg_wll_ll_log(LOG_DEBUG,"Reading message size...");
	count = 0;
	if ((err = edg_wll_gss_read_full(con, size_end, 4, &timeout, &count,&gss_stat)) < 0) {
		edg_wll_ll_log(LOG_DEBUG,"error.\n");
		answer = edg_wll_log_proto_server_failure(err,&gss_stat,"Error receiving message size");
                goto edg_wll_log_proto_server_end;
	} else {
		edg_wll_ll_log(LOG_DEBUG,"o.k.\n");
	}
	size = size_end[3]; size <<=8; 
	size |= size_end[2]; size <<=8; 
	size |= size_end[1]; size <<=8; 
	size |= size_end[0];
        edg_wll_ll_log(LOG_DEBUG,"Checking message size...");
	if (size <= 0) {
		edg_wll_ll_log(LOG_DEBUG,"error.\n");
		/* probably wrong size in the header or nothing to read */
		edg_wll_ll_log(LOG_ERR,"edg_wll_log_proto_server(): invalid size read from socket header\n");
		edg_wll_ll_log(LOG_DEBUG,"Read size '%d'.\n",size);
		answer = EINVAL;
		goto edg_wll_log_proto_server_end;
	} else {
		edg_wll_ll_log(LOG_DEBUG,"o.k.\n");
		edg_wll_ll_log(LOG_DEBUG,"- Size read from header: %d bytes.\n",size);
	}

	/* format the DG.LLLID string */
	if (asprintf(&dglllid,"DG.LLLID=%ld ",lllid) == -1) {
		edg_wll_ll_log(LOG_ERR,"edg_wll_log_proto_server(): nomem for DG.LLLID\n");
		SYSTEM_ERROR("asprintf");
		answer = ENOMEM;
		goto edg_wll_log_proto_server_end;
	}
	dglllid_size = strlen(dglllid);

	/* format the DG.USER string */
	name_esc = edg_wll_LogEscape(name);
	if (asprintf(&dguser,"DG.USER=\"%s\" ",name_esc) == -1) {
		edg_wll_ll_log(LOG_ERR,"edg_wll_log_proto_server(): nomem for DG.USER\n");
		SYSTEM_ERROR("asprintf");
		answer = ENOMEM;
		goto edg_wll_log_proto_server_end;
	}
	dguser_size = strlen(dguser);

	/* allocate enough memory for all data */
	msg_size = dglllid_size + dguser_size + size + 1;
	if ((msg = malloc(msg_size)) == NULL) {
		edg_wll_ll_log(LOG_ERR,"edg_wll_log_proto_server(): out of memory for allocating message\n");
		SYSTEM_ERROR("malloc");
		answer = ENOMEM;
		goto edg_wll_log_proto_server_end;
	}
	strncpy(msg,dglllid,dglllid_size);
	msg_begin = msg + dglllid_size; // this is the "official" beginning of the message
	strncpy(msg_begin,dguser,dguser_size);

	/* receive message */
	edg_wll_ll_log(LOG_INFO,"Reading message from socket...");
	buf = msg_begin + dguser_size;
	count = 0;
	if ((err = edg_wll_gss_read_full(con, buf, size, &timeout, &count, &gss_stat)) < 0) {
		edg_wll_ll_log(LOG_INFO,"error.\n");
		answer = edg_wll_log_proto_server_failure(err,&gss_stat,"Error receiving message");
		goto edg_wll_log_proto_server_end;
	} else {
		edg_wll_ll_log(LOG_INFO,"o.k.\n");
	}       

	if (buf[count] != '\0') buf[count] = '\0';

	/* parse message and get jobId and priority from it */
	if (!noparse && strstr(msg, "DG.TYPE=\"command\"") == NULL) {
		edg_wll_ll_log(LOG_INFO,"Parsing message for correctness...");
		if (edg_wll_ParseEvent(context,msg_begin,&event) != 0) { 
			edg_wll_ll_log(LOG_INFO,"error.\n");
			edg_wll_ll_log(LOG_ERR,"edg_wll_log_proto_server(): edg_wll_ParseEvent error\n");
			edg_wll_ll_log(LOG_ERR,"edg_wll_ParseEvent(): %s\n",context->errDesc);
			answer = edg_wll_Error(context,NULL,NULL);
			goto edg_wll_log_proto_server_end;
		} else {
			edg_wll_ll_log(LOG_INFO,"o.k.\n");
		}
		edg_wll_ll_log(LOG_DEBUG,"Getting jobId from message...");
		jobId = edg_wlc_JobIdGetUnique(event->any.jobId);
		priority = event->any.priority;
		edg_wll_FreeEvent(event);
		event->any.priority = priority;
		edg_wll_ll_log(LOG_DEBUG,"o.k.\n");
	} else {
/* FIXME: what if edg_wll_InitEvent fails? should be checked somehow -> nomem etc.  */
		event = edg_wll_InitEvent(EDG_WLL_EVENT_UNDEF);
/* XXX:
		event = calloc(1,sizeof(*event));
		if(event == NULL) {
			edg_wll_ll_log(LOG_ERR, "out of memory\n");
			answer = ENOMEM;
			goto edg_wll_log_proto_server_end;
		}
*/

/* XXX: obsolete, logd now doesn't need jobId for 'command' messages,
 * it will be probably needed for writing 'command' messages to some files
		edg_wll_ll_log(LOG_DEBUG,"Getting jobId from message...");
		jobId = edg_wll_GetJobId(msg);
		if (!jobId || edg_wlc_JobIdParse(jobId,&j)) {
			edg_wll_ll_log(LOG_DEBUG,"error.\n");
			edg_wll_ll_log(LOG_ERR,"ParseJobId(%s)\n",jobId?jobId:"NULL");
			answer = EINVAL;
			goto edg_wll_log_proto_server_end;
		} else {
			edg_wll_ll_log(LOG_DEBUG,"o.k.\n");
		}
		free(jobId);
		jobId = edg_wlc_JobIdGetUnique(j);
		edg_wlc_JobIdFree(j);
*/

/* FIXME: get the priority from message some better way */
		if (strstr(msg, "DG.PRIORITY=1") != NULL)
			event->any.priority = 1;
		else event->any.priority = 0;
	}


	/* if not command, save message to file */
	if(strstr(msg, "DG.TYPE=\"command\"") == NULL) {
		/* compose the name of the log file */
//		edg_wll_ll_log(LOG_DEBUG,"Composing filename from prefix \"%s\" and jobId \"%s\"...",prefix,jobId);
		count = strlen(prefix);
		strncpy(outfilename,prefix,count); count_total=count;
		strncpy(outfilename+count_total,".",1); count_total+=1; count=strlen(jobId);
		strncpy(outfilename+count_total,jobId,count); count_total+=count;
		outfilename[count_total]='\0';
//		edg_wll_ll_log(LOG_DEBUG,"o.k.\n");
		edg_wll_ll_log(LOG_INFO,"Writing message to \"%s\"...",outfilename);

		i = 0;
open_event_file:
		/* fopen and properly handle the filelock */
		if ((outfile = fopen(outfilename,"a")) == NULL) {
			edg_wll_ll_log(LOG_INFO,"error.\n");
			SYSTEM_ERROR("fopen"); 
			answer = errno; 
			goto edg_wll_log_proto_server_end;
		} else {
			edg_wll_ll_log(LOG_INFO,".");
		}
		if ((filedesc = fileno(outfile)) == -1) {
			edg_wll_ll_log(LOG_INFO,"error.\n");
			SYSTEM_ERROR("fileno"); 
			answer = errno; 
			fclose(outfile); 
			goto edg_wll_log_proto_server_end;
		} else {
			edg_wll_ll_log(LOG_INFO,".");
		}
		filelock.l_type = F_WRLCK;
		filelock.l_whence = SEEK_SET;
		filelock.l_start = 0;
		filelock.l_len = 0;
		if ((filelock_status=fcntl(filedesc,F_SETLK,&filelock) < 0) && (i < FCNTL_ATTEMPTS)) {
			fclose(outfile);
			edg_wll_ll_log(LOG_DEBUG,"\nWaiting %d seconds for filelock to open...\n",FCNTL_TIMEOUT);
			sleep(FCNTL_TIMEOUT);
			i++;
			goto open_event_file;
		}
		if (filelock_status < 0) {
			edg_wll_ll_log(LOG_INFO,"error.\n");
			SYSTEM_ERROR("fcntl"); 
			answer = errno; 
			fclose(outfile); 
			goto edg_wll_log_proto_server_end;
		} else {
			edg_wll_ll_log(LOG_INFO,".");
		}
		if (fseek(outfile, 0, SEEK_END) == -1) {
			SYSTEM_ERROR("fseek"); 
			answer = errno; 
			fclose(outfile); 
			goto edg_wll_log_proto_server_end;
		}
		if ((filepos=ftell(outfile)) == -1) {
			SYSTEM_ERROR("ftell"); 
			answer = errno; 
			fclose(outfile); 
			goto edg_wll_log_proto_server_end;
		}
		/* write, flush and sync */
		if (fputs(msg,outfile) == EOF) {
			edg_wll_ll_log(LOG_INFO,"error.\n");
			SYSTEM_ERROR("fputs"); 
			answer = errno; 
			fclose(outfile); 
			goto edg_wll_log_proto_server_end;
		}       
		if (fflush(outfile) == EOF) {
			edg_wll_ll_log(LOG_INFO,"error.\n");
			SYSTEM_ERROR("fflush"); 
			answer = errno; 
			fclose(outfile); 
			goto edg_wll_log_proto_server_end;
		}
		if (fsync(filedesc) < 0) { /* synchronize */
			edg_wll_ll_log(LOG_INFO,"error.\n");
			SYSTEM_ERROR("fsync"); 
			answer = errno; 
			fclose(outfile); 
			goto edg_wll_log_proto_server_end;
		} else {
			edg_wll_ll_log(LOG_INFO,"o.k.\n");
		}
		/* close and unlock */
		fclose(outfile); 
	} else {
		filepos = 0;
	}


	/* if not priority send now the answer back to client */
	if (!event->any.priority) {
		if (!send_answer_back(con,answer,&timeout)) { 
			answer_sent = 1;
		}
	} 

	/* send message via IPC (UNIX socket) */
	if (!noipc) {
		struct sockaddr_un saddr;
		edg_wll_ll_log(LOG_INFO,"The message will be send via IPC (UNIX socket):\n");

		/* initialize socket */
		edg_wll_ll_log(LOG_DEBUG,"Initializing UNIX socket...\n");

		edg_wll_ll_log(LOG_DEBUG,"- Getting UNIX socket descriptor...");
		msg_sock = socket(PF_UNIX, SOCK_STREAM, 0);
		if(msg_sock < 0) {
			edg_wll_ll_log(LOG_DEBUG,"error.\n");
			SYSTEM_ERROR("socket"); 
			answer = errno; 
			goto edg_wll_log_proto_server_end;
		} else {
			edg_wll_ll_log(LOG_DEBUG,"o.k.\n");
		}

		edg_wll_ll_log(LOG_DEBUG,"- Setting UNIX socket parameters...");
		memset(&saddr, 0, sizeof(saddr));
		saddr.sun_family = AF_UNIX;
		strcpy(saddr.sun_path, socket_path);
		edg_wll_ll_log(LOG_DEBUG,"o.k.\n");

		edg_wll_ll_log(LOG_DEBUG,"-- adding O_NONBLOCK to socket parameters...");
		if ((flags = fcntl(msg_sock, F_GETFL, 0)) < 0 ||
			fcntl(msg_sock, F_SETFL, flags | O_NONBLOCK) < 0) {
			edg_wll_ll_log(LOG_DEBUG,"error.\n");
			SYSTEM_ERROR("fcntl"); 
			answer = errno; 
			close(msg_sock); 
			goto edg_wll_log_proto_server_end;
		} else {
			edg_wll_ll_log(LOG_DEBUG,"o.k.\n");
		}

		/* for priority messages initialize also another socket for confirmation */
		if (event->any.priority) {
			edg_wll_ll_log(LOG_DEBUG,"- Initializing 2nd UNIX socket for priority messages confirmation...");
			if(init_confirmation() < 0) { 
				edg_wll_ll_log(LOG_DEBUG,"error.\n");
				answer = errno; 
				goto edg_wll_log_proto_server_end; 
			} else {
				edg_wll_ll_log(LOG_DEBUG,"o.k.\n");
			}
		}	  

		edg_wll_ll_log(LOG_DEBUG,"Connecting to UNIX socket...");
		for (i=0; i < CONNECT_ATTEMPTS; i++) {
			if(connect(msg_sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
				if ((errno == EAGAIN) || (errno == ETIMEDOUT)) {
					edg_wll_ll_log(LOG_DEBUG,"."); 
					sleep(CONNECT_TIMEOUT);
					continue;
				} else if (errno == EISCONN) {
					edg_wll_ll_log(LOG_DEBUG,"warning.\n");
					edg_wll_ll_log(LOG_ERR,"The socket is already connected!\n");
					break;
				} else {
					edg_wll_ll_log(LOG_DEBUG,"error.\n");
					SYSTEM_ERROR("connect"); 
					answer = errno; 
					close(msg_sock); 
					goto edg_wll_log_proto_server_end_1;
				}
			} else {
				edg_wll_ll_log(LOG_DEBUG,"o.k.\n");
				break;
			}
		}

		edg_wll_ll_log(LOG_DEBUG,"Sending via IPC the message position %ld (%d bytes)...", filepos, sizeof(filepos));
		count = 0;
		if (edg_wll_socket_write_full(msg_sock, &filepos, sizeof(filepos), &timeout, &count) < 0) {
			edg_wll_ll_log(LOG_DEBUG,"error.\n");
			edg_wll_ll_log(LOG_ERR,"edg_wll_socket_write_full(): error,\n");
			answer = errno; 
			close(msg_sock); 
			goto edg_wll_log_proto_server_end_1;
		} else {
			edg_wll_ll_log(LOG_DEBUG,"o.k.\n");
		}

		edg_wll_ll_log(LOG_DEBUG,"Sending via IPC the message itself (%d bytes)...",msg_size);
		if (edg_wll_socket_write_full(msg_sock, msg, msg_size, &timeout, &count) < 0) {
			edg_wll_ll_log(LOG_DEBUG,"error.\n");
			edg_wll_ll_log(LOG_ERR,"edg_wll_socket_write_full(): error."); 
			answer = errno; 
			close(msg_sock); 
			goto edg_wll_log_proto_server_end_1;
		} else {
			edg_wll_ll_log(LOG_DEBUG,"o.k.\n");
		}

		close(msg_sock);

		if (event->any.priority) {
			edg_wll_ll_log(LOG_INFO,"Waiting for confirmation...");
			if ((count = wait_for_confirmation(&timeout, &answer)) < 0) {
				edg_wll_ll_log(LOG_INFO,"error.\n");
				edg_wll_ll_log(LOG_ERR,"wait_for_confirmation(): error.\n"); 
				answer = errno;
			} else {
				edg_wll_ll_log(LOG_INFO,"o.k.\n");
				if (count == 0) {
					edg_wll_ll_log(LOG_DEBUG,"Waking up, timeout expired.\n");
					answer = EAGAIN;
				} else {
					edg_wll_ll_log(LOG_DEBUG,"Confirmation received, waking up.\n");
				}
			}
		}
	} else {
		edg_wll_ll_log(LOG_NOTICE,"Not sending via IPC.\n");
	}

edg_wll_log_proto_server_end:
	/* if not sent already, send the answer back to client */
	if (!answer_sent) {
		answer = send_answer_back(con,answer,&timeout);
	} 
	/* clean */
	edg_wll_FreeContext(context);
	if (name_esc) free(name_esc);
	if (dglllid) free(dglllid);
	if (dguser) free(dguser);
	if (jobId) free(jobId);
	if (msg) free(msg);
	if (event) free(event);

	edg_wll_ll_log(LOG_INFO,"Done.\n");

	return answer;

edg_wll_log_proto_server_end_1:
	if (event->any.priority) {
		close(confirm_sock);
		unlink(confirm_sock_name);
	}	
	goto edg_wll_log_proto_server_end;
}

/*
 *----------------------------------------------------------------------
 *
 * edg_wll_log_proto_server_failure - handle protocol failures on the server side
 *
 * Returns: errno
 *
 *----------------------------------------------------------------------
 */
int edg_wll_log_proto_server_failure(int code, edg_wll_GssStatus *gss_code, const char *text)
{
	const char	*func = "edg_wll_log_proto_server()";
        int             ret = 0;

	if(code>0) {
                return(0);
	}
	switch(code) {
		case EDG_WLL_GSS_ERROR_EOF: 
			edg_wll_ll_log(LOG_ERR,"%s: %s, EOF occured\n", func, text);	
			ret = EAGAIN;
			break;
		case EDG_WLL_GSS_ERROR_TIMEOUT: 
			edg_wll_ll_log(LOG_ERR,"%s: %s, timeout expired\n", func, text);	
			ret = EAGAIN;
			break;
		/* XXX DK: co tenhle break??: */
		case EDG_WLL_GSS_ERROR_ERRNO: perror("edg_wll_gss_read()"); break;
			edg_wll_ll_log(LOG_ERR,"%s: %s, system error occured\n", func, text);	
			ret = EAGAIN;
			break;
		case EDG_WLL_GSS_ERROR_GSS:
			{
			   char *gss_err;

			   edg_wll_gss_get_error(gss_code, "GSS error occured", &gss_err);
			   edg_wll_ll_log(LOG_ERR,"%s: %s, %s\n", func, text, gss_err);
			   free(gss_err);
			   ret = EAGAIN;
			   break;
			}
		default:
			edg_wll_ll_log(LOG_ERR,"%s: %s, unknown error occured\n");
			break;
	}
	return ret;
}

/*
 *----------------------------------------------------------------------
 *
 * edg_wll_ll_log_init - initialize the logging level
 *
 *----------------------------------------------------------------------
 */
void edg_wll_ll_log_init(int level) {
	edg_wll_ll_log_level = level;
}

/*
 *----------------------------------------------------------------------
 *
 * edg_wll_ll_log - print to stderr according to logging level
 *   serious messages are also written to syslog
 *
 *----------------------------------------------------------------------
 */
void edg_wll_ll_log(int level, const char *fmt, ...) {
    char *err_text;
	va_list fmt_args;

	va_start(fmt_args, fmt);
    vasprintf(&err_text, fmt, fmt_args);
    va_end(fmt_args);

	if(level <= edg_wll_ll_log_level) 
		fprintf(stderr, "[%d] %s", (int) getpid(), err_text);
	if(level <= LOG_ERR) {
        openlog("edg-wl-logd", LOG_PID | LOG_CONS, LOG_DAEMON);
		syslog(level, "%s", err_text);
		closelog();
	}

    if(err_text) free(err_text);

    return;
}
