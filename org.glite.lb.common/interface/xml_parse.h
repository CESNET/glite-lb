#ifndef GLITE_LB_XML_PARSE_H
#define GLITE_LB_XML_PARSE_H

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


#include "glite/jobid/cjobid.h"

#ifdef BUILDING_LB_COMMON
#include "events.h"
#include "context.h"
#include "query_rec.h"
#include "notifid.h"
#include "notif_rec.h"
#else
#include "glite/lb/events.h"
#include "glite/lb/context.h"
#include "glite/lb/query_rec.h"
#include "glite/lb/notifid.h"
#include "glite/lb/notif_rec.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
	

/* function for parsing XML responses from server */

extern edg_wll_ErrorCode edg_wll_ParseQueryEvents(edg_wll_Context, char *, edg_wll_Event **);

extern edg_wll_ErrorCode edg_wll_ParseQueryJobs(edg_wll_Context, char *, edg_wlc_JobId **, edg_wll_JobStat **);

extern edg_wll_ErrorCode edg_wll_ParseUserJobs(edg_wll_Context, char *, edg_wlc_JobId **); 

extern edg_wll_ErrorCode edg_wll_ParseJobStat(edg_wll_Context ctx, char *messageBody, long len, edg_wll_JobStat *stat);

extern edg_wll_ErrorCode edg_wll_ParseStrList(edg_wll_Context ctx, char *messageBody, long len, char *tag, char *tag2,  char ***strListOut);

extern edg_wll_ErrorCode edg_wll_ParseIntList(edg_wll_Context ctx, char *messageBody, long len, char *tag, int (*tagToIndex)(),  int **intListOut);

extern edg_wll_ErrorCode edg_wll_ParseTagList(edg_wll_Context ctx, char *messageBody, long len, char *tag, char *tag2,  edg_wll_TagValue **tagListOut);

extern edg_wll_ErrorCode edg_wll_ParseStsList(edg_wll_Context ctx, char *messageBody, long len, char *tag, char *tag2,  edg_wll_JobStat **stsListOut);

extern edg_wll_ErrorCode edg_wll_ParsePurgeResult(edg_wll_Context ctx, char *messageBody, edg_wll_PurgeResult *result);

extern edg_wll_ErrorCode edg_wll_ParseDumpResult(edg_wll_Context ctx, char *messageBody, edg_wll_DumpResult *result);

extern edg_wll_ErrorCode edg_wll_ParseLoadResult(edg_wll_Context ctx, char *messageBody, edg_wll_LoadResult *result);

extern edg_wll_ErrorCode edg_wll_ParseIndexedAttrs(edg_wll_Context ctx, char *messageBody, edg_wll_QueryRec ***attrs);

extern edg_wll_ErrorCode edg_wll_ParseNotifResult(edg_wll_Context ctx, char *messageBody, time_t *validity);

extern edg_wll_ErrorCode edg_wll_ParseQuerySequenceCodeResult(edg_wll_Context ctx, char *messageBody, char **seqCode);

extern int edg_wll_QueryEventsRequestToXML(edg_wll_Context ctx, const edg_wll_QueryRec **job_conditions, const edg_wll_QueryRec **event_conditions, char **send_mess);

extern int edg_wll_JobQueryRecToXML(edg_wll_Context ctx, edg_wll_QueryRec const * const *conditions, char **send_mess);

extern int edg_wll_QueryJobsRequestToXML(edg_wll_Context ctx, const edg_wll_QueryRec **conditions, int flags, char **send_mess);

extern int edg_wll_PurgeRequestToXML(edg_wll_Context ctx, const edg_wll_PurgeRequest *request, char **message);

extern int edg_wll_DumpRequestToXML(edg_wll_Context ctx, const edg_wll_DumpRequest *request, char **message);

extern int edg_wll_LoadRequestToXML(edg_wll_Context ctx, const edg_wll_LoadRequest *request, char **message);

extern int edg_wll_IndexedAttrsRequestToXML(edg_wll_Context ctx, char **message);

extern int edg_wll_NotifRequestToXML( edg_wll_Context ctx, const char *function, const edg_wll_NotifId notifId, const char *address, edg_wll_NotifChangeOp op, time_t validity, edg_wll_QueryRec const * const *conditions, int flags, char **message);

extern int edg_wll_QuerySequenceCodeToXML(edg_wll_Context ctx, glite_jobid_const_t jobId, char **message);

extern int edg_wll_StatsRequestToXML(edg_wll_Context,const char *,const edg_wll_QueryRec *,edg_wll_JobStatCode,int,time_t *,time_t *,char **); 

extern int edg_wll_StatsDurationFTRequestToXML(edg_wll_Context,const char *,const edg_wll_QueryRec *,edg_wll_JobStatCode,edg_wll_JobStatCode,int,time_t *,time_t *,char **);
	
extern edg_wll_ErrorCode edg_wll_ParseStatsResult(edg_wll_Context ctx, char *messageBody, time_t *from, time_t *to, float *rate, float *duration, int *res_from, int *res_to);

extern edg_wll_ErrorCode edg_wll_ParseStatsResultFull(edg_wll_Context ctx, char *messageBody, time_t *from, time_t *to, float **rate, float **duration, float **dispersion, char ***group, int *res_from, int *res_to);

#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_XML_PARSE_H */
