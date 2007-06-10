#ifndef __GLITE_LB_HTML_H__
#define __GLITE_LB_HTML_H__

#ident "$Header$"

#include "glite/lb/context.h"
#include "glite/lb/events.h"
#include "glite/lb/jobstat.h"

int edg_wll_QueryToHTML(edg_wll_Context,edg_wll_Event *,char **);
int edg_wll_JobStatusToHTML(edg_wll_Context, edg_wll_JobStat, char **);
int edg_wll_UserJobsToHTML(edg_wll_Context, edg_wlc_JobId *, char **);
char *edg_wll_ErrorToHTML(edg_wll_Context,int);

#endif /* __GLITE_LB_HTML_H__ */
