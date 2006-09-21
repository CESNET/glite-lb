#ifndef IL_LBPROXY_H
#define IL_LBPROXY_H

#ifdef __cplusplus
#extern "C" {
#endif

extern char *lbproxy_ilog_socket_path;
extern char *lbproxy_ilog_file_prefix;

int edg_wll_EventSendProxy(edg_wll_Context ctx, const glite_lbu_JobId jobid, const char *event);

#ifdef __cplusplus
}
#endif

#endif
