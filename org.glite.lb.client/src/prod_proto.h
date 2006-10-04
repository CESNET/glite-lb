#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_PROD_PROTO_H__
#define __EDG_WORKLOAD_LOGGING_CLIENT_PROD_PROTO_H__

#ident "$Header$"

#ifdef __cplusplus
extern "C" {
#endif


#include "glite/security/glite_gss.h"
#include "glite/lb/log_proto.h"
#include "glite/lb/context-int.h"

/**
 * client part of the logging protocol, used when sending messages to locallogger
 * \param[in,out] context	context to work with
 * \param[in] conn		connection to use
 * \param[in] logline		message to send
 * \return errno
 */
int edg_wll_log_proto_client(edg_wll_Context context, edg_wll_GssConnection *conn, edg_wll_LogLine logline);

/**
 * connect to locallogger
 * \param[in,out] context	context to work with
 * \param[out] conn		opened connection
 * \return errno
 */
int edg_wll_log_connect(edg_wll_Context context, edg_wll_GssConnection *conn);

/**
 * write/send to locallogger
 * \param[in,out] context	context to work with
 * \param[in] conn		connection to use
 * \param[in] logline		message to send
 * \return 	the number of bytes written (zero indicates nothing was written) or -1 on error
 */
int edg_wll_log_write(edg_wll_Context context, edg_wll_GssConnection *conn, edg_wll_LogLine logline);

/**
 * read/receive from locallogger
 * \param[in,out] context	context to work with
 * \param[in] conn		connection to use
 * \return 	the number of bytes read (zero indicates nothing was read) or -1 on error
 */
int edg_wll_log_read(edg_wll_Context context, edg_wll_GssConnection *conn);

/**
 * client part of the logging protocol, used when sending messages to lbproxy
 * \param[in,out] context	context to work with
 * \param[in] conn		connection to use
 * \param[in] logline		message to send
 * \return errno
 */
int edg_wll_log_proxy_proto_client(edg_wll_Context context, edg_wll_PlainConnection *conn, edg_wll_LogLine logline);

/**
 * connect to lbproxy
 * \param[in,out] context	context to work with
 * \param[out] conn		opened connection
 * \return errno
 */
int edg_wll_log_proxy_connect(edg_wll_Context context, edg_wll_PlainConnection *conn);

/**
 * write/send to lbproxy
 * \param[in,out] context	context to work with
 * \param[in] conn		connection to use
 * \param[in] logline		message to send
 * \return 	the number of bytes written (zero indicates nothing was written) or -1 on error
 */
int edg_wll_log_proxy_write(edg_wll_Context context, edg_wll_PlainConnection *conn, edg_wll_LogLine logline);

/**
 * read/receive from lbproxy
 * \param[in,out] context	context to work with
 * \param[in] conn		connection to use
 * \return 	the number of bytes read (zero indicates nothing was read) or -1 on error
 */
int edg_wll_log_proxy_read(edg_wll_Context context, edg_wll_PlainConnection *conn);

/**
 * client part of the logging protocol, used when sending messages directly to bkserver
 * \param[in,out] context	context to work with
 * \param[in] conn		connection to use
 * \param[in] logline		message to send
 * \return errno
 */
int edg_wll_log_direct_proto_client(edg_wll_Context context, edg_wll_GssConnection *conn, edg_wll_LogLine logline);

/** 
 * connect to bkserver
 * \param[in,out] context	context to work with
 * \param[out] conn		opened connection
 * \return errno
 */
int edg_wll_log_direct_connect(edg_wll_Context context, edg_wll_GssConnection *conn);

/**
 * write/send to bkserver
 * \param[in,out] context	context to work with
 * \param[in] conn		connection to use
 * \param[in] logline		message to send
 * \return 	the number of bytes written (zero indicates nothing was written) or -1 on error
 */
int edg_wll_log_direct_write(edg_wll_Context context, edg_wll_GssConnection *conn, edg_wll_LogLine logline);

/**
 * read/receive from bkserver
 * \param[in,out] context	context to work with
 * \param[in] conn		connection to use
 * \return 	the number of bytes read (zero indicates nothing was read) or -1 on error
 */
int edg_wll_log_direct_read(edg_wll_Context context, edg_wll_GssConnection *conn);

/**
 * Handle GSS failures on the client side
 * \param[in,out] context	context to work with
 * \param[in] code		code returned by a previous call
 * \param[in] gss_code		GSS code returned by a previous call
 * \param[in] test		additional text to print
 * \return errno
 */
int edg_wll_log_proto_handle_gss_failures(edg_wll_Context context, int code, edg_wll_GssStatus *gss_code, const char *text);

/**
 * Handle UNIX socket failures on the client side
 * \param[in,out] context	context to work with
 * \param[in] code		code returned by a previous call
 * \param[in] test		additional text to print
 * \return errno
 */
int edg_wll_log_proto_handle_plain_failures(edg_wll_Context context, int code, const char *text);


#ifdef __cplusplus
}
#endif

#endif /* __EDG_WORKLOAD_LOGGING_CLIENT_PROD_PROTO_H__ */
