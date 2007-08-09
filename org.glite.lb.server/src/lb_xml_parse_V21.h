#ifndef GLITE_LB_LB_XML_PARSE_V21_H
#define GLITE_LB_LB_XML_PARSE_V21_H

#ident "$Header$"

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
