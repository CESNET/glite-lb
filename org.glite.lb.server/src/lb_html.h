#ifndef GLITE_LB_HTML_H
#define GLITE_LB_HTML_H

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


#include "glite/lb/context.h"
#include "glite/lb/events.h"
#include "glite/lb/jobstat.h"
#include "lb_proto.h"

typedef struct {
	char *id_unparsed;
	char *parent_unparsed;
	char *key;
	int order; } JobIdSorter;

typedef enum _http_output_type{
	HTTP_OUTPUT_TYPE_HTML,
	HTTP_OUTPUT_TYPE_TEXT,
	HTTP_OUTPUT_TYPE_TEXT_APPEND
} http_output_type;

int edg_wll_QueryToHTML(edg_wll_Context,edg_wll_Event *,char **);
int edg_wll_ConfigurationToHTML(edg_wll_Context ctx, int admin, char **message, int text);
int edg_wll_WSDLOutput(edg_wll_Context ctx, char **message, char *filename);
int edg_wll_GeneralJobStatusToHTML(edg_wll_Context, edg_wll_JobStat, char **, int text);
int edg_wll_UserInfoToHTML(edg_wll_Context, edg_wlc_JobId *, edg_wll_JobStat *, char **, int text);
int edg_wll_UserNotifsToHTML(edg_wll_Context ctx, char **notifids, char **message, http_admin_option option, int adm);
int edg_wll_NotificationToHTML(edg_wll_Context ctx, notifInfo *ni, char **message, int text);
char *edg_wll_ErrorToHTML(edg_wll_Context,int);
int edg_wll_StatisticsToHTML(edg_wll_Context ctx, char **message, int text);

#endif /* GLITE_LB_HTML_H */
