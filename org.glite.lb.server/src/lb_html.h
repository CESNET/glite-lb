#ifndef GLITE_LB_HTML_H
#define GLITE_LB_HTML_H

#ident "$Header$"

#include "glite/lb/context.h"
#include "glite/lb/events.h"
#include "glite/lb/jobstat.h"
#include "lb_proto.h"

int edg_wll_QueryToHTML(edg_wll_Context,edg_wll_Event *,char **);
int edg_wll_JobStatusToHTML(edg_wll_Context, edg_wll_JobStat, char **);
int edg_wll_UserInfoToHTML(edg_wll_Context, edg_wlc_JobId *, char **);
int edg_wll_UserNotifsToHTML(edg_wll_Context ctx, char **notifids, char **message);
int edg_wll_NotificationToHTML(edg_wll_Context ctx, notifInfo *ni, char **message);
char *edg_wll_ErrorToHTML(edg_wll_Context,int);

#endif /* GLITE_LB_HTML_H */
