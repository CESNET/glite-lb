/**
 * il_notification.c
 *   - implementation of IL API calls for notifications
 *
 */
#ident "$Header$"

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "glite/lb/context-int.h"
#include "glite/lb/notifid.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/escape.h"
#include "glite/lb/log_proto.h"

#include "il_notification.h"
#include "lb_xml_parse.h"



#define FCNTL_ATTEMPTS		5
#define FCNTL_TIMEOUT		1
#define FILE_PREFIX             "/tmp/notif_events"
#define DEFAULT_SOCKET          "/tmp/notif_interlogger.sock"

char *notif_ilog_socket_path = DEFAULT_SOCKET;

#define tv_sub(a,b) {\
	(a).tv_usec -= (b).tv_usec;\
	(a).tv_sec -= (b).tv_sec;\
	if ((a).tv_usec < 0) {\
		(a).tv_sec--;\
		(a).tv_usec += 1000000;\
	}\
}

static
int
notif_create_ulm(
	edg_wll_Context	context,
	edg_wll_NotifId	reg_id,
	const char	*host,
	const uint16_t	port,
	const char	*owner,
	const char	*notif_data,
	char		**ulm_data,
	char		**reg_id_s)
{
	int		ret;
	edg_wll_Event	*event=NULL;

	*ulm_data = NULL;
	*reg_id_s = NULL;

	event = edg_wll_InitEvent(EDG_WLL_EVENT_NOTIFICATION);

	gettimeofday(&event->any.timestamp,0);
	if (context->p_host) event->any.host = strdup(context->p_host);
	event->any.level = context->p_level;
	event->any.source = context->p_source;
	if (context->p_instance) event->notification.src_instance = strdup(context->p_instance);
	event->notification.notifId = reg_id;
	if (owner) event->notification.owner = strdup(owner);
	if (host) event->notification.dest_host = strdup(host);
	event->notification.dest_port = port;
	if (notif_data) event->notification.jobstat = strdup(notif_data);

	if ((*ulm_data = edg_wll_UnparseNotifEvent(context,event)) == NULL) {
		edg_wll_SetError(context, ret = ENOMEM, "edg_wll_UnparseNotifEvent()"); 
		goto out;
	}

	if((*reg_id_s = edg_wll_NotifIdGetUnique(reg_id)) == NULL) {
		edg_wll_SetError(context, ret = ENOMEM, "edg_wll_NotifIdGetUnique()");
		goto out;
	}

	ret = 0;

out:
	if(event) { 
		edg_wll_FreeEvent(event);
		free(event);
	}
	if(ret) edg_wll_UpdateError(context, ret, "notif_create_ulm()");
	return(ret);
}


static
int
notif_save_to_file(edg_wll_Context     context,
		   const char         *event_file,
		   const char         *ulm_data,
		   long               *filepos)
{
	int ret;
	FILE *outfile;
	int filedesc;
	struct flock filelock;
	int i, filelock_status=-1;

	for(i=0; i < FCNTL_ATTEMPTS; i++) {
		/* fopen and properly handle the filelock */
		if ((outfile = fopen(event_file,"a")) == NULL) {
			edg_wll_SetError(context, ret = errno, "fopen()");
			goto out;
		}
		if ((filedesc = fileno(outfile)) == -1) {
			edg_wll_SetError(context, ret = errno, "fileno()");
			goto out1;
		}
		filelock.l_type = F_WRLCK;
		filelock.l_whence = SEEK_SET;
		filelock.l_start = 0;
		filelock.l_len = 0;
		filelock_status=fcntl(filedesc, F_SETLK, &filelock);
		if(filelock_status < 0) {
			switch(errno) {
			case EAGAIN:
			case EACCES:
			case EINTR:
				/* lock is held by someone else */
				sleep(FCNTL_TIMEOUT);
				break;
			default:
				/* other error */
				edg_wll_SetError(context, ret=errno, "fcntl()");
				goto out1;
			}
		} else {
			/* lock acquired, break out of the loop */
			break;
		}
	}
	if (fseek(outfile, 0, SEEK_END) == -1) {
		edg_wll_SetError(context, ret = errno, "fseek()");
		goto out1;
	}
	if ((*filepos=ftell(outfile)) == -1) {
		edg_wll_SetError(context, ret = errno, "ftell()");
		goto out1;
	}
	/* write, flush and sync */
	if (fputs(ulm_data, outfile) == EOF) {
		edg_wll_SetError(context, ret = errno, "fputs()");
		goto out1;
	}       
	if (fflush(outfile) == EOF) {
		edg_wll_SetError(context, ret = errno, "fflush()");
		goto out1;
	}
	if (fsync(filedesc) < 0) { /* synchronize */
		edg_wll_SetError(context, ret = errno, "fsync()");
		goto out1;
	} 

	ret = 0;
out1:
	/* close and unlock */
	fclose(outfile); 
out:
	if(ret) edg_wll_UpdateError(context, ret, "notif_save_to_file()");
	return(ret);
}


