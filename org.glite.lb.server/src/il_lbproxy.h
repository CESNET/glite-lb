#ifndef __GLITE_LB_IL_LBPROXY_H__
#define __GLITE_LB_IL_LBPROXY_H__

#include "glite/lb/context.h"

#ifdef __cplusplus
#extern "C" {
#endif

extern char *lbproxy_ilog_socket_path;
extern char *lbproxy_ilog_file_prefix;

int edg_wll_EventSendProxy(edg_wll_Context ctx, const edg_wlc_JobId jobid, const char *event);

#ifdef __cplusplus
}
#endif

#endif /* __GLITE_LB_IL_LBPROXY_H__ */
