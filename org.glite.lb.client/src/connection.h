#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_CONNECTION_H__
#define __EDG_WORKLOAD_LOGGING_CLIENT_CONNECTION_H__

#ident "$Header"

int edg_wll_close(edg_wll_Context ctx);
int edg_wll_open(edg_wll_Context ctx);
int edg_wll_http_send_recv(edg_wll_Context, char *, const char * const *, char *, char **, char ***, char **);
int http_check_status(edg_wll_Context, char *);


#endif /* __EDG_WORKLOAD_LOGGING_CLIENT_CONNECTION_H__ */
