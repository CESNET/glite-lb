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

#include "glite/lb/log_proto.h"
#include "glite/lb/dgssl.h"

int edg_wll_log_proto_server(SSL *ssl, char *name, char *prefix, int noipc, int noparse);
int edg_wll_log_proto_server_failure(int code, const char *text);

#define SYSTEM_ERROR(my_err) { \
	if (errno !=0 ) \
		edg_wll_ll_log(LOG_ERR,"%s: %s\n",my_err,strerror(errno)); \
	else \
		edg_wll_ll_log(LOG_ERR,"%s\n",my_err); }

void edg_wll_set_environment(char *user_cert,
                    char *user_key,
	     	    char *user_proxy,
	            char *CAcert_file,
		    char *CAcert_dir,
		    char *gridmapfile);

/* locallogger daemon error handling */

extern int edg_wll_ll_log_level;
void edg_wll_ll_log_init(int level);
void edg_wll_ll_log(int level, const char *fmt, ...);

/* fcntl defaults */

#define FCNTL_ATTEMPTS		5
#define FCNTL_TIMEOUT		1

/* locallogger daemon listen and connect functions prototypes */

int do_listen(int port);
int do_connect(char *hostname, int port);


#ifdef __cplusplus
}
#endif

#endif /* __EDG_WORKLOAD_LOGGING_LOCALLOGGER_LOGD_PROTO_H__ */
