#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>

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
	memmove((conn)->buffer, (conn)->buffer+(shift), (conn)->bufuse-(shift)); \
	(conn)->bufuse -= (shift); \
}

int edg_wll_plain_connect(
	char const		   *hostname,
	int					port,
	struct timeval	   *to,
	edg_wll_Connection *conn)
{
	return 0;
}

int edg_wll_plain_accept(
	int					sock,
	edg_wll_Connection *conn)
{
	struct sockaddr_in	a;
	int					alen = sizeof(a);

	/* Do not free the buffer here - just reuse the memmory
	 */
	conn->bufuse = 0;
	/*
	if ( (conn->sock = accept(sock, (struct sockaddr *)&a, &alen)) )
		return -1;
	*/
	return 0;
}

int edg_wll_plain_read(
	edg_wll_Connection	   *conn,
	void				   *outbuf,
	size_t					outbufsz,
	struct timeval		   *to)
{
	size_t			ct, toread = 0;
	fd_set			fds;
	struct timeval	timeout, before, after;


	if ( conn->bufsz == 0 ) {
		if ( !(conn->buffer = malloc(BUFSIZ)) ) return -1;
		conn->bufsz = BUFSIZ;
		conn->bufuse = 0;
	}

	if ( to ) {
		memcpy(&timeout, to, sizeof(timeout));
		gettimeofday(&before, NULL);
	}

	errno = 0;

	if ( conn->bufuse > 0 ) goto cleanup;

	toread = 0;
	do {
		FD_ZERO(&fds);
		FD_SET(conn->sock, &fds);
		switch (select(conn->sock+1, &fds, NULL, NULL, to ? &timeout : NULL)) {
		case 0: errno = ETIMEDOUT; goto cleanup; break;
		case -1: goto cleanup; break;
		}

		if ( conn->bufuse == conn->bufsz ) {
			char *tmp = realloc(conn->buffer, conn->bufsz+BUFSIZ);
			if ( !tmp ) return -1;
			conn->buffer = tmp;
			conn->bufsz += BUFSIZ;
		}
		toread = conn->bufsz - conn->bufuse;
		if ( (ct = read(conn->sock, conn->buffer+conn->bufuse, toread)) < 0 ) {
			if ( errno == EINTR ) continue;
			goto cleanup;
		}

		if ( ct == 0 && conn->bufuse == 0 && errno == 0 ) {
			errno = ENOTCONN;
			goto cleanup;
		}

		conn->bufuse += ct;
	} while ( ct == toread );


cleanup:
	if ( to ) {
		gettimeofday(&after, NULL);
		tv_sub(after, before);
		tv_sub(*to, after);
		if ( to->tv_sec < 0 ) to->tv_sec = to->tv_usec = 0;
	}

	if ( errno ) return -1;

	if ( conn->bufuse > 0 ) {
		size_t len = (conn->bufuse < outbufsz) ? conn->bufuse : outbufsz;
		memcpy(outbuf, conn->buffer, len);
		outbufsz = len;
		bufshift(conn, len);
		return len;
	}

	return 0;
}


int edg_wll_plain_read_full(
	edg_wll_Connection *conn,
	void			   *outbuf,
	size_t				outbufsz,
	struct timeval	   *to)
{
	size_t		total = 0;


	if ( conn->bufuse > 0 ) {
		size_t len = (conn->bufuse < outbufsz) ? conn->bufuse : outbufsz;
		memcpy(outbuf, conn->buffer, len);
		outbufsz = len;
		bufshift(conn, len);
		total += len;
	}

	while ( total < outbufsz ) {
		size_t ct = edg_wll_plain_read(conn, outbuf+total, outbufsz-total, to);
		if ( ct < 0) return ct;
		total += ct;
	}

	return total;
}

int edg_wll_plain_write_full(
	edg_wll_Connection *conn,
	const void		   *buf,
	size_t				bufsz,
	struct timeval	   *to)
{
	size_t			written = 0;
	ssize_t			ct = -1;
	fd_set			fds;
	struct timeval	timeout, before, after;


	if ( to ) {
		memcpy(&timeout, to, sizeof(timeout));
		gettimeofday(&before, NULL);
	}

	errno = 0;
	while ( written < bufsz ) {
		FD_ZERO(&fds);
		FD_SET(conn->sock, &fds);

		switch ( select(conn->sock+1, NULL, &fds, NULL, to? &timeout: NULL) ) {
			case 0: errno = ETIMEDOUT; goto end; break;
			case -1: goto end; break;
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

	return (errno)? -1: written;
}
