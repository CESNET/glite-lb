/**
 * il_lbproxy.c
 *   - implementation of IL API calls for LB proxy
 *
 */
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "glite/lb/context-int.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/escape.h"
#include "glite/lb/log_proto.h"
#include "glite/lb/lb_plain_io.h"

#include "il_lbproxy.h"
#include "lb_xml_parse.h"



#define FCNTL_ATTEMPTS		5
#define FCNTL_TIMEOUT		1
#define FILE_PREFIX             "/tmp/lbproxy_events"
#define DEFAULT_SOCKET          "/tmp/lbproxy_interlogger.sock"

char *lbproxy_ilog_socket_path = DEFAULT_SOCKET;
char *lbproxy_ilog_file_prefix = FILE_PREFIX;

#define tv_sub(a,b) {\
	(a).tv_usec -= (b).tv_usec;\
	(a).tv_sec -= (b).tv_sec;\
	if ((a).tv_usec < 0) {\
		(a).tv_sec--;\
		(a).tv_usec += 1000000;\
	}\
}

static int event_save_to_file_proxy(
	edg_wll_Context     ctx,
	const char         *event_file,
	const char         *ulm_data,
	long               *filepos)
{
	FILE		   *outfile;
	struct flock	filelock;
	int				filedesc,
					i, filelock_status=-1;


	for( i = 0; i < FCNTL_ATTEMPTS; i++ ) {
		/* fopen and properly handle the filelock */
		if ( (outfile = fopen(event_file, "a")) == NULL ) {
			edg_wll_SetError(ctx, errno, "fopen()");
			goto out;
		}
		if ( (filedesc = fileno(outfile)) == -1 ) {
			edg_wll_SetError(ctx, errno, "fileno()");
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
				edg_wll_SetError(ctx, errno, "fcntl()");
				goto out1;
			}
		} else {
			/* lock acquired, break out of the loop */
			break;
		}
	}
	if (fseek(outfile, 0, SEEK_END) == -1) { edg_wll_SetError(ctx, errno, "fseek()"); goto out1; }
	if ((*filepos=ftell(outfile)) == -1) { edg_wll_SetError(ctx, errno, "ftell()"); goto out1; }
	if (fputs(ulm_data, outfile) == EOF) { edg_wll_SetError(ctx, errno, "fputs()"); goto out1; }       
	if (fflush(outfile) == EOF) { edg_wll_SetError(ctx, errno, "fflush()"); goto out1; }
	if (fsync(filedesc) < 0) { edg_wll_SetError(ctx, errno, "fsync()"); goto out1; } 

out1:
	fclose(outfile); 

out:
	return edg_wll_Error(ctx, NULL, NULL)?
			edg_wll_UpdateError(ctx, 0, "event_save_to_file_proxy()"): 0;
}


static int event_send_socket_proxy(
	edg_wll_Context		ctx,
	long				filepos,
	const char		   *ulm_data)
{
	edg_wll_PlainConnection	conn;
	struct sockaddr_un		saddr;
	int						flags;
	struct timeval			timeout;


	timeout.tv_sec = EDG_WLL_LOG_TIMEOUT_MAX;
	timeout.tv_usec = 0;	

	if ( (conn.sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0 )
		edg_wll_SetError(ctx, errno, "socket()");

	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	strcpy(saddr.sun_path, lbproxy_ilog_socket_path);

	if (   (flags = fcntl(conn.sock, F_GETFL, 0)) < 0
		|| fcntl(conn.sock, F_SETFL, flags | O_NONBLOCK) < 0) {
		edg_wll_SetError(ctx, errno, "fcntl()");
		goto out;
	}

	if ( connect(conn.sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		if(errno != EISCONN) { edg_wll_SetError(ctx, errno, "connect()"); goto out; }
	}

	if (   edg_wll_plain_write_full(&conn, &filepos, sizeof(filepos), &timeout) < 0
		|| edg_wll_plain_write_full(&conn, (void*)ulm_data, strlen(ulm_data), &timeout) < 0 ) {
		edg_wll_SetError(ctx, errno, "edg_wll_plain_write_full()");
		goto out;
	}

out:
	close(conn.sock);
	return edg_wll_Error(ctx, NULL, NULL)?
			edg_wll_UpdateError(ctx, 0, "event_send_socket_proxy()"): 0;
}


int edg_wll_EventSendProxy(edg_wll_Context ctx, const edg_wlc_JobId jobid, const char *event)
{
	long	filepos;
	char   *jobid_s, *event_file = NULL;


	edg_wll_ResetError(ctx);
	jobid_s = edg_wlc_JobIdGetUnique(jobid);
	if ( !jobid_s ) { edg_wll_SetError(ctx, ENOMEM, "edg_wlc_JobIdGetUnique()"); goto out; }

	asprintf(&event_file, "%s.%s", lbproxy_ilog_file_prefix, jobid_s);
	if ( !event_file ) { edg_wll_SetError(ctx, ENOMEM, "asprintf()"); goto out; }

	if (   event_save_to_file_proxy(ctx, event_file, event, &filepos)
		|| event_send_socket_proxy(ctx, filepos, event) ) goto out;

out:
	if ( jobid_s ) free(jobid_s);
	if ( event_file ) free(event_file);

	return  edg_wll_Error(ctx, NULL, NULL)?  edg_wll_UpdateError(ctx, 0, "edg_wll_EventSendProxy()"): 0;
}
