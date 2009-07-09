#ifndef __EDG_WORKLOAD_LOGGING_LOCALLOGGER_LOGD_PROTO_H__
#define __EDG_WORKLOAD_LOGGING_LOCALLOGGER_LOGD_PROTO_H__

#ident "$Header$"

/**
 * \file edg/workload/logging/locallogger/logd_proto.h
 * \brief server part of the logging protocol
 * \note private
 */

#ifdef __cplusplus
extern "C" {
#endif


#include <syslog.h>
#include <log4c.h>

#include "glite/lb/log_proto.h"
#include "glite/security/glite_gss.h"

int edg_wll_log_proto_server(edg_wll_GssConnection *con, struct timeval *timeout, char *name, char *prefix, int noipc, int noparse);
int edg_wll_log_proto_server_failure(int code, edg_wll_GssStatus *gss_code, const char *text);

/* locallogger daemon error handling */
/* gLite common logging recommendations v1.1 https://twiki.cern.ch/twiki/pub/EGEE/EGEEgLite/logging.html */

#define LOG_CATEGORY_NAME 	"root"
#define LOG_CATEGORY_SECURITY 	"glite-common-logging-security"
#define LOG_CATEGORY_ACCESS 	"glite-common-logging-access"
#define LOG_CATEGORY_CONTROL 	"glite-common-logging-control"

/* other priorities may be added, see include/log4c/priority.h */
#define LOG_PRIORITY_FATAL 	LOG4C_PRIORITY_FATAL
#define LOG_PRIORITY_ERROR 	LOG4C_PRIORITY_ERROR
#define LOG_PRIORITY_WARN  	LOG4C_PRIORITY_WARN
#define LOG_PRIORITY_INFO  	LOG4C_PRIORITY_INFO
#define LOG_PRIORITY_DEBUG 	LOG4C_PRIORITY_DEBUG
#define LOG_PRIORITY_NOTSET	LOG4C_PRIORITY_NOTSET

#define glite_common_log_security(priority,msg) glite_common_log_msg(LOG_CATEGORY_SECURITY,priority,msg)
#define glite_common_log_access(priority,msg)   glite_common_log_msg(LOG_CATEGORY_ACCESS,priority,msg)
#define glite_common_log_control(priority,msg)  glite_common_log_msg(LOG_CATEGORY_CONTROL,priority,msg)

#define SYSTEM_ERROR(my_err) { \
	if (errno !=0 ) \
		glite_common_log(LOG_CATEGORY_NAME,LOG_PRIORITY_ERROR,"%s: %s\n",my_err,strerror(errno)); \
	else \
		glite_common_log(LOG_CATEGORY_NAME,LOG_PRIORITY_ERROR,"%s\n",my_err); }

extern int glite_common_log_priority_security;
extern int glite_common_log_priority_access;
extern int glite_common_log_priority_control;

int glite_common_log_init(int a_priority);
int glite_common_log_fini(void);
// int  glite_common_log_setappender(char *catName, char *appName);
// void edg_wll_ll_log(int level, const char *fmt, ...);
void glite_common_log_msg(char *catName,int a_priority, char *msg);
void glite_common_log(char *catName,int a_priority, const char* a_format,...);


/* fcntl defaults */
#define FCNTL_ATTEMPTS		5
#define FCNTL_TIMEOUT		1

/* connect defaults */
#define CONNECT_ATTEMPTS	5

/* accept defaults */
#define ACCEPT_TIMEOUT		30

/* connection defaults */
#define CONNECTION_TIMEOUT	EDG_WLL_LOG_SYNC_TIMEOUT_MAX

/* locallogger daemon listen and connect functions prototypes */
int do_listen(int port);
int do_connect(char *hostname, int port);


#ifdef __cplusplus
}
#endif

#endif /* __EDG_WORKLOAD_LOGGING_LOCALLOGGER_LOGD_PROTO_H__ */
