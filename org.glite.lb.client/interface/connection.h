#ifndef GLITE_LB_CONNECTION_H
#define GLITE_LB_CONNECTION_H

#ident "$Header$"

int edg_wll_close(edg_wll_Context ctx,int *);
int edg_wll_open(edg_wll_Context ctx,int *);
int edg_wll_http_send_recv(edg_wll_Context, char *, const char * const *, char *, char **, char ***, char **);

int edg_wll_close_proxy(edg_wll_Context ctx);
int edg_wll_open_proxy(edg_wll_Context ctx);
int edg_wll_http_send_recv_proxy(edg_wll_Context, char *, const char * const *, char *, char **, char ***, char **);

int http_check_status(edg_wll_Context, char *);

int ConnectionIndex(edg_wll_Context ctx, const char *name, int port);
int AddConnection(edg_wll_Context ctx, char *name, int port);
int ReleaseConnection(edg_wll_Context ctx, char *name, int port);
int CloseConnection(edg_wll_Context ctx, int* conn_index);

#define PROXY_CONNECT_RETRY 10 /* ms */


#endif /* GLITE_LB_CONNECTION_H */
