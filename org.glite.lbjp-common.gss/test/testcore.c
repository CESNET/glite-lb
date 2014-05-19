#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "glite_gss.h"
#include "testcore.h"


const struct option test_longopts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "proxy", required_argument, NULL, 'u' },
	{ "cert", required_argument, NULL, 'c' },
	{ "key", required_argument, NULL, 'k' },
	{ "user-cert", required_argument, NULL, 'C' },
	{ "user-key", required_argument, NULL, 'K' },
	{ "hostname", required_argument, NULL, 'H' },
	{ "timeout", required_argument, NULL, 't' },
	{ NULL, 0, NULL, 0 },
};

const char *test_getopt_string = "hc:k:C:K:H:t:";

const char *test_prog_name = NULL;

char *server_cert = NULL;
char *server_key = NULL;
char *user_cert = NULL;
char *user_key = NULL;
struct timeval default_timeout = { 5, 0 };
char *hostname = NULL;


void vtprintf(test_ctx_t *ctx, const char *msg, va_list ap) {
	if (ctx && ctx->test_name) printf("[%s] ", ctx->test_name);
	vprintf(msg, ap);
}


void tprintf(test_ctx_t *ctx, const char *msg, ...) {
	va_list ap;

	va_start(ap, msg);
	vtprintf(ctx, msg, ap);
	va_end(ap);
}


int vcheck_gss(test_ctx_t *ctx, int err, edg_wll_GssStatus *gss_stat, const char *msg, va_list ap) {
	char *desc;
	const char *s;

	if (err == 0) return 1;

	vtprintf(ctx, msg, ap);
	printf(": ");
	switch (err) {
	case EDG_WLL_GSS_OK:
		printf("no error\n");
		break;
	case EDG_WLL_GSS_ERROR_GSS:
		edg_wll_gss_get_error(gss_stat, msg, &desc);
		printf("%s\n", desc);
		free(desc);
		break;
	case EDG_WLL_GSS_ERROR_HERRNO:
		s = hstrerror(h_errno);
		if (s) printf("%s\n",  s);
		else printf("h_errno %d\n", h_errno);
		break;
	case EDG_WLL_GSS_ERROR_ERRNO:
		printf("%s\n", strerror(errno));
		break;
	case EDG_WLL_GSS_ERROR_TIMEOUT:
		printf("connection timed out\n");
		break;
	case EDG_WLL_GSS_ERROR_EOF:
		printf("no more data (GSS EOF)\n");
		break;
	default:
		printf("unknown error type %d from glite_gss\n", err);
		break;
	}

	return 0;
}


int check_gss(test_ctx_t *ctx, int err, edg_wll_GssStatus *gss_stat, const char *msg, ...) {
	va_list ap;
	int ret;

	va_start(ap, msg);
	ret = vcheck_gss(ctx, err, gss_stat, msg, ap);
	va_end(ap);

	return ret;
}


int vcheck_errno(test_ctx_t *ctx, int err, const char *msg, va_list ap) {
	if (err != -1) return 1;
	vtprintf(ctx, msg, ap);
	printf(": ");
	printf("%s\n", strerror(errno));

	return 0;
}


int check_errno(test_ctx_t *ctx, int err, const char *msg, ...) {
	va_list ap;
	int ret;

	va_start(ap, msg);
	ret = vcheck_errno(ctx, err, msg, ap);
	va_end(ap);

	return ret;
}


int vcheck_errno_pid(test_ctx_t *ctx, pid_t pid, const char *msg, va_list ap) {
	if (pid != (pid_t)-1) return 1;
	vtprintf(ctx, msg, ap);
	printf(": %s", strerror(errno));

	return 0;
}


int check_errno_pid(test_ctx_t *ctx, pid_t pid, const char *msg, ...) {
	va_list ap;
	int ret;

	va_start(ap, msg);
	ret = vcheck_errno_pid(ctx, pid, msg, ap);
	va_end(ap);

	return ret;
}


static const char *str_default(const char *s) {
	return s ? : "(default)";
}


void usage() {
	printf("Usage: %s [OPTIONS]\n\
OPTIONS are:\n\
  -h, --help\n\
  -c, --cert=HOST_CERT ....... server certificate or proxy\n\
  -k, --key=HOST_KEY ......... server key\n\
  -C, --user-cert=USER_CERT .. user certificate or proxy\n\
  -K, --user-key=USER_KEY .... user key\n\
  -H, --hostname ............. local host name to connect to\n\
  -t, --timeout .............. network timeout in seconds\n\
\n\
For proper functionality user proxy certificate must be valid. Also server\n\
certificate and key (or server proxy certificate) must correspond to the\n\
specified hostname.\n\
", test_prog_name);
}


