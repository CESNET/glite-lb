#ifndef __EDG_WORKLOAD_LOGGING_LBSERVER_WS_PLUGIN_H__
#define __EDG_WORKLOAD_LOGGING_LBSERVER_WS_PLUGIN_H__

/* /cvs/jra1mw/org.glite.lb.server/src/ws_plugin.h,v 1.2 2004/10/14 14:27:30 jskrabal */

#include "lb_gss.h"

#define PLUGIN_ID		"GLITE_WS_PLUGIN"

typedef struct _edg_wll_ws_Context {
	edg_wll_GssConnection *connection;
	struct timeval *timeout;
	char *error_msg;
} _edg_wll_ws_Context;

typedef _edg_wll_ws_Context * edg_wll_ws_Context;

int edg_wll_ws_plugin(struct soap *, struct soap_plugin *, void *);

#endif /* __EDG_WORKLOAD_LOGGING_LBSERVER_WS_PLUGIN_H__ */
