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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "glite/lbu/log.h"
#include "srvbones.h"

#ifndef dprintf
#define dprintf(x) { if (debug) printf x; fflush(stdout); }
#endif

#define sizofa(a)	(sizeof(a)/sizeof((a)[0]))

int debug  = 1;

static int writen(int fd, char *ptr, int nbytes);
static int readln(int fd, char *out, int nbytes);

static int new_conn(int, struct timeval *, void *);
static int reject(int);
static int disconnect(int, struct timeval *, void *);

static int echo(int, struct timeval *, void *);
static int upper_echo(int, struct timeval *, void *);

#define ECHO_PORT		"9999"
#define UPPER_ECHO_PORT		"9998"

#define SRV_ECHO			0
#define SRV_UPPER_ECHO		1

static struct glite_srvbones_service service_table[] = {
	{ "Echo Service",		-1, new_conn, echo, reject, disconnect },
	{ "Upper Echo Service",	-1, new_conn, upper_echo, reject, disconnect }
};

int main(void)
{
	if (glite_common_log_init()) {
		fprintf(stderr,"glite_common_log_init() failed, exiting.");
		exit(1);
	}

	if (glite_srvbones_daemon_listen(NULL, ECHO_PORT, &service_table[SRV_ECHO].conn) != 0)
		exit(1);

	if (glite_srvbones_daemon_listen(NULL, UPPER_ECHO_PORT, &service_table[SRV_UPPER_ECHO].conn) != 0) {
		close(service_table[SRV_ECHO].conn);
		exit(1);
	}

	setpgid(0, getpid());

	glite_srvbones_set_param(GLITE_SBPARAM_SLAVES_COUNT, 1);
	glite_srvbones_run(NULL, service_table, sizofa(service_table), 1);

	return 0;
}

int upper_echo(int fd, struct timeval *to, void *data)
{
	int		n, i;
	char	line[80];

	n = readln(fd, line, 80);
	if ( n < 0 )
	{
		perror("read() message");
		return n;
	}
	else if ( n == 0 )
		return ENOTCONN;

	for ( i = 0; i < n; i++ )
		line[i] = toupper(line[i]);

	if ( writen(fd, line, n) != n )
	{
		perror("write() message back");
		return -1;
	}

	return 0;
}

int echo(int fd, struct timeval *to, void *data)
{
	int		n;
	char	line[80];

	n = readln(fd, line, 80);
	dprintf(("%d bytes read\n", n));
	if ( n < 0 )
	{
		perror("read() message");
		return n;
	}
	else if ( n == 0 )
		return ENOTCONN;

	if ( writen(fd, line, n) != n )
	{
		perror("write() message back");
		return -1;
	}

	return 0;
}

int new_conn(int conn, struct timeval *to, void *cdata)
{
	dprintf(("srv-bones example: new_conn handler\n"));
	return 0;
}

int reject(int conn)
{
	dprintf(("srv-bones example: reject handler\n"));
	return 0;
}

int disconnect(int conn, struct timeval *to, void *cdata)
{
	dprintf(("srv-bones example: disconnect handler\n"));
	return 0;
}

int writen(int fd, char *ptr, int nbytes)
{
	int		nleft, nwritten;

	nleft = nbytes;
	dprintf(("start writing %d bytes\n", nbytes));
	while ( nleft > 0 ) {
		nwritten = write(fd, ptr, nleft);
		dprintf(("written %d bytes", nwritten));
		if ( nwritten <= 0 )
			return (nwritten);

		nleft -= nwritten;
		ptr += nwritten;
		dprintf((" (left %d bytes)\n", nleft));
	}

	dprintf(("written %d bytes (return: %d)\n", nwritten, nbytes - nleft));
	return (nbytes - nleft);
}

#define BUFFER_SZ			512

int readln(int fd, char *out, int nbytes)
{
	static char		buffer[BUFFER_SZ];
	static char	   *buffer_end = buffer;
	int				n;

	dprintf(("reading line\n"));
	while ( 1 ) {
		if ( buffer_end - buffer ) {
			/*	buffer contains data
			 */
			char	   *endl;

			dprintf(("nonempty buffer\n"));
			if ( (endl = memchr(buffer, '\n', buffer_end-buffer)) ) {
				int		linesz = endl-buffer+1;

				memcpy(out, buffer, linesz);
				if ( endl+1 != buffer_end ) memmove(buffer, endl+1, buffer_end-endl-1);
				buffer_end -= linesz;
				return linesz;
			}
		}

		dprintf(("reding...\n"));
		n = read(fd, buffer_end, BUFFER_SZ-(buffer_end-buffer));
		if ( n < 0 ) {
			if ( errno == EAGAIN ) n = 0;
			else return n;
		}
		if ( n == 0 ) {
			int		ret = buffer_end-buffer;
			dprintf(("end of reading - returning %d bytes\n", ret));
			memcpy(out, buffer, ret);
			buffer_end = buffer;
			return ret;
		}
		dprintf(("read %d bytes\n", n));

		buffer_end += n;
	}

	return 0;
}
