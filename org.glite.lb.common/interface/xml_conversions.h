#ifndef GLITE_LB_XML_CONVERSIONS_H
#define GLITE_LB_XML_CONVERSIONS_H

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
#include "query_rec.h"
#include "notifid.h"
#include "notif_rec.h"
#else
#include "glite/lb/events.h"
#include "glite/lb/query_rec.h"
#include "glite/lb/notifid.h"
#include "glite/lb/notif_rec.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define EDG_WLL_STAT_NO_JOBS    1024	/* private flags for edg_wll_QueryJobs */
#define EDG_WLL_STAT_NO_STATES  2048

enum edg_wll_QueryType {
	EDG_WLL_QUERY_TYPE_UNDEF,
	EDG_WLL_QUERY_TYPE_JOB_CONDITION,
	EDG_WLL_QUERY_TYPE_EVENT_CONDITION,
};	

typedef struct _edg_wll_XML_ctx {
	edg_wll_Context ctx;			
        void		*p;
	char		*message_body;		/* copy of pointer to data to be parsed */		
        edg_wll_EventCode   eventCode;          /* code of processed event */
        int             position;               /* row index in the result list */
                                                /* always should mean 'where to write' */
	int 		position2;		/* used only for event_conditions */
	int		max_index;		/* used in IntList */
	int		row;			/* row index in QueryRec job_conditions */
	int		row2;			/* row index in QueryRec event_conditions */
        int             level;                  /* level of XML tags nesting */
        char            element[50];            /* name of actual XML tag */
        char            *char_buf;              /* for storing 'between tags' data */
        int             char_buf_len;
	char		*XML_tag;		/* for handling *ListOut's tag comparisons */
	char		*XML_tag2;
	edg_wll_QueryRec	**job_conditions;	/* temporal results */
	edg_wll_QueryRec	**event_conditions;
	enum edg_wll_QueryType	type;	
	edg_wll_QueryRec        **conditions;
	int		    	flags;
        glite_jobid_t 	    	*jobsOutGlobal;   
        edg_wll_Event       	*eventsOutGlobal;
        edg_wll_JobStat     	*jobStatGlobal;
        edg_wll_JobStat     	jobStatSingleGlobal;
	char			**strListGlobal;
	int			*intListGlobal;
	int			(*tagToIndex)();
	char			*(*indexToTag)();
	edg_wll_TagValue	*tagListGlobal;
	edg_wll_JobStat		*stsListGlobal;
	edg_wll_PurgeRequest	purgeRequestGlobal;
	edg_wll_PurgeResult	purgeResultGlobal;
	edg_wll_DumpRequest	dumpRequestGlobal;
	edg_wll_DumpResult	dumpResultGlobal;
	edg_wll_LoadRequest	loadRequestGlobal;
	edg_wll_LoadResult	loadResultGlobal;
	edg_wll_QueryRec	**attrsGlobal;
	char			*notifFunction;
	char			*notifClientAddress;
	edg_wll_NotifId		notifId;
	edg_wll_NotifChangeOp	notifChangeOp;
	time_t			notifValidity;
	char                    *statsFunction;
	edg_wll_QueryRec        **statsConditions;
	edg_wll_JobStatCode     statsBaseState;
	edg_wll_JobStatCode     statsFinalState;
	int                     statsMinor;
	time_t                  statsFrom, statsTo;
	float                   *statsRate, *statsDuration, *statsDispersion;
	char			**statsGroup;
	int                     statsResFrom, statsResTo;
	glite_jobid_t		jobId;
	char			*source;
	char			*seqCode;
	int			errCode;
	int			bound;		/* marks 2nd value of within operator */
	char			*errDesc;
	long			stat_begin;	/* begin of jobStat tag */
	long			jobQueryRec_begin;	/* begin of orJobConditions  tag */
        char            *errtxt;                /* variable for cummulating error messages   */
	char		*warntxt;		/* variable for cummulating warning messages */
} edg_wll_XML_ctx;

void edg_wll_initXMLCtx(edg_wll_XML_ctx *c);
void edg_wll_freeXMLCtx(edg_wll_XML_ctx *c);
void edg_wll_freeBuf(edg_wll_XML_ctx *c);


#define edg_wll_add_bool_to_XMLBody edg_wll_add_int_to_XMLBody
#define edg_wll_add_port_to_XMLBody edg_wll_add_uint16_t_to_XMLBody
#define edg_wll_from_string_to_bool edg_wll_from_string_to_uint16_t
#define edg_wll_from_string_to_port edg_wll_from_string_to_int

