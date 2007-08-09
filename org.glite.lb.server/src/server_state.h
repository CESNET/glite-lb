#ifndef GLITE_LB_SERVER_STATE_H
#define GLITE_LB_SERVER_STATE_H

#ident "$Header$"

#define EDG_WLL_STATE_DUMP_START	"StartDump"
#define EDG_WLL_STATE_DUMP_END		"EndDump"

int edg_wll_GetServerState(edg_wll_Context,const char *,char **);
int edg_wll_SetServerState(edg_wll_Context,const char *,const char *);

#endif /* GLITE_LB_SERVER_STATE_H */
