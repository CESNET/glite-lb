#ifndef __EDG_WORKLOAD_LOGGING_COMMON_LOG_PROTO_H__
#define __EDG_WORKLOAD_LOGGING_COMMON_LOG_PROTO_H__

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
