#ifndef GLITE_LB_LB_XML_PARSE_V21_H
#define GLITE_LB_LB_XML_PARSE_V21_H

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
#include "glite/lb/jobstat.h"
#include "glite/lb/query_rec.h"

/* function for parsing/unparsing XML requests from client */

int parseQueryJobsRequestV21(edg_wll_Context ctx, char *messageBody, edg_wll_QueryRec ***conditions, int *flags);
int parseQueryEventsRequestV21(edg_wll_Context ctx, char *messageBody, edg_wll_QueryRec ***job_conditions, edg_wll_QueryRec ***event_conditions);
int parsePurgeRequestV21(edg_wll_Context ctx, char *messageBody, edg_wll_PurgeRequest *request);
int parseDumpRequestV21(edg_wll_Context ctx, char *messageBody, edg_wll_DumpRequest *request);
int edg_wll_QueryEventsToXMLV21(edg_wll_Context, edg_wll_Event *, char **);
int edg_wll_QueryJobsToXMLV21(edg_wll_Context, edg_wlc_JobId *, edg_wll_JobStat *, char **);
int edg_wll_JobStatusToXMLV21(edg_wll_Context, edg_wll_JobStat, char **);
int edg_wll_UserJobsToXMLV21(edg_wll_Context, edg_wlc_JobId *, char **);
int edg_wll_PurgeResultToXMLV21(edg_wll_Context ctx, edg_wll_PurgeResult *result, char **message);
int edg_wll_DumpResultToXMLV21(edg_wll_Context ctx, edg_wll_DumpResult *result, char **message);

#endif /* GLITE_LB_LB_XML_PARSE_V21_H */
