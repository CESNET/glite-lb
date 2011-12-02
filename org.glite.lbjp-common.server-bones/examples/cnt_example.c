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
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#ifndef dprintf
#define dprintf(x) { if (debug) printf x; fflush(stdout); }
#endif

#define DEF_MSG			"Test message\n"
#define DEF_PORT		9999

static struct option opts[] = {
	{ "help",	no_argument,            NULL, 'h'},
	{ "debug",	no_argument,            NULL, 'd'},
	{ "msg",	required_argument,      NULL, 'm'},
	{ "port",	required_argument,      NULL, 'p'},
	{ "repeat",	required_argument,      NULL, 'r'},
};

int debug  = 0;
int	port = DEF_PORT;
char *msg = NULL;

static int writen(int fd, char *ptr, int nbytes);
static int readln(int fd, char *out);

static void usage(char *me)
{
	fprintf(stderr,
			"usage: %s [option]\n"
			"    -h, --help                 print this screen\n"
			"    -d, --debug                prints debug messages\n"
			"    -m, --msg <text>           message to send\n"
			"    -p, --port <num>           service port\n", me);
}


static int try_connect(char *addr, char *port) {
	struct addrinfo		hints;
	struct addrinfo		*ai_all = NULL, *ai;
	int			gaie, sock = -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICHOST | AI_NUMERICSERV;
	gaie = getaddrinfo(addr, port, &hints, &ai_all);
	if (gaie != 0 || !ai_all) return -1;

	for (ai = ai_all; ai; ai = ai->ai_next) {
		sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sock == -1) continue;
		if (connect(sock, ai->ai_addr, ai->ai_addrlen) == 0) {
			break;
		} else {
			close(sock);
			sock = -1;
		}
	}
	freeaddrinfo(ai_all);

	return sock;
}


int main(int argc, char **argv)
{
	char 			portstr[10];
	char			buff[512],
				*me;
	int			opt,
				sock = -1,
				fd,
				n;
	int	repeat = 1;

	me = strrchr(argv[0], '/');
	if ( me ) me++; else me = argv[0];
	while ( (opt = getopt_long(argc, argv,"p:m:hdr:", opts, NULL)) != EOF )
	{
		switch ( opt )
		{
		case 'm':
			msg = strdup(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'd': debug = 1; break;
		case 'r': repeat = atoi(optarg); break;
		case 'h': usage(me); return 0;
		case '?': usage(me); return 1;
		}
	}

	snprintf(portstr, sizeof(portstr), "%d", port);

	if ((sock = try_connect("127.0.0.1", portstr)) == -1 &&
	    (sock = try_connect("::1", portstr)) == -1) {
		fprintf(stderr, "can't connect\n");
		exit(1);
	}

	n = strlen(msg? msg: DEF_MSG);
	fd = fileno(stdout);
	for (;repeat; repeat--) {
		if ( writen(sock, msg? msg: DEF_MSG, n) != n )
		{
			dprintf(("error writing message\n"));
			exit(1);
		}
		printf("reply: "); fflush(stdout);
		n = readln(sock, buff);
		if ( n < 0 )
		{
			perror("read() reply error");
			free(msg);
			return 1;
		}
		writen(fd, buff, n);
	}
	close(sock);

	free(msg);
	return 0;
}

int writen(int fd, char *ptr, int nbytes)
{
	int		nleft, nwritten;

	nleft = nbytes;
	dprintf(("start writing %d bytes\n", nbytes));
	while ( nleft > 0 )
	{
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

int readln(int fd, char *out)
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

				dprintf(("using buffer data\n"));
				memcpy(out, buffer, linesz);
				if ( endl+1 != buffer_end )
					memmove(buffer, endl+1, buffer_end-endl-1);
				buffer_end -= linesz;
				return linesz;
			}
		}
		dprintf(("reading...\n"));
		n = read(fd, buffer_end, BUFFER_SZ-(buffer_end-buffer));
		if ( n < 0 ) {
			if ( errno == EAGAIN ) continue;
			dprintf(("reading error\n"));
			return n;
		}
		else if ( n == 0 ) {
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