int parse_opts(int argc, char * const argv[]) {
	int opt;
	char *to;

	test_prog_name = strrchr(argv[0], '/');
	if (test_prog_name) test_prog_name++;
	else test_prog_name = argv[0];

	user_key = user_cert = getenv("X509_USER_PROXY");
	if (!user_key) user_key = getenv("X509_USER_KEY");
	if (!user_cert) user_cert = getenv("X509_USER_CERT");

	server_key = server_cert = getenv("X509_HOST_PROXY");
	if (!server_key) server_key = getenv("X509_HOST_KEY");
	if (!server_cert) server_cert = getenv("X509_HOST_CERT");

	to = getenv("GSS_TEST_TIMEOUT");
	if (to) default_timeout.tv_sec = atoi(to);

	while ((opt = getopt_long(argc, argv, test_getopt_string, test_longopts, NULL)) != EOF)
		switch (opt) {
			case 'c':
				server_cert = optarg;
				break;
			case 'k':
				server_key = optarg;
				break;
			case 'C':
				user_cert = optarg;
				break;
			case 'K':
				user_key = optarg;
				break;
			case 'h':
				usage();
				return RESULT_FATAL;
				break;
			case 'H':
				hostname = optarg;
				break;
			case 't':
				default_timeout.tv_sec = atoi(optarg);
				break;
		}
	if (optind < argc) {
		usage();
		return RESULT_FATAL;
	}
	if (!server_key) server_key = server_cert;
	if (!user_key) user_key = user_cert;
	if (!hostname) hostname = "localhost";

	printf("[main] test parameters:\n\
  server cert: %s\n\
  server key: %s\n\
  user cert: %s\n\
  user key: %s\n\
  host name: %s\n\
  timeout: %lu sec\n\
", str_default(server_cert), str_default(server_key), str_default(user_cert), str_default(user_key), hostname, (unsigned long)default_timeout.tv_sec);

	return RESULT_OK;
}


int test_setup(test_ctx_t *ctx, const char *name) {
	edg_wll_GssStatus gss_stat;
	int ret;

	struct addrinfo	hints;
	struct addrinfo	*ai;
	char servname[16];
	struct sockaddr_storage	a;
	socklen_t alen = sizeof(a);

	memset(ctx, 0, sizeof(test_ctx_t));
	ctx->test_name = name;
	ctx->timeout = default_timeout;

	if (!check_gss(ctx, edg_wll_gss_acquire_cred(server_cert, server_key, GSS_C_ACCEPT, &ctx->server_cred, &gss_stat), &gss_stat,
		"can't acquire server credentials"))
		goto err;
	tprintf(ctx, "server credentials initialized\n");

	if (!check_gss(ctx, edg_wll_gss_acquire_cred(user_cert, user_key, GSS_C_INITIATE, &ctx->user_cred, &gss_stat), &gss_stat,
		"can't acquire user credentials"))
		goto err;
	tprintf(ctx, "user credentials initialized\n");

	memset(&hints, '\0', sizeof (hints));
	hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE | AI_ADDRCONFIG;
	hints.ai_socktype = SOCK_STREAM;

	snprintf(servname, sizeof servname, "%d", ctx->port);
	if ((ret = getaddrinfo (NULL, servname, &hints, &ai)) != 0) {
		tprintf(ctx, "getaddrinfo() failed: %s\n", gai_strerror(ret));
		goto err;
	}

	ctx->sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (!check_errno(ctx, ctx->sock, "creating socket failed")) {
		freeaddrinfo(ai);
		goto err;
	}
	tprintf(ctx, "socket created (%d)\n", ctx->sock);

	if (!check_errno(ctx, bind(ctx->sock, ai->ai_addr, ai->ai_addrlen), "bind() failed")) {
		freeaddrinfo(ai);
		goto err;
	}

	freeaddrinfo(ai);

	if (!check_errno(ctx, listen(ctx->sock, 1), "listen() failed"))
		goto err;

	if (!check_errno(ctx, getsockname(ctx->sock, (struct sockaddr *)&a, &alen), "getsockname() failed"))
		goto err;

	if ((ret = getnameinfo((struct sockaddr *)&a, alen, NULL, 0, servname, sizeof(servname), NI_NUMERICSERV)) != 0) {
		tprintf(ctx, "getnameinfo() failed: %s\n", gai_strerror(ret));
		goto err;
	}
	ctx->port = atoi(servname);
	tprintf(ctx, "port: %d\n", ctx->port);

	return 0;
err:
	test_cleanup(ctx);
	return -1;
}


void test_cleanup(test_ctx_t *ctx) {
	edg_wll_GssStatus gss_stat;

	if (ctx->sock != -1) {
		close(ctx->sock);
		ctx->sock = -1;
		tprintf(ctx, "socket closed\n");
	}

	if (ctx->server_cred) {
		check_gss(ctx, edg_wll_gss_release_cred(&ctx->server_cred, &gss_stat), &gss_stat,
			"can't release server credentials");
		ctx->server_cred = NULL;
		tprintf(ctx, "server credenials released\n");
	}

	if (ctx->user_cred) {
		check_gss(ctx, edg_wll_gss_release_cred(&ctx->user_cred, &gss_stat), &gss_stat,
			"can't release server credentials");
		ctx->user_cred = NULL;
		tprintf(ctx, "user credenials released\n");
	}

	return;
}
