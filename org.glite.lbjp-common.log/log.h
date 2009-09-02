#ifndef GLITE_LBU_LOG_H
#define GLITE_LBU_LOG_H

#ident "$Header$"

/* gLite common logging recommendations v1.1 https://twiki.cern.ch/twiki/pub/EGEE/EGEEgLite/logging.html */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdio.h>
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
#define LOG_CATEGORY_LB_SERVER_ACCESS	"LB.SERVER.ACCESS"

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
static inline int glite_common_log_init(void) {
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
static inline int glite_common_log_fini(void) {
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
static inline void glite_common_log_msg(char *catName,int a_priority, char *msg) {
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
static inline void glite_common_log(char *catName,int a_priority, const char* a_format,...) {
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
static inline void glite_common_log_reread(void) {
#ifndef WITHOUT_LOG4C
	log4c_reread();
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* GLITE_LBU_LOG_H */