void edg_wll_add_string_to_XMLBody(char **body, const char *toAdd, const char *tag, const char *null);
void edg_wll_add_tagged_string_to_XMLBody(char **body, const char *toAdd, const char *tag, const char *name, const char *tag2, const char *null);
void edg_wll_add_int_to_XMLBody(char **body, const int toAdd, const char *tag, const int null);
void edg_wll_add_float_to_XMLBody(char **body, const float toAdd, const char *tag, const float null);
void edg_wll_add_double_to_XMLBody(char **body, const double toAdd, const char *tag, const double null);
void edg_wll_add_timeval_to_XMLBody(char **body, struct timeval toAdd, const char *tag, const struct timeval null);
void edg_wll_add_jobid_to_XMLBody(char **body, glite_jobid_const_t toAdd, const char *tag, const void *null);
void edg_wll_add_notifid_to_XMLBody(char **body, edg_wll_NotifId toAdd, const char *tag, const void *null);
void edg_wll_add_edg_wll_JobStatCode_to_XMLBody(char **body, edg_wll_JobStatCode toAdd, const char *tag, const edg_wll_JobStatCode null);
void edg_wll_add_edg_wll_EventCode_to_XMLBody(char **body, edg_wll_EventCode toAdd, const char *tag, const edg_wll_EventCode null);
void edg_wll_add_time_t_to_XMLBody(char **body, const time_t toAdd, const char *tag, const time_t null);
void edg_wll_add_tagged_time_t_to_XMLBody(char **body, const time_t toAdd, const char *tag, const char *name, const char *tag2, const time_t null);
void edg_wll_add_uint16_t_to_XMLBody(char **body, const uint16_t toAdd, const char *tag, const uint16_t null);
void edg_wll_add_logsrc_to_XMLBody(char **body, const edg_wll_Source toAdd, const char *tag, const edg_wll_Source null);
void edg_wll_add_intlist_to_XMLBody(char **body, const int *toAdd, const char *tag, char *(*indexToTag)(), const char *indent, const int from, const int to);
void edg_wll_add_strlist_to_XMLBody(char **body, char * const *toAdd, const char *tag, const char *subTag, const char *indent, const  char *null);
void edg_wll_add_taglist_to_XMLBody(char **body, const edg_wll_TagValue *toAdd, const char *tag, const edg_wll_TagValue *null);
void edg_wll_add_usertag_to_XMLBody(char **body,  const edg_wll_TagValue *toAdd, const char *tag,  const char *subTag, const char *indent, const char *subTag2, const  char *null);
void edg_wll_add_time_t_list_to_XMLBody(char **body, const time_t *toAdd, const char *tag, char *(*indexToTag)(), const char *indent, const int from, const int to);
void edg_wll_add_cclassad_to_XMLBody(char **body, void *toAdd, const char *tag, const char *null);
char *edg_wll_from_string_to_string(edg_wll_XML_ctx *XMLCtx);
glite_jobid_t edg_wll_from_string_to_jobid(edg_wll_XML_ctx *XMLCtx);
edg_wll_NotifId edg_wll_from_string_to_notifid(edg_wll_XML_ctx *XMLCtx);
edg_wll_JobStatCode edg_wll_from_string_to_edg_wll_JobStatCode(edg_wll_XML_ctx *XMLCtx);
int edg_wll_from_string_to_int(edg_wll_XML_ctx *XMLCtx);
float edg_wll_from_string_to_float(edg_wll_XML_ctx *XMLCtx);
double edg_wll_from_string_to_double(edg_wll_XML_ctx *XMLCtx);
long edg_wll_from_string_to_long(edg_wll_XML_ctx *XMLCtx);
uint16_t edg_wll_from_string_to_uint16_t(edg_wll_XML_ctx *XMLCtx);
struct timeval edg_wll_from_string_to_timeval(edg_wll_XML_ctx *XMLCtx);
time_t edg_wll_from_string_to_time_t(edg_wll_XML_ctx *XMLCtx);
edg_wll_Source edg_wll_from_string_to_logsrc(edg_wll_XML_ctx *XMLCtx);
edg_wll_TagValue *edg_wll_from_string_to_taglist(edg_wll_XML_ctx *XMLCtx);

char *edg_wll_stat_flags_to_string(int flags);
int edg_wll_string_to_stat_flags(char *cflags);
char *edg_wll_purge_flags_to_string(int flags);
int edg_wll_string_to_purge_flags(char *cflags);
int edg_wll_StringToDumpConst(const char *name);
char *edg_wll_DumpConstToString(int dumpConst);
int edg_wll_StringTodone_code(const char *name);
char *edg_wll_done_codeToString(int done_codeConst);
edg_wll_QueryAttr edg_wll_StringToquery_attr(const char *name);
char *edg_wll_query_attrToString(edg_wll_QueryAttr query_attrConst);
edg_wll_NotifChangeOp edg_wll_StringToNotifChangeOp(const char *name);
char *edg_wll_NotifChangeOpToString(edg_wll_NotifChangeOp notifChangeOpConst);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* GLITE_LB_XML_CONVERSIONS_H */
