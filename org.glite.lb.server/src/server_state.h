#ifndef _EDG_WORKLOAD_LOGGING_LBSERVER_H_
#define EDG_WLL_STATE_DUMP_START	"StartDump"
#define EDG_WLL_STATE_DUMP_END		"EndDump"

#ident "$Header$"

int edg_wll_GetServerState(edg_wll_Context,const char *,char **);
int edg_wll_SetServerState(edg_wll_Context,const char *,const char *);

#endif
