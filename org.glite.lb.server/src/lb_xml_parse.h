#ifndef _LB_XML_PARSE_H
#define _LB_XML_PARSE_H

#ident "$Header$"

#include "glite/lb/consumer.h"
#include "glite/lb/notification.h"
#include "glite/lb/purge.h"
#include "glite/lb/dump.h"
#include "glite/lb/load.h"

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
int parseNotifRequest(edg_wll_Context ctx, char *messageBody, char **function, edg_wll_NotifId *notifId, char **address, edg_wll_NotifChangeOp *op, edg_wll_QueryRec ***conditions);
int parseQuerySequenceCodeRequest(edg_wll_Context ctx, char *messageBody, glite_lbu_JobId *jobId, char **source);
int edg_wll_QueryEventsToXML(edg_wll_Context, edg_wll_Event *, char **);
int edg_wll_QueryJobsToXML(edg_wll_Context, glite_lbu_JobId *, edg_wll_JobStat *, char **);
int edg_wll_JobStatusToXML(edg_wll_Context, edg_wll_JobStat, char **);
int edg_wll_UserJobsToXML(edg_wll_Context, glite_lbu_JobId *, char **);
int edg_wll_PurgeResultToXML(edg_wll_Context ctx, edg_wll_PurgeResult *result, char **message);
int edg_wll_DumpResultToXML(edg_wll_Context ctx, edg_wll_DumpResult *result, char **message);
int edg_wll_LoadResultToXML(edg_wll_Context ctx, edg_wll_LoadResult *result, char **message);
int edg_wll_IndexedAttrsToXML(edg_wll_Context ctx, char **message);
int edg_wll_NotifResultToXML(edg_wll_Context ctx, time_t validity, char **message);
int edg_wll_QuerySequenceCodeResultToXML(edg_wll_Context ctx, char *source, char **message);

int edg_wll_StatsResultToXML(edg_wll_Context,time_t,time_t,float,float,int,int,char **);

int parseStatsRequest(edg_wll_Context,char *,char **,edg_wll_QueryRec ***,edg_wll_JobStatCode *,int *,time_t *,time_t *);


#ifdef __cplusplus
}
#endif
	
#endif
