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


#include <stdarg.h>
#include <stdio.h>
#include <log4c.h>
#include "log.h"

/**
 * constructor
 * 
 * @returns 0 for success 
 */
int glite_common_log_init(void) {
#ifndef WITHOUT_LOG4C
	return(log4c_init());
#else
	return 0;
#endif
}

/**
 * destructor
 *
 * @returns 0 for success 
 */
int glite_common_log_fini(void) {
#ifndef WITHOUT_LOG4C
	return(log4c_fini());
#else
	return 0;
#endif
}

/** 
 * Log a message with the specified priority.
 * @param catName category name
 * @param a_priority The priority of this log message.
 * @param msg message
 **/
void glite_common_log_msg(const char *catName,int a_priority, const char *msg) {
#ifndef WITHOUT_LOG4C
        const log4c_category_t* a_category = log4c_category_get(catName);

        if (log4c_category_is_priority_enabled(a_category, a_priority)) {
                log4c_category_log(log4c_category_get(catName), a_priority, "%s", msg);
        }
#else
	printf(msg);
#endif
}

/** 
 * Log a message with the specified priority.
 * @param catName category name
 * @param a_priority The priority of this log message.
 * @param a_format Format specifier for the string to write in the log file.
 * @param ... The arguments for a_format 
 **/
void glite_common_log(const char *catName, int a_priority, const char* a_format,...) {
#ifndef WITHOUT_LOG4C
        const log4c_category_t* a_category = log4c_category_get(catName);

        if (log4c_category_is_priority_enabled(a_category, a_priority)) {
                va_list va;

                va_start(va, a_format);
                log4c_category_vlog(a_category, a_priority, a_format, va);
                va_end(va);
        }
	if (a_priority == LOG_PRIORITY_FATAL) {
		va_list	va;
		va_start(va,a_format);
		vfprintf(stderr,a_format,va);
		fputc('\n',stderr);
		va_end(va);
	}
#else
	va_list va;

	va_start(va, a_format);
	vprintf(a_format,va);
	va_end(va);
#endif
}

const char *glite_common_log_priority_to_string(int a_priority) {
#ifndef WITHOUT_LOG4C
	return log4c_priority_to_string(a_priority);
#else
	char *out;

	switch (priority) {
	case LOG_PRIORITY_FATAL: out=strdup("FATAL");	break;
	case LOG_PRIORITY_ERROR: out=strdup("ERROR");	break;
	case LOG_PRIORITY_WARN:  out=strdup("WARNING");	break;
	case LOG_PRIORITY_INFO:  out=strdup("INFO");	break;
	case LOG_PRIORITY_DEBUG: out=strdup("DEBUG");	break;
	case LOG_PRIORITY_NOTSET:
	default:
		out=strdup("PRIORITY_NOT_SET");	
	}
	return out;
#endif
}

/**
 * Returns priority of a given category
 */
int glite_common_log_get_priority(const char *catName) {
#ifndef WITHOUT_LOG4C
	return log4c_category_get_priority(log4c_category_get(catName));
#else
	/* if not LOG4C, mapping of priorities to categories 
	 * must be specified somehow differently (not specified at the moment)
	 */
	return LOG_PRIORITY_NOTSET;
#endif
}

/*
 * Rereads any log4crc files that have changed
 */
void glite_common_log_reread(void) {
#ifndef WITHOUT_LOG4C
	log4c_reread();
#endif
}


