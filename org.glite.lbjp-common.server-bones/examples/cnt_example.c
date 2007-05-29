#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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


int main(int argc, char **argv)
{
	struct sockaddr_in	addr;
	char				buff[512],
					   *me;
	int					opt,
						sock,
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

	bzero((char *) &addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(port);
	if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		exit(1);
	}
	if ( connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0 )
	{
		perror("connect");
		exit(1);
	}
	n = strlen(msg? msg: DEF_MSG);
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
			return 1;
		}
		writen(0, buff, n);
	}
	close(sock);

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
