#ifndef __EDG_WORKLOAD_LOGGING_COMMON_LOG_PROTO_H__
#define __EDG_WORKLOAD_LOGGING_COMMON_LOG_PROTO_H__

#ident "$Header$"

/**
 * \file edg/workload/logging/common/log_proto.h
 * \brief common part of the logging protocol
 * \note private
 */

#ifdef __cplusplus
extern "C" {
#endif


#include "glite/lb/context.h"

/**
 * default (noauth) user name
 */
/** default user */
#define EDG_WLL_LOG_USER_DEFAULT                "anonymous"


/**
 * default prefix for names of log files 
 */
/** default prefix */
#define EDG_WLL_LOG_PREFIX_DEFAULT              "/var/glite/log/dglogd.log"


/** 
 * default local-logger Socket header 
 */
/** header text */
#define EDG_WLL_LOG_SOCKET_HEADER               "DGLOG"
/** header length */
#define EDG_WLL_LOG_SOCKET_HEADER_LENGTH        5


/**
 * default local-logger destination 
 */
/** host */
#define EDG_WLL_LOG_HOST_DEFAULT		"localhost"
/** port */
#define EDG_WLL_LOG_PORT_DEFAULT		9002


/**
 * default and maximal logging timeout (in seconds)
 */
#define EDG_WLL_LOG_TIMEOUT_DEFAULT		120
#define EDG_WLL_LOG_TIMEOUT_MAX			300
#define EDG_WLL_LOG_SYNC_TIMEOUT_DEFAULT	120
#define EDG_WLL_LOG_SYNC_TIMEOUT_MAX		600


/**
 * maximal message size for sync logging
 */
/** max message size in bytes */
// unlimited for tests!
#define EDG_WLL_LOG_SYNC_MAXMSGSIZE		102400000

	
/**
 * default maximal number of simultaneously open connections from one client
 */
// XXX: not used? #define EDG_WLL_LOG_CONNECTIONS_DEFAULT		4


#ifdef __cplusplus
}
#endif

/*
 * Functions which manage communication with interlogger.
 * (especially used in local logger when sending events to ilogd)
 */
extern int
edg_wll_log_event_write(
	edg_wll_Context     ctx,
	const char         *event_file,
	const char         *msg,
	unsigned int        fcntl_attempts,
	unsigned int        fcntl_timeout,
	long               *filepos);

extern int
edg_wll_log_event_send(
	edg_wll_Context     ctx,
	const char         *socket_path,
	long                filepos,
	const char         *msg,
	int                 msg_size,
	int                 conn_attempts,
	struct timeval     *timeout);


#endif /* __EDG_WORKLOAD_LOGGING_COMMON_LOG_PROTO_H__ */
