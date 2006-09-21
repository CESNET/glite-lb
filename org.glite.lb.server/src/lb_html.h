#ifndef _LB_HTML
#define _LB_HTML

#ident "$Header$"

#include "glite/lb/consumer.h"

int edg_wll_QueryToHTML(edg_wll_Context,edg_wll_Event *,char **);
int edg_wll_JobStatusToHTML(edg_wll_Context, edg_wll_JobStat, char **);
int edg_wll_UserJobsToHTML(edg_wll_Context, glite_lbu_JobId *, char **);
char *edg_wll_ErrorToHTML(edg_wll_Context,int);

#endif
