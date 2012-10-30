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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pwd.h>
#include <sys/un.h>
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <grp.h>

#include "context-int.h"

#define tv_sub(a, b) {\
	(a).tv_usec -= (b).tv_usec;\
	(a).tv_sec -= (b).tv_sec;\
	if ((a).tv_usec < 0) {\
		(a).tv_sec--;\
		(a).tv_usec += 1000000;\
	}\
}


/*!
 * Write to socket
 * Needn't write entire buffer. Timeout is applicable only for non-blocking
 * connections 
 * \param sock IN: connection to work with
 * \param buf IN: buffer
 * \param bufsize IN: max size to write
 * \param timeout INOUT: max time allowed for operation, remaining time on return
 * \retval bytes written (>0) on success
 * \retval -1 on write error
 */
static ssize_t
edg_wll_socket_write(
	int				sock,
	const void	   *buf,
	size_t			bufsize,
	struct timeval *timeout)
{ 
	ssize_t			len = 0;
	fd_set			fds;
	struct timeval	to, before, after;


	if ( timeout ) {
		memcpy(&to, timeout, sizeof to);
		gettimeofday(&before, NULL);
	}
	len = write(sock, buf, bufsize);
	if ( len <= 0  && errno == EAGAIN ) {
		FD_ZERO(&fds);
		FD_SET(sock,&fds);
		if ( select(sock+1, NULL, &fds, NULL, timeout? &to: NULL) < 0 ) {
			len = -1;
		} else {
			len = write(sock, buf, bufsize);
		}
	}
	if ( timeout ) {
		gettimeofday(&after, NULL);
		tv_sub(after, before);
		tv_sub(*timeout, after);
		if ( timeout->tv_sec < 0 ) {
			timeout->tv_sec = 0;
			timeout->tv_usec = 0;
		}
	}

	return len;
}

/*!
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
 */
static ssize_t
edg_wll_socket_write_full(
	int				sock,
	void		   *buf,
	size_t			bufsize,
	struct timeval *timeout,
	ssize_t		   *total)
{
	ssize_t	len;
	*total = 0;

	while ( *total < bufsize ) {
		len = edg_wll_socket_write(sock, buf+*total, bufsize-*total, timeout);
		if (len < 0) return len;
		*total += len;
	}

	return 0;
}

/*
 * edg_wll_log_event_write - write event to the local file
 *
 * Returns: 0 if done properly or errno
 *
 */
