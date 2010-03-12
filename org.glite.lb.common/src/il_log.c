#ident "$Header$"
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
		fprintf(stderr, "[%6ld] ", (long) pthread_self());
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
