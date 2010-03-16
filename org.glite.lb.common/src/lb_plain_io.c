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

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#ifndef INFTIM
#define INFTIM (-1)
#endif

#include "lb_plain_io.h"

#define tv_sub(a,b) {\
	(a).tv_usec -= (b).tv_usec;\
	(a).tv_sec -= (b).tv_sec;\
	if ((a).tv_usec < 0) {\
		(a).tv_sec--;\
		(a).tv_usec += 1000000;\
	}\
}

#define bufshift(conn, shift) { \
	memmove((conn)->buf, (conn)->buf+(shift), (conn)->bufUse-(shift)); \
	(conn)->bufUse -= (shift); \
}

int edg_wll_plain_connect(
	char const				   *hostname,
	int							port,
	struct timeval			   *to,
	edg_wll_PlainConnection	   *conn)
{
	return 0;
}

int edg_wll_plain_accept(
	int							sock,
	edg_wll_PlainConnection	   *conn)
{
	/* Do not free the buffer here - just reuse the memmory
	 */
	conn->bufUse = 0;
	conn->sock = sock;
	return 0;
}

int edg_wll_plain_close(edg_wll_PlainConnection *conn)
{
	errno = 0;
	if ( conn->buf ) free(conn->buf);
	if ( conn->sock > -1 ) close(conn->sock);
	memset(conn, 0, sizeof(*conn));
	conn->sock = -1;

	return errno? -1: 0;
}

int edg_wll_plain_read(
	edg_wll_PlainConnection	   *conn,
	void				   *outbuf,
	size_t					outbufsz,
	struct timeval		   *to)
{
	int				ct, toread = 0;
	struct pollfd			pollfds[1];
	int				polltime = 0;
	struct timeval	timeout, before, after;


	if ( conn->bufSize == 0 ) {
		if ( !(conn->buf = malloc(BUFSIZ)) ) return -1;
		conn->bufSize = BUFSIZ;
		conn->bufUse = 0;
	}

	if ( to ) {
		memcpy(&timeout, to, sizeof(timeout));
		gettimeofday(&before, NULL);
	}

	errno = 0;

	if ( conn->bufUse > 0 ) goto cleanup;

	toread = 0;
	do {
		pollfds[0].fd = conn->sock;
		pollfds[0].events = POLLIN;
		polltime = to ? (timeout.tv_sec*1000+timeout.tv_usec/1000) : INFTIM;
		switch (poll(pollfds, 1, polltime)) {
			case 0: errno = ETIMEDOUT; goto cleanup; break;
			case -1: goto cleanup; break;
			default: if (!(pollfds[0].revents & POLLIN)) {
					errno = EIO;
					goto cleanup; break;
				}
		}

		if ( conn->bufUse == conn->bufSize ) {
			char *tmp = realloc(conn->buf, conn->bufSize+BUFSIZ);
			if ( !tmp ) return -1;
			conn->buf = tmp;
			conn->bufSize += BUFSIZ;
		}
		toread = conn->bufSize - conn->bufUse;
		if ( (ct = read(conn->sock, conn->buf+conn->bufUse, toread)) < 0 ) {
			if ( errno == EINTR ) continue;
			goto cleanup;
		}

		if ( ct == 0 && conn->bufUse == 0 && errno == 0 ) {
			errno = ENOTCONN;
			goto cleanup;
		}

		conn->bufUse += ct;
	} while ( ct == toread );


cleanup:
	if ( to ) {
		gettimeofday(&after, NULL);
		tv_sub(after, before);
		tv_sub(*to, after);
		if ( to->tv_sec < 0 ) to->tv_sec = to->tv_usec = 0;
	}

	if ( errno == ECONNRESET) errno = ENOTCONN;
	if ( errno ) return -1;

	if ( conn->bufUse > 0 ) {
		size_t len = (conn->bufUse < outbufsz) ? conn->bufUse : outbufsz;
		memcpy(outbuf, conn->buf, len);
/* no use	outbufsz = len;	*/
		bufshift(conn, len);
		return len;
	}

	return 0;
}


int edg_wll_plain_read_full(
	edg_wll_PlainConnection	   *conn,
	void					   *outbuf,
	size_t						outbufsz,
	struct timeval			   *to)
{
	size_t		total = 0;


	if ( conn->bufUse > 0 ) {
		size_t len = (conn->bufUse < outbufsz) ? conn->bufUse : outbufsz;
		memcpy(outbuf, conn->buf, len);
/* fuj		outbufsz = len; */
		bufshift(conn, len);
		total += len;
	}

	while ( total < outbufsz ) {
		int ct = edg_wll_plain_read(conn, outbuf+total, outbufsz-total, to);
		if ( ct < 0) return ct;
		total += ct;
	}

	return total;
}

int edg_wll_plain_write_full(
	edg_wll_PlainConnection	   *conn,
	const void				   *buf,
	size_t						bufsz,
	struct timeval			   *to)
{
	size_t			written = 0;
	int				ct = -1;
	struct pollfd			pollfds[1];
	int				polltime = 0;
	struct timeval	timeout, before, after;
	struct sigaction        sa,osa;

	memset(&sa,0,sizeof(sa)); assert(sa.sa_handler == NULL);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE,&sa,&osa);

	if ( to ) {
		memcpy(&timeout, to, sizeof(timeout));
		gettimeofday(&before, NULL);
	}

	errno = 0;
	while ( written < bufsz ) {

		pollfds[0].fd = conn->sock;
		pollfds[0].events = POLLOUT;
		polltime = to ? (timeout.tv_sec*1000+timeout.tv_usec/1000) : INFTIM;

		switch (poll(pollfds, 1, polltime)) {
			case 0: errno = ETIMEDOUT; goto end; break;
			case -1: goto end; break;
			default: if (!(pollfds[0].revents & POLLOUT)) {
					errno = ENOTCONN;
					goto end; break;
				}
		}
		if ( (ct=write(conn->sock, ((char*)buf)+written, bufsz-written)) < 0 ) {
			if ( errno == EINTR ) { errno = 0; continue; }
			else goto end;
		}
		written += ct;
	}

end:
	if ( to ) {
		gettimeofday(&after, NULL);
		tv_sub(after, before);
		tv_sub(*to, after);
		if (to->tv_sec < 0) to->tv_sec = to->tv_usec = 0;
	}

	sigaction(SIGPIPE,&osa,NULL);
	if (errno == EPIPE) errno = ENOTCONN;
	return (errno)? -1: written;
}