static 
ssize_t 
socket_write_full(edg_wll_Context  context,
		  int              sock,
		  void            *buf,
		  size_t           bufsize,
		  struct timeval  *timeout,
		  ssize_t         *total)
{
	int ret = 0;
        ssize_t	len;

        *total = 0;
        while (bufsize > 0) {

		fd_set	fds;
		struct timeval	to,before,after;
		
		if (timeout) {
			memcpy(&to, timeout, sizeof(to));
			gettimeofday(&before, NULL);
		}

		len = write(sock, buf, bufsize);
		while (len <= 0) {
			FD_ZERO(&fds);
			FD_SET(sock, &fds);
			if (select(sock+1, &fds, NULL, NULL, timeout ? &to : NULL) < 0) {
				edg_wll_SetError(context, ret = errno, "select()");
				goto out;
			}
			len = write(sock, buf, bufsize);
		}
		if (timeout) {
			gettimeofday(&after,NULL);
			tv_sub(after, before);
			tv_sub(*timeout,after);
			if (timeout->tv_sec < 0) {
				timeout->tv_sec = 0;
				timeout->tv_usec = 0;
			}
		}
		
                if (len < 0) {
			edg_wll_SetError(context, ret = errno, "write()");
			goto out;
		}

		bufsize -= len;
		buf += len;
                *total += len;
        }

	ret = 0;
out:
	if(ret) edg_wll_UpdateError(context, ret, "socket_write_full()");
        return ret;
}


static 
int
notif_send_socket(edg_wll_Context       context,
		  long                  filepos,
		  const char           *ulm_data)
{
	int ret;
	struct sockaddr_un saddr;
	int msg_sock, flags, count;
	struct timeval timeout;

	timeout.tv_sec = EDG_WLL_LOG_TIMEOUT_MAX;
        timeout.tv_usec = 0;	

	msg_sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if(msg_sock < 0) {
		edg_wll_SetError(context, ret = errno, "socket()");
		goto out;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	strcpy(saddr.sun_path, notif_ilog_socket_path);

	if ((flags = fcntl(msg_sock, F_GETFL, 0)) < 0 ||
	    fcntl(msg_sock, F_SETFL, flags | O_NONBLOCK) < 0) {
		edg_wll_SetError(context, ret = errno, "fcntl()");
		goto out1;
	}

	if(connect(msg_sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		if(errno != EISCONN) {
			edg_wll_SetError(context, ret = errno, "connect()");
			goto out1;
		}
	}

	if (socket_write_full(context, msg_sock, &filepos, sizeof(filepos), &timeout, &count) < 0) {
		ret = errno; 
		goto out1;
	}

	if (socket_write_full(context, msg_sock, (void*)ulm_data, strlen(ulm_data), &timeout, &count) < 0) {
		ret = errno; 
		goto out1;
	}

	ret = 0;

out1:
	close(msg_sock);
out:
	if(ret) edg_wll_UpdateError(context, ret, "notif_send_socket()");
	return(ret);
}


int
edg_wll_NotifSend(edg_wll_Context       context,
	          edg_wll_NotifId       reg_id,
		  const char           *host,
                  int                   port,
		  const char           *owner,
                  const char           *notif_data)
{
	int ret;
	long filepos;
	char *ulm_data, *reg_id_s, *event_file;

	if((ret=notif_create_ulm(context, 
				 reg_id, 
				 host, 
				 port, 
				 owner, 
				 notif_data,
				 &ulm_data,
				 &reg_id_s))) {
		goto out;
	}

	asprintf(&event_file, "%s.%s", FILE_PREFIX, reg_id_s);
	if(event_file == NULL) {
		edg_wll_SetError(context, ret=ENOMEM, "asprintf()");
		goto out;
	}

	if((ret=notif_save_to_file(context,
				   event_file,
				   ulm_data,
				   &filepos))) {
		goto out;
	}

	if((ret=notif_send_socket(context,
				  filepos,
				  ulm_data))) {
		goto out;
	}
	ret = 0;

out:
	if(ulm_data) free(ulm_data);
	if(reg_id_s) free(reg_id_s);
	if(ret) edg_wll_UpdateError(context, ret, "edg_wll_NotifSend()");
	return(ret);
}


int
edg_wll_NotifJobStatus(edg_wll_Context	context,
		       edg_wll_NotifId	reg_id,
		       const char      *host,
                       int              port,
		       const char      *owner,
		       const edg_wll_JobStat notif_job_stat)
{
	int ret=0;
	char   *xml_data, *xml_esc_data=NULL;

	if(edg_wll_JobStatusToXML(context, notif_job_stat, &xml_data)) 
		goto out;
	
	if((xml_esc_data = edg_wll_EscapeXML(xml_data)) == NULL) {
		edg_wll_SetError(context, ret=ENOMEM, "edg_wll_EscapeXML()");
		goto out;
	}

	ret=edg_wll_NotifSend(context, reg_id, host, port, owner, xml_esc_data);

out:
	if(xml_data) free(xml_data);
	if(xml_esc_data) free(xml_esc_data);
	if(ret) edg_wll_UpdateError(context, ret, "edg_wll_NotifJobStatus()");
	return(ret);
}


int 
edg_wll_NotifChangeDestination(edg_wll_Context context,
                               edg_wll_NotifId reg_id,
                               const char      *host,
                               int             port)
{
	return(edg_wll_NotifSend(context, reg_id, host, port, "", ""));
}


int
edg_wll_NotifCancelRegId(edg_wll_Context context,
			 edg_wll_NotifId reg_id)
{
	return(edg_wll_NotifSend(context, reg_id, NULL, 0, "", ""));
}

