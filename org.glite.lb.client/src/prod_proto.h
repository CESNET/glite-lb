#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_PROD_PROTO_H__
#define __EDG_WORKLOAD_LOGGING_CLIENT_PROD_PROTO_H__

#ident "$Header$"

/**
 * \file edg/workload/logging/client/prod_proto.h
 * \brief client (producer) part of the logging protocol
 * \note private
 */

#ifdef __cplusplus
extern "C" {
#endif


#include "glite/lb/log_proto.h"
#include "glite/lb/context-int.h"
#include "glite/lb/lb_gss.h"

int edg_wll_log_proto_client(edg_wll_Context context, edg_wll_GssConnection *con, edg_wll_LogLine logline);
int edg_wll_log_proto_client_direct(edg_wll_Context context, edg_wll_GssConnection *con, edg_wll_LogLine logline);
int edg_wll_log_proto_client_proxy(edg_wll_Context context, edg_wll_PlainConnection *conn, edg_wll_LogLine logline);

int edg_wll_log_proto_handle_gss_failures(edg_wll_Context context, int code, edg_wll_GssStatus *gss_code, const char *text);
int edg_wll_log_proto_handle_plain_failures(edg_wll_Context context, int code, const char *text);

#ifdef __cplusplus
}
#endif

#endif /* __EDG_WORKLOAD_LOGGING_CLIENT_PROD_PROTO_H__ */
