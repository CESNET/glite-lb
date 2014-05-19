#ifndef TESTCORE_H
#define TESTCORE_H

#include <sys/time.h>
#include <sys/types.h>
#include <stdarg.h>

#include "glite_gss.h"

#define RESULT_OK 0
#define RESULT_FAILED 1
#define RESULT_FATAL 2

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	const char *test_name;
	edg_wll_GssCred server_cred, user_cred;
	struct timeval timeout;
	int sock;
	int port;
} test_ctx_t;


extern const char* test_prog_name;

extern char *server_cert;
extern char *server_key;
extern char *user_cert;
extern char *user_key;
extern struct timeval default_timeout;
extern char *hostname;


void vtprintf(test_ctx_t *ctx, const char *msg, va_list ap);
void tprintf(test_ctx_t *ctx, const char *msg, ...);
int vcheck_gss(test_ctx_t *ctx, int err, edg_wll_GssStatus *gss_stat, const char *msg, va_list ap);
int check_gss(test_ctx_t *ctx, int err, edg_wll_GssStatus *gss_stat, const char *msg, ...);
int vcheck_errno(test_ctx_t *ctx, int err, const char *msg, va_list ap);
int check_errno(test_ctx_t *ctx, int err, const char *msg, ...);
int vcheck_errno_pid(test_ctx_t *ctx, pid_t pid, const char *msg, va_list ap);
int check_errno_pid(test_ctx_t *ctx, pid_t pid, const char *msg, ...);

void usage();
int parse_opts(int argc, char * const argv[]);

int test_setup(test_ctx_t *ctx, const char *name);
void test_cleanup(test_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif
