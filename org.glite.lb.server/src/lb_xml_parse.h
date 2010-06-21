#ifndef GLITE_LB_LB_XML_PARSE_H
#define GLITE_LB_LB_XML_PARSE_H

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
#include "glite/lb/notif_rec.h"
#include "glite/lb/query_rec.h"

#ifdef __cplusplus
extern "C" {
#endif
	
/* function for parsing/unparsing XML requests from client */

int parseJobQueryRec(edg_wll_Context ctx, const char *messageBody, long len, edg_wll_QueryRec ***conditions);
int parseQueryJobsRequest(edg_wll_Context ctx, char *messageBody, edg_wll_QueryRec ***conditions, int *flags);
int parseQueryEventsRequest(edg_wll_Context ctx, char *messageBody, edg_wll_QueryRec ***job_conditions, edg_wll_QueryRec ***event_conditions);
int parsePurgeRequest(edg_wll_Context ctx, char *messageBody, int (*tagToIndex)(), edg_wll_PurgeRequest *request);
int parseDumpRequest(edg_wll_Context ctx, char *messageBody, edg_wll_DumpRequest *request);
int parseLoadRequest(edg_wll_Context ctx, char *messageBody, edg_wll_LoadRequest *request);
int parseNotifRequest(edg_wll_Context ctx, char *messageBody, char **function, edg_wll_NotifId *notifId, char **address, edg_wll_NotifChangeOp *op, time_t *validity, edg_wll_QueryRec ***conditions, int *flags);
int parseQuerySequenceCodeRequest(edg_wll_Context ctx, char *messageBody, edg_wlc_JobId *jobId, char **source);
int edg_wll_QueryEventsToXML(edg_wll_Context, edg_wll_Event *, char **);
int edg_wll_QueryJobsToXML(edg_wll_Context, edg_wlc_JobId *, edg_wll_JobStat *, char **);
int edg_wll_JobStatusToXML(edg_wll_Context, edg_wll_JobStat, char **);
int edg_wll_UserJobsToXML(edg_wll_Context, edg_wlc_JobId *, char **);
int edg_wll_PurgeResultToXML(edg_wll_Context ctx, edg_wll_PurgeResult *result, char **message);
int edg_wll_DumpResultToXML(edg_wll_Context ctx, edg_wll_DumpResult *result, char **message);
int edg_wll_LoadResultToXML(edg_wll_Context ctx, edg_wll_LoadResult *result, char **message);
int edg_wll_IndexedAttrsToXML(edg_wll_Context ctx, char **message);
int edg_wll_NotifResultToXML(edg_wll_Context ctx, time_t validity, char **message);
int edg_wll_QuerySequenceCodeResultToXML(edg_wll_Context ctx, char *source, char **message);

int edg_wll_StatsResultToXML(edg_wll_Context,time_t,time_t,float,float,int,int,char **);

int parseStatsRequest(edg_wll_Context,char *,char **,edg_wll_QueryRec ***,edg_wll_JobStatCode *,edg_wll_JobStatCode *,int *,time_t *,time_t *);


#ifdef __cplusplus
}
#endif
	
#endif /* GLITE_LB_LB_XML_PARSE_H */
