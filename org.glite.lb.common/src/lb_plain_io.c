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


int edg_wll_plain_write_full(
	int				conn,
	const void	   *buf,
	size_t			bufsz,
	struct timeval *to)
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
		FD_SET(conn, &fds);

		switch ( select(conn+1, NULL, &fds, NULL, to ? &timeout : NULL) ) {
			case 0: errno = ETIMEDOUT; goto end; break;
			case -1: goto end; break;
		}
		if ( (ct = write(conn, ((char*)buf)+written, bufsz-written)) < 0 ) {
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


int edg_wll_plain_read_full(
	int					conn,
	void			  **out,
	size_t				outsz,
	struct timeval	   *to)
{
	ssize_t			ct, sz, total = 0;
	char			buf[4098],
				   *tmp = NULL;
	fd_set			fds;
	struct timeval	timeout, before, after;


	if ( to ) {
		memcpy(&timeout, to, sizeof(timeout));
		gettimeofday(&before, NULL);
	}

	errno = 0;
	sz = sizeof(buf);
	do {
		FD_ZERO(&fds);
		FD_SET(conn, &fds);
		switch (select(conn+1, &fds, NULL, NULL, to ? &timeout : NULL)) {
		case 0: errno = ETIMEDOUT; goto cleanup; break;
		case -1: goto cleanup; break;
		}

		if ( sz > outsz-total ) sz = outsz-total;
		if ( (ct = read(conn, buf, sz)) < 0 ) {
			if ( errno == EINTR ) continue;
			goto cleanup;
		}

		if ( ct > 0 ) {
			char *t = realloc(tmp, total+ct);

			if ( !t ) goto cleanup;
			tmp = t;
			memcpy(tmp+total, buf, ct);
			total += ct;
		}
		else if ( total == 0 && errno == 0 ) { errno = ENOTCONN; goto cleanup; }
	} while ( total < outsz );


cleanup:
	if ( to ) {
		gettimeofday(&after, NULL);
		tv_sub(after, before);
		tv_sub(*to, after);
		if ( to->tv_sec < 0 ) to->tv_sec = to->tv_usec = 0;
	}

	if ( errno ) { free(tmp); return -1; }

	*out = tmp;
	return total;
}

int edg_wll_plain_read_fullbuf(
	int					conn,
	void			   *outbuf,
	size_t				outbufsz,
	struct timeval	   *to)
{
	ssize_t			ct, sz, total = 0;
	char			buf[4098];
	fd_set			fds;
	struct timeval	timeout, before, after;


	if ( to ) {
		memcpy(&timeout, to, sizeof(timeout));
		gettimeofday(&before, NULL);
	}

	errno = 0;
	sz = sizeof(buf);
	do {
		FD_ZERO(&fds);
		FD_SET(conn, &fds);
		switch (select(conn+1, &fds, NULL, NULL, to ? &timeout : NULL)) {
		case 0: errno = ETIMEDOUT; goto cleanup; break;
		case -1: goto cleanup; break;
		}

		if ( sz > outbufsz-total ) sz = outbufsz-total;
		if ( (ct = read(conn, buf, sz)) < 0 ) {
			if ( errno == EINTR ) continue;
			goto cleanup;
		}

		if ( ct > 0 ) {
			memcpy(outbuf+total, buf, ct);
			total += ct;
		}
		else if ( total == 0 && errno == 0 ) { errno = ENOTCONN; goto cleanup; }
	} while ( total < outbufsz );

cleanup:
	if ( to ) {
		gettimeofday(&after, NULL);
		tv_sub(after, before);
		tv_sub(*to, after);
		if ( to->tv_sec < 0 ) to->tv_sec = to->tv_usec = 0;
	}

	return errno? -1: total;
}
