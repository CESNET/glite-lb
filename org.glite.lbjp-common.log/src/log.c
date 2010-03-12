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
void glite_common_log_msg(char *catName,int a_priority, char *msg) {
#ifndef WITHOUT_LOG4C
        const log4c_category_t* a_category = log4c_category_get(catName);

        if (log4c_category_is_priority_enabled(a_category, a_priority)) {
                log4c_category_log(log4c_category_get(catName), a_priority, msg);
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
void glite_common_log(char *catName,int a_priority, const char* a_format,...) {
#ifndef WITHOUT_LOG4C
        const log4c_category_t* a_category = log4c_category_get(catName);

        if (log4c_category_is_priority_enabled(a_category, a_priority)) {
                va_list va;

                va_start(va, a_format);
                log4c_category_vlog(a_category, a_priority, a_format, va);
                va_end(va);
        }
#else
	va_list va;

	va_start(va, a_format);
	vprintf(va, a_format);
	va_end(va);
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


