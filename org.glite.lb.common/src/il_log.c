#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <pthread.h>

int log_level;

int
il_log(int level, char *fmt, ...)
{
	char *err_text;
	va_list fmt_args;

	va_start(fmt_args, fmt);
	vasprintf(&err_text, fmt, fmt_args);
	va_end(fmt_args);
	
	if(level <= log_level) {
		fprintf(stderr, "[%6d] ", pthread_self());
		fprintf(stderr, err_text);
	}

	if(level <= LOG_ERR) {
		openlog(NULL, LOG_PID | LOG_CONS, LOG_DAEMON);
		syslog(level, "%s", err_text);
		closelog();
	}

	if(err_text) free(err_text);

	return(0);
}
