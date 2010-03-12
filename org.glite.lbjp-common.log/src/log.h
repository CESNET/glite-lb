#ifndef GLITE_LBU_LOG_H
#define GLITE_LBU_LOG_H

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


/* gLite common logging recommendations v1.1 https://twiki.cern.ch/twiki/pub/EGEE/EGEEgLite/logging.html */

#ifdef __cplusplus
extern "C" {
#endif

#include <log4c.h>

/* default categories */
#define LOG_CATEGORY_NAME       	"root"
#define LOG_CATEGORY_SECURITY   	"SECURITY"
#define LOG_CATEGORY_ACCESS     	"ACCESS"
#define LOG_CATEGORY_CONTROL    	"CONTROL"
#define LOG_CATEGORY_LB 		"LB"
#define LOG_CATEGORY_LB_LOGD 		"LB.LOGD"
#define LOG_CATEGORY_LB_IL 		"LB.INTERLOGD"
#define LOG_CATEGORY_LB_SERVER 		"LB.SERVER"
#define LOG_CATEGORY_LB_SERVER_DB  	"LB.SERVER.DB"
#define LOG_CATEGORY_LB_SERVER_REQUEST	"LB.SERVER.REQUEST"

/* default priorities */
#define LOG_PRIORITY_FATAL      LOG4C_PRIORITY_FATAL
#define LOG_PRIORITY_ERROR      LOG4C_PRIORITY_ERROR
#define LOG_PRIORITY_WARN       LOG4C_PRIORITY_WARN
#define LOG_PRIORITY_INFO       LOG4C_PRIORITY_INFO
#define LOG_PRIORITY_DEBUG      LOG4C_PRIORITY_DEBUG
#define LOG_PRIORITY_NOTSET     LOG4C_PRIORITY_NOTSET

#define SYSTEM_ERROR(my_err) { \
        if (errno !=0 ) \
                glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_ERROR,"%s: %s\n",my_err,strerror(errno)); \
        else \
                glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_ERROR,"%s\n",my_err); }

#define SYSTEM_FATAL(my_err) { \
        if (errno !=0 ) \
                glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_FATAL,"%s: %s\n",my_err,strerror(errno)); \
        else \
                glite_common_log(LOG_CATEGORY_CONTROL,LOG_PRIORITY_FATAL,"%s\n",my_err); }

/* logging functions */

/**
 * constructor
 * 
 * @returns 0 for success 
 */
extern int glite_common_log_init(void);

/**
 * destructor
 *
 * @returns 0 for success 
 */
extern int glite_common_log_fini(void);

/** 
 * Log a message with the specified priority.
 * @param catName category name
 * @param a_priority The priority of this log message.
 * @param msg message
 **/
extern void glite_common_log_msg(char *catName,int a_priority, char *msg);

/** 
 * Log a message with the specified priority.
 * @param catName category name
 * @param a_priority The priority of this log message.
 * @param a_format Format specifier for the string to write in the log file.
 * @param ... The arguments for a_format 
 **/
extern void glite_common_log(char *catName,int a_priority, const char* a_format,...);

/*
 * Rereads any log4crc files that have changed
 */
extern void glite_common_log_reread(void);

#ifdef __cplusplus
}
#endif

#endif /* GLITE_LBU_LOG_H */