int edg_wll_log_event_write(
	edg_wll_Context		ctx,
	const char		   *event_file, 
	const char         *msg,
	unsigned int		fcntl_attempts,
	unsigned int		fcntl_timeout,
	long               *filepos)
{
	FILE		   *outfile;
	struct flock	filelock;
	int				filedesc,
					i, filelock_status=-1;
	struct stat statbuf;
	char *group_name;

try_again:
	if ( (outfile = fopen(event_file, "a")) == NULL ) {
		edg_wll_SetError(ctx, errno, "fopen()");
		goto event_write_end;
	}

	if ( (filedesc = fileno(outfile)) == -1 ) {
		edg_wll_SetError(ctx, errno, "fileno()");
		goto cleanup;
	}

	for ( i = 0; i < fcntl_attempts; i++ ) {
		filelock.l_type = F_WRLCK;
		filelock.l_whence = SEEK_SET;
		filelock.l_start = 0;
		filelock.l_len = 0;

		if ( (filelock_status = fcntl(filedesc, F_SETLK, &filelock) < 0) ) {
			switch(errno) {
			case EAGAIN:
			case EACCES:
			case EINTR:
				if ((i+1) < fcntl_attempts) sleep(fcntl_timeout);
				break;
			default:
				edg_wll_SetError(ctx, errno, "fcntl()");
				goto cleanup;
			}
		} else {
			/* check that the file still exists */
			if(stat(event_file, &statbuf)) {
				if(errno == ENOENT) {
					/* not there anymore - reopen it */
					fclose(outfile);
					goto try_again;
				} else {
					/* could not stat the output file */
					edg_wll_SetError(ctx, errno, "stat()");
					goto cleanup;
				}
			} else {
				/* file exists and is locked */
				break;
			}
		}
	}
	if (i == fcntl_attempts) {
		edg_wll_SetError(ctx, ETIMEDOUT, "timed out trying to lock event file");
		goto cleanup;
	}

	/* make the file writable for given group, if specified */
	if(NULL != (group_name = getenv("GLITE_GROUP"))) {
		struct group *glite_group = getgrnam(group_name);

		if(NULL != glite_group) {
			/* errors are ignored */
			fchown(filedesc, -1, glite_group->gr_gid);
			fchmod(filedesc, 0660);
		}
	}


	if ( fseek(outfile, 0, SEEK_END) == -1 ) {	
		edg_wll_SetError(ctx, errno, "fseek()");
		goto cleanup;
	}
	if ( (*filepos = ftell(outfile)) == -1 ) {
		edg_wll_SetError(ctx, errno, "ftell()");
		goto cleanup;
	}
	i = strlen(msg);
	if( i != fwrite(msg, sizeof(char), i, outfile)) {
		edg_wll_SetError(ctx, errno, "fwrite()");
		/* partially written message may corrupt event file */
		ftruncate(filedesc, *filepos);
		goto cleanup;
	}
	if ( fflush(outfile) == EOF ) {
		edg_wll_SetError(ctx, errno, "fflush()");
		/* partially written message may corrupt event file */
		ftruncate(filedesc, *filepos);
		goto cleanup;
	}
	if ( fsync(filedesc) < 0 ) {
		edg_wll_SetError(ctx, errno, "fsync()");
		/* partially written message may corrupt event file */
		ftruncate(filedesc, *filepos);
		goto cleanup;
	}


cleanup:
	fclose(outfile); 

event_write_end:
	return edg_wll_Error(ctx, NULL, NULL)? edg_wll_Error(ctx, NULL, NULL): 0;

}

/*
 * edg_wll_log_event_send - send event to the socket
 *
 * Returns: 0 if done properly or errno
 *
 */
int edg_wll_log_event_send(
	edg_wll_Context		ctx,
	const char         *socket_path,
	long				filepos,
	const char         *msg,
	int					msg_size,
	int					conn_attempts,
	struct timeval	   *timeout)
{
	struct sockaddr_un	saddr;
	int					msg_sock,
						flags,
						conn_timeout,
						i;
        ssize_t	count = 0;


	if ( (msg_sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0 ) {
		edg_wll_SetError(ctx, errno, "socket()"); 
		goto event_send_end;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	strcpy(saddr.sun_path, socket_path);

	if (   (flags = fcntl(msg_sock, F_GETFL, 0)) < 0
		|| fcntl(msg_sock, F_SETFL, flags | O_NONBLOCK) < 0 ) {
		edg_wll_SetError(ctx, errno, "fcntl()"); 
		goto cleanup;
	}

	conn_timeout = floor(timeout->tv_sec/(conn_attempts + 1));
	for ( i = 0; i < conn_attempts; i++) {
		if ( connect(msg_sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0 ) {
			if ( errno == EISCONN ) break;
			else if ((errno == EAGAIN) || (errno == ETIMEDOUT)) {
				sleep(conn_timeout);
				timeout->tv_sec -= conn_timeout;
				continue;
			} else {
				edg_wll_SetError(ctx, errno, "Can't connect to the interlogger"); 
				goto cleanup;
			}
		} else break;
	}

	if ( edg_wll_socket_write_full(msg_sock, &filepos, sizeof(filepos), timeout, &count) < 0 ) {
		edg_wll_SetError(ctx, errno, "edg_wll_socket_write_full()"); 
		goto cleanup;
	}

	if ( edg_wll_socket_write_full(msg_sock, (void *)msg, msg_size, timeout, &count) < 0 ) {
		edg_wll_SetError(ctx, errno, "edg_wll_socket_write_full()"); 
		goto cleanup;
	}

cleanup:
	close(msg_sock); 

event_send_end:
	return edg_wll_Error(ctx, NULL, NULL)? edg_wll_Error(ctx, NULL, NULL): 0;
}
