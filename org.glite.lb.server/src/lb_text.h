#ifndef GLITE_LB_TEXT_H
#define GLITE_LB_TEXT_H

#include "glite/lb/context.h"
#include "glite/lb/events.h"
#include "glite/lb/jobstat.h"
#include "lb_proto.h"

int edg_wll_QueryToText(edg_wll_Context,edg_wll_Event *,char **);
int edg_wll_JobStatusToText(edg_wll_Context, edg_wll_JobStat, char **);
int edg_wll_UserInfoToText(edg_wll_Context, edg_wlc_JobId *, char **);
int edg_wll_UserNotifsToText(edg_wll_Context ctx, char **notifids, char **message);
int edg_wll_NotificationToText(edg_wll_Context ctx, notifInfo *ni, char **message);
char *edg_wll_ErrorToText(edg_wll_Context,int);

#endif /* GLITE_LB_TEXT */
