#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#include <libgen.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "glite_gss.h"
#include "testcore.h"


sig_atomic_t signal_received = 0;
test_ctx_t ctx;


void handle_signal(int sig __attribute__((unused))) {
	signal_received = 1;
	/* XXX: it is not portable to call printf() here */
	tprintf(&ctx, "signal received\n");
}


void replier(const char *name, int wait) {
	struct timespec wait_ts;
	int s;
	edg_wll_GssConnection	conn;
	struct sockaddr_storage	a;
	socklen_t		alen = sizeof(a);
	edg_wll_GssStatus gss_stat;
	int result = RESULT_FATAL;

	int len;
	char buf[8*BUFSIZ];

	ctx.test_name = name;
	tprintf(&ctx, "started (pid %d)\n", getpid());

	if (wait) {
		tprintf(&ctx, "sleeping...\n");
		wait_ts.tv_sec = wait;
		wait_ts.tv_nsec = 0;
		if (!check_errno(&ctx, nanosleep(&wait_ts, NULL), "nanosleep() failed or interrupted"))
			goto err;
		tprintf(&ctx, "sleep finished\n");
	}

	s = accept(ctx.sock, (struct sockaddr *)&a, &alen);
	if (!check_errno(&ctx, s, "accept() failed"))
		goto err;
	tprintf(&ctx, "new connection, socket %d\n", s);

	if (!check_gss(&ctx, edg_wll_gss_accept(ctx.server_cred, s, &ctx.timeout, &conn, &gss_stat), &gss_stat,
		"GSS accept failed"))
		goto err_sock;
	tprintf(&ctx, "new connection accepted, GSS OK\n");

	while ((len = edg_wll_gss_read(&conn, buf, sizeof(buf), &ctx.timeout, &gss_stat)) >= 0 ) {
		tprintf(&ctx, "read %d bytes\n", len);
		if (!check_gss(&ctx, edg_wll_gss_write(&conn, buf, len, &ctx.timeout, &gss_stat), &gss_stat,
			"GSS write failed"))
			goto err_conn;
		tprintf(&ctx, "wrote %d bytes\n", len);
	}

	result = 0;

err_conn:
	if (edg_wll_gss_close(&conn, &ctx.timeout) != 0) {
		tprintf(&ctx, "GSS close failed");
		result = RESULT_FATAL;
	}
err_sock:
	close(s);
err:
	test_cleanup(&ctx);
	tprintf(&ctx, "end, result %s\n", result ? "fail" : "ok");
	exit(result);
}


int sender(const char *name) {
	edg_wll_GssConnection	conn;
	edg_wll_GssStatus	gss_stat;
	const char *buf = "This is a test.";
	char buf2[100];
	size_t total;
	int len, result = RESULT_FATAL;

	ctx.test_name = name;
	tprintf(&ctx, "started (pid %d)\n", getpid());

	tprintf(&ctx, "waiting for connection...\n");
	if (!check_gss(&ctx, edg_wll_gss_connect(ctx.user_cred, hostname, ctx.port, &ctx.timeout, &conn, &gss_stat), &gss_stat,
		"GSS connect failed"))
		goto err;
	tprintf(&ctx, "connected\n");

	tprintf(&ctx, "signal received = %d\n", signal_received);

	if (!check_gss(&ctx, edg_wll_gss_write(&conn, buf, strlen(buf)+1, &ctx.timeout, &gss_stat), &gss_stat,
		"GSS write failed"))
		goto err_conn;
	tprintf(&ctx, "written %zd bytes\n", strlen(buf)+1);

	len = edg_wll_gss_read_full(&conn, buf2, strlen(buf)+1, &ctx.timeout, &total, &gss_stat);
	if (!check_gss(&ctx, len, &gss_stat, "GSS read failed"))
		goto err_conn;
	tprintf(&ctx, "read %d bytes\n", len);

	result = RESULT_FAILED;
	if (strlen(buf) + 1 != total) {
		tprintf(&ctx, "sent and read data size differs!\n");
		goto err_conn;
	}
	if (strcmp(buf, buf2) != 0) {
		tprintf(&ctx, "sent and read data differs!\n");
		goto err_conn;
	}
	tprintf(&ctx, "data matches!\n");
	result = RESULT_OK;

err_conn:
	ctx.timeout = default_timeout;
	if (edg_wll_gss_close(&conn, &ctx.timeout) != 0) {
		tprintf(&ctx, "GSS close failed");
		result = RESULT_FATAL;
	}
err:
	test_cleanup(&ctx);
	tprintf(&ctx, "end, result %s\n", result ? "fail" : "ok");
	exit(result);
}


int test_reply() {
	struct timespec wait_ts;
	pid_t pid_replier, pid_sender;
	int err, result = RESULT_FATAL;

	ctx.test_name = "test_reply";
	ctx.timeout = default_timeout;

	pid_replier = fork();
	if (!check_errno_pid(&ctx, pid_replier, "fork() for replier failed"))
		goto err;
	if (pid_replier == 0) replier("test_reply][replier", 2);

	pid_sender = fork();
	if (!check_errno_pid(&ctx, pid_sender, "fork() for sender failed")) {
		kill(pid_replier, SIGTERM);
		goto err;
	}
	if (pid_sender == 0) sender("test_reply][sender");

	wait_ts.tv_sec = 0;
	wait_ts.tv_nsec = 500000000;
	nanosleep(&wait_ts, NULL);

	if (!check_errno(&ctx, kill(pid_sender, SIGHUP), "couldn't send signal"))
		goto err_proc;
	tprintf(&ctx, "signal sent\n");

	result = RESULT_OK;

err_proc:
	tprintf(&ctx, "wait for replier...\n");
	waitpid(pid_replier, &err, 0);
	if (WIFEXITED(err)) {
		if (result < WEXITSTATUS(err)) result = WEXITSTATUS(err);
	} else result = RESULT_FATAL;

	tprintf(&ctx, "wait for sender...\n");
	waitpid(pid_sender, &err, 0);
	if (WIFEXITED(err)) {
		if (result < WEXITSTATUS(err)) result = WEXITSTATUS(err);
	} else result = RESULT_FATAL;

err:
	return result;
}


int main(int argc, char * const argv[]) {
	struct sigaction sa,osa;
	int result;

	ctx.test_name = "main";
	if ((result = parse_opts(argc, argv)) != 0) return result;
	result = RESULT_FATAL;

	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGHUP);
	sa.sa_handler = handle_signal;
	if (sigaction(SIGHUP, &sa, &osa) == -1) {
		tprintf(&ctx, "installing signal handler failed: %s\n", strerror(errno));
		goto err;
	}

	edg_wll_gss_initialize();

	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGHUP);
	if (sigprocmask(SIG_UNBLOCK, &sa.sa_mask, NULL) == -1) {
		tprintf(&ctx, "unmasking signal %d failed: %s\n", SIGHUP, strerror(errno));
		edg_wll_gss_finalize();
		goto err;
	}

	if (test_setup(&ctx, "main") != 0) {
		edg_wll_gss_finalize();
		goto err;
	}

	result = test_reply();
	ctx.test_name = "main";

	test_cleanup(&ctx);
	edg_wll_gss_finalize();
err:
	sigaction(SIGHUP, &osa, NULL);

	tprintf(&ctx, "%s\n", result == 0 ? "OK" : "TEST FAILED");
	return result;
}
