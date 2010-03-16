#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_CONNECTION_H__
#define __EDG_WORKLOAD_LOGGING_CLIENT_CONNECTION_H__

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


#endif /* __EDG_WORKLOAD_LOGGING_CLIENT_CONNECTION_H__ */
