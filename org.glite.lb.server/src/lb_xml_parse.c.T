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


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <expat.h>

#include "glite/jobid/cjobid.h"
#include "glite/lbu/trio.h"
#include "glite/lbu/escape.h"

#include "glite/lb/context-int.h"
#include "glite/lb/xml_conversions.h"
#include "glite/lb/query_rec.h"

#include "lb_xml_parse.h"

#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif

#define QUERY_EVENTS_BEGIN			"<edg_wll_QueryEventsResult"
#define QUERY_EVENTS_END			"</edg_wll_QueryEventsResult>\r\n"
#define QUERY_JOBS_BEGIN			"<edg_wll_QueryJobsResult"
#define QUERY_JOBS_END				"</edg_wll_QueryJobsResult>\r\n"
#define PURGE_RESULT_BEGIN			"<edg_wll_PurgeResult"
#define PURGE_RESULT_END			"</edg_wll_PurgeResult>\r\n"
#define DUMP_RESULT_BEGIN			"<edg_wll_DumpResult"
#define DUMP_RESULT_END				"</edg_wll_DumpResult>\r\n"
#define LOAD_RESULT_BEGIN			"<edg_wll_LoadResult"
#define LOAD_RESULT_END				"</edg_wll_LoadResult>\r\n"
#define INDEXED_ATTRS_BEGIN			"<edg_wll_GetIndexedAttributesResult"
#define INDEXED_ATTRS_END			"</edg_wll_GetIndexedAttributesResult>\r\n"
#define NOTIF_RESULT_BEGIN			"<edg_wll_NotifResult"
#define NOTIF_RESULT_END			"</edg_wll_NotifResult>\r\n"
#define QUERY_SEQUENCE_CODE_RESULT_BEGIN	"<edg_wll_QuerySequenceCodeResult"
#define QUERY_SEQUENCE_CODE_RESULT_END		"</edg_wll_QuerySequenceCodeResult>\r\n"

#define STATS_RESULT_BEGIN	"<edg_wll_StatsResult"
#define STATS_RESULT_END	"</edg_wll_StatsResult>\r\n"
  
  
// XXX will be redundant soon
#define USERJOBS_BEGIN		"<edg_wll_UserJobs"
#define USERJOBS_END		"</edg_wll_UserJobs>\r\n"

static const struct timeval null_timeval = {0,0};

#define unexp() {\
	char	*e;\
\
	if (XMLCtx->errtxt) {\
		asprintf(&e,"%s\nunexpected <%s> at line %d",XMLCtx->errtxt,\
			el,XML_GetCurrentLineNumber(XMLCtx->p));\
		free(XMLCtx->errtxt);\
	} else asprintf(&e,"unexpected <%s> at line %d",\
		el,XML_GetCurrentLineNumber(XMLCtx->p));\
	XMLCtx->errtxt = e;\
}



static void startJobQueryRec(void *data, const char *el, const char **attr)
{
	unsigned int	i;
	edg_wll_XML_ctx *XMLCtx = data;

	
	strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
		case 0: if (strcasecmp(el,"and")) unexp()
			break;
		case 1: if (strcasecmp(el,"orJobConditions")) unexp()
			else {
				XMLCtx->position = -1;
				XMLCtx->job_conditions = realloc(XMLCtx->job_conditions,
					(++XMLCtx->row+2)*sizeof(*XMLCtx->job_conditions));
				XMLCtx->job_conditions[XMLCtx->row] = NULL;
				XMLCtx->job_conditions[XMLCtx->row+1] = NULL;
			}
			break;
		case 2:
			for (i=0; i<EDG_WLL_QUERY_OP__LAST; i++)
				if (!strcasecmp(el,edg_wll_QueryOpNames[i])) break;
			if (i == EDG_WLL_QUERY_OP__LAST) unexp()
			else {
				if (!XMLCtx->job_conditions) break;

				/* malloc also terminator and set it to 0 (= EDG_WLL_QUERY_ATTR_UNDEF) */
				XMLCtx->job_conditions[XMLCtx->row] = realloc(XMLCtx->job_conditions[XMLCtx->row],
					(++XMLCtx->position+2)*sizeof(**XMLCtx->job_conditions));
				memset(&XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position],0,2*sizeof(**XMLCtx->job_conditions));
				XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].op = i;
				XMLCtx->bound = 0;
			}
			break;
		case 3: for (i=1; i<EDG_WLL_QUERY_ATTR__LAST && 
				strcasecmp(el,edg_wll_QueryAttrNames[i]); i++);
			if (i == EDG_WLL_QUERY_ATTR__LAST) unexp()
			else {
				if (!XMLCtx->job_conditions) break;
				if (!XMLCtx->job_conditions[XMLCtx->row]) break;

				if ( (i) == EDG_WLL_QUERY_ATTR_USERTAG) {
					if (!attr[0] || !attr[1]) { unexp() break;}
	                                if (strcmp(attr[0],"name")) { unexp() break;}
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].attr_id.tag = strdup(attr[1]);
				} 
				else if ( (i) == EDG_WLL_QUERY_ATTR_TIME ) {
					if (!attr[0] || !attr[1]) { unexp() break;}
                                        if (attr[0] && strcmp(attr[0],"state")) { unexp() break;}
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].attr_id.state = edg_wll_StringToStat(attr[1]);
				}
				if ( (i) == EDG_WLL_QUERY_ATTR_JDL_ATTR) {
					if (!attr[0] || !attr[1]) 
						XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].attr_id.tag = NULL;
					else if (strcmp(attr[0],"name")) { unexp() break;}
					else XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].attr_id.tag = strdup(attr[1]);
				} 
				XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].attr = i;
			}		
			break;
		default: unexp(); break;
	}
	XMLCtx->level++;
}



static void startQueryJobsRequest(void *data, const char *el, const char **attr)
{
	unsigned int	i;
	edg_wll_XML_ctx *XMLCtx = data;

	
	strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
		case 0:	if (strcasecmp(el,"edg_wll_QueryJobsRequest")) { unexp() break; }
			for ( i = 0; attr[i] && attr[i+1]; i += 2 )
			{
				if ( !strcmp(attr[i],"softLimit") )
					XMLCtx->ctx->softLimit = atoi(attr[i+1]);
				else if ( !strcmp(attr[i],"queryRes") )
					XMLCtx->ctx->p_query_results = atoi(attr[i+1]);
				else { unexp() break; }
			}
			break;
		case 1: if (!strcasecmp(el,"and")) {
				XMLCtx->jobQueryRec_begin = XML_GetCurrentByteIndex(XMLCtx->p);
				break;
			}
			else if (!strcasecmp(el,"flags")) break;
			else unexp()
			break;
		case 2: /* fall through */
		case 3: /* do not check xml tags, processed in startJobQueryRec */
		case 4: break;
		default: unexp(); break;
	}
	XMLCtx->level++;
}


static void startQueryEventsRequest(void *data, const char *el, const char **attr UNUSED_VAR)
{
	unsigned int	i;
	edg_wll_XML_ctx *XMLCtx = data;


	strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
		case 0:	if (strcasecmp(el,"edg_wll_QueryEventsRequest")) { unexp() break; }
			for ( i = 0; attr[i] && attr[i+1]; i += 2 )
			{
				if ( !strcmp(attr[i],"softLimit") )
					XMLCtx->ctx->softLimit = atoi(attr[i+1]);
				else if ( !strcmp(attr[i],"queryRes") )
					XMLCtx->ctx->p_query_results = atoi(attr[i+1]);
				else { unexp() break; }
			}
			break;
		case 1: if (strcasecmp(el,"and")) unexp()
			break;
		case 2: if (!strcasecmp(el,"orJobConditions")) {
				XMLCtx->type = EDG_WLL_QUERY_TYPE_JOB_CONDITION;
				XMLCtx->position = -1;
                                XMLCtx->job_conditions = realloc(XMLCtx->job_conditions,
                                        (++XMLCtx->row+2)*sizeof(*XMLCtx->job_conditions));
                                XMLCtx->job_conditions[XMLCtx->row] = NULL;
                                XMLCtx->job_conditions[XMLCtx->row+1] = NULL;

			}
			else if (!strcasecmp(el,"orEventConditions")) {
				XMLCtx->type = EDG_WLL_QUERY_TYPE_EVENT_CONDITION;
				XMLCtx->position2 = -1;
                                XMLCtx->event_conditions = realloc(XMLCtx->event_conditions,
                                        (++XMLCtx->row2+2)*sizeof(*XMLCtx->event_conditions));
                                XMLCtx->event_conditions[XMLCtx->row2] = NULL;
                                XMLCtx->event_conditions[XMLCtx->row2+1] = NULL;
			}
			else unexp()
			break;
		case 3:
			for (i=0; i<EDG_WLL_QUERY_OP__LAST; i++)
				if (!strcasecmp(el,edg_wll_QueryOpNames[i])) break;
			if (i == EDG_WLL_QUERY_OP__LAST) unexp()
			else if (XMLCtx->type == EDG_WLL_QUERY_TYPE_JOB_CONDITION) {
				if (!XMLCtx->job_conditions) break;

				/* malloc also terminator and set it to 0 (= EDG_WLL_QUERY_ATTR_UNDEF) */
				XMLCtx->job_conditions[XMLCtx->row] = realloc(XMLCtx->job_conditions[XMLCtx->row],
					(++XMLCtx->position+2)*sizeof(**XMLCtx->job_conditions));
				memset(&XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position],0,2*sizeof(**XMLCtx->job_conditions));
				XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].op = i;
				XMLCtx->bound = 0;
			}
			else if (XMLCtx->type == EDG_WLL_QUERY_TYPE_EVENT_CONDITION) {
				if (!XMLCtx->event_conditions) break;
				/* malloc also terminator and set it to 0 (= EDG_WLL_QUERY_ATTR_UNDEF) */
				XMLCtx->event_conditions[XMLCtx->row2] = realloc(XMLCtx->event_conditions[XMLCtx->row2],
					(++XMLCtx->position2+2)*sizeof(**XMLCtx->event_conditions));
				memset(&XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position2],0,2*sizeof(**XMLCtx->event_conditions));
				XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position2].op = i;
				XMLCtx->bound = 0;
			}
			
			break;
		case 4: for (i=1; i<EDG_WLL_QUERY_ATTR__LAST && 
				strcasecmp(el,edg_wll_QueryAttrNames[i]); i++);
			if (i == EDG_WLL_QUERY_ATTR__LAST) unexp()
			else if (XMLCtx->type == EDG_WLL_QUERY_TYPE_JOB_CONDITION) { 
				if (!XMLCtx->job_conditions[XMLCtx->row]) break;
				if ( (i) == EDG_WLL_QUERY_ATTR_USERTAG) {
					if (!attr[0] || !attr[1]) { unexp() break;}
	                                if (attr[0] && strcmp(attr[0],"name")) { unexp() break;}
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].attr_id.tag = strdup(attr[1]);
				} 
				else if ( (i) == EDG_WLL_QUERY_ATTR_TIME ) {
					if (!attr[0] || !attr[1]) { unexp() break;}
                                        if (attr[0] && strcmp(attr[0],"state")) { unexp() break;}
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].attr_id.state = edg_wll_StringToStat(attr[1]);
					//printf("\nchecking time attr\n%s = %s (%d)\n\n", attr[0], attr[1], edg_wll_StringToStat(attr[1]));
				}
				XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].attr = i;
			}
			else if (XMLCtx->type == EDG_WLL_QUERY_TYPE_EVENT_CONDITION) {
				if (!XMLCtx->event_conditions[XMLCtx->row2]) break;
				if ( (i) == EDG_WLL_QUERY_ATTR_USERTAG) {
					if (!attr[0] || !attr[1]) { unexp() break;}
	                                if (attr[0] && strcmp(attr[0],"name")) { unexp() break;}
					XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position].attr_id.tag = strdup(attr[1]);
				} 
				else if ( (i) == EDG_WLL_QUERY_ATTR_TIME ) {
					if (!attr[0] || !attr[1]) { unexp() break;}
                                        if (attr[0] && strcmp(attr[0],"state")) { unexp() break;}
					XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position].attr_id.state = edg_wll_StringToStat(attr[1]);
				}
				XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position2].attr = i;
			}
			break;
		default: unexp(); break;
	}
	XMLCtx->level++;
}



static void startPurgeRequest(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;

	
	strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
		case 0:	if (strcasecmp(el,"edg_wll_PurgeRequest")) unexp()
			break;
		case 1: if (strcasecmp(el,"jobs") && strcasecmp(el,"timeout")
				&& strcasecmp(el,"flags") && strcasecmp(el,"target_runtime")) unexp()
			else 
				XMLCtx->position = 0;
			break;
		case 2: if (!strcasecmp(el,"jobId")) {
				XMLCtx->purgeRequestGlobal.jobs = realloc(XMLCtx->purgeRequestGlobal.jobs,
						(XMLCtx->position+2) * sizeof(XMLCtx->purgeRequestGlobal.jobs));

                                if (!XMLCtx->purgeRequestGlobal.jobs) { 
					edg_wll_SetError(XMLCtx->ctx, ENOMEM, NULL); 
					unexp() return;
				}
                                XMLCtx->purgeRequestGlobal.jobs[XMLCtx->position+1] = NULL;
			}
			else if (XMLCtx->tagToIndex(el) >= 0 ) {
				/* static array, no need to initialize & allocate anything */
			}
			else 
				unexp()
			break;
		default: unexp() 
			 break;
	}
	XMLCtx->level++;
}


static void startDumpRequest(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;

	
	strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
		case 0:	if (strcasecmp(el,"edg_wll_DumpRequest")) unexp()
			break;
		case 1: if (strcasecmp(el,"from") && strcasecmp(el,"to")) unexp()
			break;
		default: unexp() 
			 break;
	}
	XMLCtx->level++;
}



static void startLoadRequest(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;

	
	strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
		case 0:	if (strcasecmp(el,"edg_wll_LoadRequest")) unexp()
			break;
		case 1: if (strcasecmp(el,"server_file")) unexp()
			break;
		default: unexp() 
			 break;
	}
	XMLCtx->level++;
}



static void startNotifRequest(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;

	
	strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
		case 0:	if (!strcasecmp(el,"edg_wll_NotifRequest") && attr[0] && attr[1]) {
				if (strcmp(attr[0],"function")) { unexp() break;}
				else XMLCtx->notifFunction = strdup(attr[1]);
			}
			else unexp()
			break;
		case 1: if (!strcasecmp(el,"and")) {
				XMLCtx->jobQueryRec_begin = XML_GetCurrentByteIndex(XMLCtx->p);
			}
			else if ( (strcasecmp(el,"notifId")) && (strcasecmp(el,"clientAddress")) && 
				(strcasecmp(el,"notifChangeOp")) && (strcasecmp(el,"requestedValidity")) &&
				(strcasecmp(el,"flags")) ) unexp()
			break;
		case 2: /* fall through */
		case 3: /* do not check xml tags, processed in startJobQueryRec */
		case 4: break;
		default: unexp() 
			 break;
	}
	XMLCtx->level++;
}



static void startQuerySequenceCodeRequest(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;

	
	strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
		case 0:	if (strcasecmp(el,"edg_wll_QuerySequenceCodeRequest")) unexp()
			break;
		case 1: if ( (strcasecmp(el,"jobId")) && (strcasecmp(el,"source")) ) unexp()
			break;
		default: unexp() 
			 break;
	}
	XMLCtx->level++;
}



static void startStatsRequest(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;

	
	strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
		case 0:	if (!strcasecmp(el,"edg_wll_StatsRequest") && attr[0] && attr[1]) {
				if (strcmp(attr[0],"function")) { unexp() break;}
				else XMLCtx->statsFunction = strdup(attr[1]);
			}
			else unexp()
			break;
		case 1: if (!strcasecmp(el,"and")) {
				XMLCtx->jobQueryRec_begin = XML_GetCurrentByteIndex(XMLCtx->p);
			}
			else if ( (strcasecmp(el,"base_state")) && (strcasecmp(el,"final_state")) && (strcasecmp(el,"minor")) && 
				(strcasecmp(el,"from")) && (strcasecmp(el,"to")) ) unexp()
			break;
		case 2: /* fall through */
		case 3: /* do not check xml tags, processed in startJobQueryRec */
		case 4: break;
		default: unexp() 
			 break;
	}
	XMLCtx->level++;
}

#undef unexp


static void char_handler(void *data, const char *s, int len)
{
        edg_wll_XML_ctx *XMLCtx = data;
        int i, found = -1;
	char *temp_s;


	/* if date are only spaces, t\, \r, \n ... don't bother with them */
        for (i=0; i<len; i++)	
        	        if (!isspace(s[i])) { found = i; break; }
        if (found == -1) return;

	temp_s = realloc(XMLCtx->char_buf, XMLCtx->char_buf_len + len + 1);
	if (temp_s == NULL) {
	   /* XXX propagate ENOMEM somehow */
	   return;
	}
	XMLCtx->char_buf = temp_s;
	memcpy(XMLCtx->char_buf + XMLCtx->char_buf_len, s, len);
	XMLCtx->char_buf[XMLCtx->char_buf_len + len] = '\0';
	XMLCtx->char_buf_len += len;
}



static void endJobQueryRec(void *data, const char *el UNUSED_VAR)
{
	edg_wll_XML_ctx *XMLCtx = data;
	char *e;
	char *s;

	if (XMLCtx->level == 4 &&
		XMLCtx->job_conditions != NULL &&
		XMLCtx->job_conditions[XMLCtx->row] != NULL) {
		switch (XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].attr) {
			case EDG_WLL_QUERY_ATTR_JOBID:
			case EDG_WLL_QUERY_ATTR_PARENT:
				if ( (XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value.j = 
					edg_wll_from_string_to_jobid(XMLCtx)) == NULL )
				{
					s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
					if (XMLCtx->errtxt) {
						asprintf(&e,"%s\n%s: invalid JobId at line %d",
							XMLCtx->errtxt, s,
							XML_GetCurrentLineNumber(XMLCtx->p));
						free(XMLCtx->errtxt);
					} else asprintf(&e,"%s: invalid JobId at line %d",
						s,XML_GetCurrentLineNumber(XMLCtx->p));
					XMLCtx->errtxt = e;
					free(s);
				}
				break;
			case EDG_WLL_QUERY_ATTR_OWNER:
			// XXX - this is way how to pass NULL, user will be extracted from ssl partner later
				/* XXX char_buf contains an escaped value, however there's nothing to escape in 'NULL' so we're fine */
				if (XMLCtx->char_buf != NULL && !strcmp(XMLCtx->char_buf,"NULL")) {  
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value.c = NULL; 
					break; 
				}
				else /* fall through */
			case EDG_WLL_QUERY_ATTR_LOCATION:
			case EDG_WLL_QUERY_ATTR_DESTINATION:
			case EDG_WLL_QUERY_ATTR_NETWORK_SERVER:
			case EDG_WLL_QUERY_ATTR_USERTAG:
			case EDG_WLL_QUERY_ATTR_JDL_ATTR:
				XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value.c = 
					edg_wll_from_string_to_string(XMLCtx);
				break;
			case EDG_WLL_QUERY_ATTR_STATUS:
				if ( !XMLCtx->bound )
				{
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value.i = 
						edg_wll_from_string_to_edg_wll_JobStatCode(XMLCtx);
					XMLCtx->bound++;
				}
				else
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value2.i = 
						edg_wll_from_string_to_edg_wll_JobStatCode(XMLCtx);

				break;
			case EDG_WLL_QUERY_ATTR_DONECODE:
			case EDG_WLL_QUERY_ATTR_EXITCODE:
			case EDG_WLL_QUERY_ATTR_RESUBMITTED:
				if ( !XMLCtx->bound )
				{
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value.i = 
						edg_wll_from_string_to_int(XMLCtx);
					XMLCtx->bound++;
				}
				else
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value2.i = 
						edg_wll_from_string_to_int(XMLCtx);

				break;
			case EDG_WLL_QUERY_ATTR_TIME:
			case EDG_WLL_QUERY_ATTR_STATEENTERTIME:
			case EDG_WLL_QUERY_ATTR_LASTUPDATETIME:
				if ( !XMLCtx->bound )
				{
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value.t.tv_sec = 
						edg_wll_from_string_to_time_t(XMLCtx);
					XMLCtx->bound++;
				}
				else
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value2.t.tv_sec =
						edg_wll_from_string_to_time_t(XMLCtx);
				break;
			case EDG_WLL_QUERY_ATTR_JOB_TYPE:
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value.i = 
						edg_wll_from_string_to_edg_wll_StatJobtype(XMLCtx);

				break;
			default: 
				edg_wll_freeBuf(XMLCtx);
				XMLCtx->level--;
				s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
				if (XMLCtx->errtxt) {
					asprintf(&e,"%s\n%s: invalid attribute type at line %d",
						XMLCtx->errtxt, s,
						XML_GetCurrentLineNumber(XMLCtx->p));
					free(XMLCtx->errtxt);
				} else asprintf(&e,"%s: invalid attribute type at line %d",
					s,XML_GetCurrentLineNumber(XMLCtx->p));
				XMLCtx->errtxt = e;
				free(s);
				break;
		}
	}
	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}



static void endQueryJobsRequest(void *data, const char *el UNUSED_VAR)
{
	edg_wll_XML_ctx *XMLCtx = data;
	char *s;


	if (XMLCtx->level == 2) {
		if (!strcmp(XMLCtx->element,"flags") && XMLCtx->char_buf) {
			s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
			// XXX: check if it works
			XMLCtx->flags = edg_wll_string_to_stat_flags(s);
			free(s);
		}
		else if (!strcmp(el,"and")) {
			long len = (XML_GetCurrentByteIndex(XMLCtx->p) + XML_GetCurrentByteCount(XMLCtx->p))
					-  XMLCtx->jobQueryRec_begin;

			parseJobQueryRec(XMLCtx->ctx, XMLCtx->message_body + XMLCtx->jobQueryRec_begin, len,
                        	&XMLCtx->job_conditions);
		}
	}
	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}


static void endQueryEventsRequest(void *data, const char *el UNUSED_VAR)
{
	edg_wll_XML_ctx *XMLCtx = data;
	char *e;
	char *s;

        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"orJobConditions")) {
                        /* make end-of-row */
                        XMLCtx->job_conditions[XMLCtx->row] = realloc(XMLCtx->job_conditions[XMLCtx->row],
                                (++XMLCtx->position+1)*sizeof(**XMLCtx->job_conditions));
                        memset(&XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position],0,sizeof(**XMLCtx->job_conditions));
                }
		else if (!strcmp(XMLCtx->element,"orEventConditions")) {
                        /* make end-of-row */
                        XMLCtx->event_conditions[XMLCtx->row2] = realloc(XMLCtx->event_conditions[XMLCtx->row2],
                                (++XMLCtx->position2+1)*sizeof(**XMLCtx->event_conditions));
                        memset(&XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position2],0,sizeof(**XMLCtx->event_conditions));
                }
	}
	else if (XMLCtx->level == 5) {
		if (XMLCtx->type == EDG_WLL_QUERY_TYPE_JOB_CONDITION &&
			XMLCtx->job_conditions != NULL &&
			XMLCtx->job_conditions[XMLCtx->row] !=NULL) {
			switch (XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].attr) {
				case EDG_WLL_QUERY_ATTR_JOBID:
				case EDG_WLL_QUERY_ATTR_PARENT:
					if ( (XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value.j = 
						edg_wll_from_string_to_jobid(XMLCtx)) == NULL )
					{
						s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
						if (XMLCtx->errtxt) {
							asprintf(&e,"%s\n%s: invalid JobId at line %d",
								XMLCtx->errtxt, s,
								XML_GetCurrentLineNumber(XMLCtx->p));
							free(XMLCtx->errtxt);
						} else asprintf(&e,"%s: invalid JobId at line %d",
							s,XML_GetCurrentLineNumber(XMLCtx->p));
						XMLCtx->errtxt = e;
						free(s);
					}
					break;
				case EDG_WLL_QUERY_ATTR_OWNER:
				case EDG_WLL_QUERY_ATTR_LOCATION:
				case EDG_WLL_QUERY_ATTR_DESTINATION:
				case EDG_WLL_QUERY_ATTR_USERTAG:
				case EDG_WLL_QUERY_ATTR_JDL_ATTR:
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value.c = 
						edg_wll_from_string_to_string(XMLCtx);
					break;
				case EDG_WLL_QUERY_ATTR_STATUS:
				case EDG_WLL_QUERY_ATTR_DONECODE:
				case EDG_WLL_QUERY_ATTR_EXITCODE:
				case EDG_WLL_QUERY_ATTR_RESUBMITTED:
					if ( !XMLCtx->bound )
					{
						XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value.i = 
							edg_wll_from_string_to_int(XMLCtx);
						XMLCtx->bound++;
					}
					else
						XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value2.i = 
							edg_wll_from_string_to_int(XMLCtx);
					break;
				case EDG_WLL_QUERY_ATTR_TIME:
				case EDG_WLL_QUERY_ATTR_STATEENTERTIME:
					if ( !XMLCtx->bound )
					{
						XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value.t.tv_sec = 
							edg_wll_from_string_to_time_t(XMLCtx);
						XMLCtx->bound++;
					}
					else
						XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value2.t.tv_sec =
							edg_wll_from_string_to_time_t(XMLCtx);
					break;
				case EDG_WLL_QUERY_ATTR_JOB_TYPE:
						XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].value.i = 
							edg_wll_from_string_to_edg_wll_StatJobtype(XMLCtx);

					break;
				default: 
					edg_wll_freeBuf(XMLCtx);
					XMLCtx->level--;
					s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
					if (XMLCtx->errtxt) {
						asprintf(&e,"%s\n%s: invalid attribute type at line %d",
							XMLCtx->errtxt, s,
							XML_GetCurrentLineNumber(XMLCtx->p));
						free(XMLCtx->errtxt);
					} else asprintf(&e,"%s: invalid attribute type at line %d",
						s,XML_GetCurrentLineNumber(XMLCtx->p));
					XMLCtx->errtxt = e;
					free(s);
					break;
			}
		}
		else if (XMLCtx->type == EDG_WLL_QUERY_TYPE_EVENT_CONDITION &&
			XMLCtx->event_conditions != NULL &&
			XMLCtx->event_conditions[XMLCtx->row2] != NULL) {
			switch (XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position2].attr) {
				case EDG_WLL_QUERY_ATTR_TIME:
					if ( !XMLCtx->bound )
					{
						XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position2].value.t.tv_sec = 
							edg_wll_from_string_to_time_t(XMLCtx);
						XMLCtx->bound++;
					}
					else
						XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position2].value2.t.tv_sec =
							edg_wll_from_string_to_time_t(XMLCtx);
					break;
				case EDG_WLL_QUERY_ATTR_LEVEL:
				case EDG_WLL_QUERY_ATTR_SOURCE:
				case EDG_WLL_QUERY_ATTR_EVENT_TYPE:
					if ( !XMLCtx->bound )
					{
						XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position2].value.i =
							edg_wll_from_string_to_int(XMLCtx);
						XMLCtx->bound++;
					}
					else
						XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position2].value2.i =
							edg_wll_from_string_to_int(XMLCtx);
					break;
				case EDG_WLL_QUERY_ATTR_HOST:
				case EDG_WLL_QUERY_ATTR_INSTANCE:
				case EDG_WLL_QUERY_ATTR_USERTAG:
				case EDG_WLL_QUERY_ATTR_JDL_ATTR:
					XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position2].value.c =
						edg_wll_from_string_to_string(XMLCtx);
					break;
				default: 
					edg_wll_freeBuf(XMLCtx);
					XMLCtx->level--;
					s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
					if (XMLCtx->errtxt) {
						asprintf(&e,"%s\n%s: invalid attribute type at line %d",
							XMLCtx->errtxt, s,
							XML_GetCurrentLineNumber(XMLCtx->p));
						free(XMLCtx->errtxt);
					} else asprintf(&e,"%s: invalid attribute type at line %d",
						s,XML_GetCurrentLineNumber(XMLCtx->p));
					XMLCtx->errtxt = e;
					free(s);
					break;
			}
		} 
	
		edg_wll_freeBuf(XMLCtx);
	}
	XMLCtx->level--;
}


static void endPurgeRequest(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;
        char *e, *s;
	int index;

        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"flags")) {
			s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
			XMLCtx->purgeRequestGlobal.flags = edg_wll_string_to_purge_flags(s);
			free(s);
		} else if (!strcmp(XMLCtx->element,"target_runtime")) {
			XMLCtx->purgeRequestGlobal.target_runtime = edg_wll_from_string_to_time_t(XMLCtx);
		}
	}
	else if (XMLCtx->level == 3) {
		if (!strcmp(XMLCtx->element,"jobId") && XMLCtx->purgeRequestGlobal.jobs != NULL) {
			if ( (XMLCtx->purgeRequestGlobal.jobs[XMLCtx->position++] = 
				edg_wll_from_string_to_string(XMLCtx)) == NULL )
			{
				/* XXX char_buf contains an escaped value, unescaping is done within edg_wll_from_string_to_string(), which failed */
				if (XMLCtx->errtxt) {
					asprintf(&e,"%s\n%s: invalid JobId at line %d",
						XMLCtx->errtxt, XMLCtx->char_buf,
						XML_GetCurrentLineNumber(XMLCtx->p));
					free(XMLCtx->errtxt);
				} else asprintf(&e,"%s: invalid JobId at line %d",
					XMLCtx->char_buf,XML_GetCurrentLineNumber(XMLCtx->p));
				XMLCtx->errtxt = e;
			}
		}
		else if ( ((index = XMLCtx->tagToIndex(XMLCtx->element)) >= 0 ) && XMLCtx->char_buf ) {
			XMLCtx->purgeRequestGlobal.timeout[index] = 
				edg_wll_from_string_to_time_t(XMLCtx);
		}
	}

	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}


static void endDumpRequest(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;
	char *s;

        if (XMLCtx->level == 2) {
		s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
                if (!strcmp(XMLCtx->element,"from")) {
			if (isdigit(s[0]))
			XMLCtx->dumpRequestGlobal.from = edg_wll_from_string_to_time_t(XMLCtx);
			else
				XMLCtx->dumpRequestGlobal.from = edg_wll_StringToDumpConst(s);
		}
		else if (!strcmp(XMLCtx->element,"to")) {
			if (isdigit(s[0]))
                        XMLCtx->dumpRequestGlobal.to = edg_wll_from_string_to_time_t(XMLCtx);
			else
				XMLCtx->dumpRequestGlobal.to = edg_wll_StringToDumpConst(s);
		}
		free(s);
	}

	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}



static void endLoadRequest(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;

        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"server_file")) 
			XMLCtx->loadRequestGlobal.server_file = edg_wll_from_string_to_string(XMLCtx);
	}

	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}



static void endNotifRequest(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;
	char *pom;

        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"notifId")) {
			pom = edg_wll_from_string_to_string(XMLCtx);
			edg_wll_NotifIdParse(pom, &XMLCtx->notifId);
			free(pom);
		}
		else if (!strcmp(XMLCtx->element,"clientAddress")) {
			XMLCtx->notifClientAddress = edg_wll_from_string_to_string(XMLCtx);
		}
		else if (!strcmp(XMLCtx->element,"notifChangeOp")) {
			pom = edg_wll_from_string_to_string(XMLCtx);
			XMLCtx->notifChangeOp = edg_wll_StringToNotifChangeOp(pom);
			free(pom);
		}
		else if (!strcmp(XMLCtx->element,"requestedValidity")) {
			XMLCtx->notifValidity = edg_wll_from_string_to_time_t(XMLCtx);
		}
		else if (!strcmp(el,"and")) {
			long len = (XML_GetCurrentByteIndex(XMLCtx->p) + XML_GetCurrentByteCount(XMLCtx->p))
					-  XMLCtx->jobQueryRec_begin;
	
			parseJobQueryRec(XMLCtx->ctx, XMLCtx->message_body + XMLCtx->jobQueryRec_begin, len,
				&XMLCtx->job_conditions);
		}
		else if (!strcmp(XMLCtx->element,"flags") && XMLCtx->char_buf) {
			char	*s;

			s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
			// XXX: check if it works
			XMLCtx->flags = edg_wll_string_to_stat_flags(s);
			free(s);
		}

	}

	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}



static void endQuerySequenceCodeRequest(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;

        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"jobId")) {
			XMLCtx->jobId = edg_wll_from_string_to_jobid(XMLCtx);
		}
		else if (!strcmp(XMLCtx->element,"source")) {
			XMLCtx->source = edg_wll_from_string_to_string(XMLCtx);
		}
	}

	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}




static void endStatsRequest(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;

        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"base_state")) {
			XMLCtx->statsBaseState = 
				edg_wll_from_string_to_edg_wll_JobStatCode(XMLCtx);
		}
		else if (!strcmp(XMLCtx->element,"final_state")) {
                        XMLCtx->statsFinalState =
                                edg_wll_from_string_to_edg_wll_JobStatCode(XMLCtx);
                }
		else if (!strcmp(XMLCtx->element,"minor")) {
			XMLCtx->statsMinor = edg_wll_from_string_to_int(XMLCtx);
		}
		else if (!strcmp(XMLCtx->element,"from")) {
			XMLCtx->statsFrom = edg_wll_from_string_to_time_t(XMLCtx);;
		}
		else if (!strcmp(XMLCtx->element,"to")) {
			XMLCtx->statsTo = edg_wll_from_string_to_time_t(XMLCtx);;
		}
		else if (!strcmp(el,"and")) {
			long len = (XML_GetCurrentByteIndex(XMLCtx->p) + XML_GetCurrentByteCount(XMLCtx->p))
					-  XMLCtx->jobQueryRec_begin;
	
			parseJobQueryRec(XMLCtx->ctx, XMLCtx->message_body + XMLCtx->jobQueryRec_begin, len,
				&XMLCtx->statsConditions);
		}
	}

	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}



int parseJobQueryRec(edg_wll_Context ctx, const char *messageBody, long len, edg_wll_QueryRec ***conditions)
{
	int	ret;
	edg_wll_XML_ctx 	XMLCtx;
	XML_Char *encoding = "ISO-8859-1";

	errno = 0;
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.row = -1;
	XMLCtx.ctx = ctx;
	edg_wll_ResetError(ctx);


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startJobQueryRec, endJobQueryRec);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, len, 1)) {
                char *errorMessage;

                asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p)));

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((ret = edg_wll_Error(ctx,NULL,NULL))) {
		int i,j;

		if (XMLCtx.job_conditions) {
			for (j = 0; XMLCtx.job_conditions[j]; j++) {
				for (i = 0; (XMLCtx.job_conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
					edg_wll_QueryRecFree(&XMLCtx.job_conditions[j][i]);
				free(XMLCtx.job_conditions[j]);
			}
			free(XMLCtx.job_conditions);
		}

		/* empty list terminators */
		conditions[0] = NULL;
	} else {
		*conditions = XMLCtx.job_conditions;
	}

	
        XML_ParserFree(XMLCtx.p);
	edg_wll_freeXMLCtx(&XMLCtx);
	return ret;
}


/* parse queryJobs request from client */
int parseQueryJobsRequest(edg_wll_Context ctx, char *messageBody, edg_wll_QueryRec ***conditions, int *flags)
{
	int	ret;
	edg_wll_XML_ctx 	XMLCtx;
	XML_Char *encoding = "ISO-8859-1";

	errno = 0;
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.ctx = ctx;
	XMLCtx.message_body = messageBody;
	XMLCtx.position = 0;
	edg_wll_ResetError(ctx);


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startQueryJobsRequest, endQueryJobsRequest);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p)));

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((ret = edg_wll_Error(ctx,NULL,NULL))) {
		int i,j;

		if (XMLCtx.job_conditions) {
			for (j = 0; XMLCtx.job_conditions[j]; j++) {
				for (i = 0; (XMLCtx.job_conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
					edg_wll_QueryRecFree(&XMLCtx.job_conditions[j][i]);
				free(XMLCtx.job_conditions[j]);
			}
			free(XMLCtx.job_conditions);
		}

		/* empty list terminators */
		conditions[0] = NULL;
		*flags = 0;
	} else {
		*conditions = XMLCtx.job_conditions;
		*flags = XMLCtx.flags;
	}

	
        XML_ParserFree(XMLCtx.p);
	edg_wll_freeXMLCtx(&XMLCtx);
	return ret;
}


/* parse queryEvents request from client */
int parseQueryEventsRequest(edg_wll_Context ctx, char *messageBody, edg_wll_QueryRec ***job_conditions, edg_wll_QueryRec ***event_conditions)
{
	int	ret;
	edg_wll_XML_ctx 	XMLCtx;
	XML_Char *encoding = "ISO-8859-1";

	errno = 0;
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.row = -1;
	XMLCtx.row2 = -1;
	XMLCtx.ctx = ctx;
	edg_wll_ResetError(ctx);


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startQueryEventsRequest, endQueryEventsRequest);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p)));

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((ret = edg_wll_Error(ctx,NULL,NULL))) {
		int i,j;

		if (XMLCtx.job_conditions) {
			for (j = 0; XMLCtx.job_conditions[j]; j++) {
				for (i = 0 ; (XMLCtx.job_conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
					edg_wll_QueryRecFree(&XMLCtx.job_conditions[j][i]);
				free(XMLCtx.job_conditions[j]);
			}
			free(XMLCtx.job_conditions);
		}
		if (XMLCtx.event_conditions) {
			for (j = 0; XMLCtx.event_conditions[j]; j++) {
				for (i = 0 ; (XMLCtx.event_conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
					edg_wll_QueryRecFree(&XMLCtx.event_conditions[j][i]);
				free(XMLCtx.event_conditions[j]);
			}
			free(XMLCtx.event_conditions);
		}

		/* empty list terminators */
		job_conditions[0] = NULL;
		event_conditions[0] = NULL;
	} else {
		*job_conditions = XMLCtx.job_conditions;
		*event_conditions = XMLCtx.event_conditions;
	}

	
        XML_ParserFree(XMLCtx.p);
	edg_wll_freeXMLCtx(&XMLCtx);
	return ret;
}



/* parse purge request from client */
int parsePurgeRequest(edg_wll_Context ctx, char *messageBody, int (*tagToIndex)(), edg_wll_PurgeRequest *request)
{
	int	ret;
	edg_wll_XML_ctx 	XMLCtx;
	XML_Char *encoding = "ISO-8859-1";

	errno = 0;
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.ctx = ctx;
	edg_wll_ResetError(ctx);
	XMLCtx.tagToIndex = tagToIndex;


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startPurgeRequest, endPurgeRequest);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p)));

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((ret = edg_wll_Error(ctx,NULL,NULL))) {
		int i;

		if (XMLCtx.purgeRequestGlobal.jobs) {
			for (i=0; XMLCtx.purgeRequestGlobal.jobs[i]; i++) 
				free(XMLCtx.purgeRequestGlobal.jobs[i]);
			free(XMLCtx.purgeRequestGlobal.jobs);
		}
		memset(request,0,sizeof(*request));
		
	} else {
		memcpy(request, &XMLCtx.purgeRequestGlobal, sizeof(XMLCtx.purgeRequestGlobal));
	}

	
        XML_ParserFree(XMLCtx.p);
	edg_wll_freeXMLCtx(&XMLCtx);
	return ret;
}



/* parse dump request from client */
int parseDumpRequest(edg_wll_Context ctx, char *messageBody, edg_wll_DumpRequest *request)
{
	int	ret;
	edg_wll_XML_ctx 	XMLCtx;
	XML_Char *encoding = "ISO-8859-1";

	errno = 0;
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.ctx = ctx;
	edg_wll_ResetError(ctx);


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startDumpRequest, endDumpRequest);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p)));

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((ret = edg_wll_Error(ctx,NULL,NULL))) 
		memset(request,0,sizeof(*request));
		
	else {
		memcpy(request, &XMLCtx.dumpRequestGlobal, sizeof(XMLCtx.dumpRequestGlobal));
	}

	
        XML_ParserFree(XMLCtx.p);
	edg_wll_freeXMLCtx(&XMLCtx);
	return ret;
}

/* parse load request from client */
int parseLoadRequest(edg_wll_Context ctx, char *messageBody, edg_wll_LoadRequest *request)
{
	int	ret;
	edg_wll_XML_ctx 	XMLCtx;
	XML_Char *encoding = "ISO-8859-1";

	errno = 0;
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.ctx = ctx;
	edg_wll_ResetError(ctx);


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startLoadRequest, endLoadRequest);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p)));

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((ret = edg_wll_Error(ctx,NULL,NULL))) 
		memset(request,0,sizeof(*request));
		
	else {
		memcpy(request, &XMLCtx.loadRequestGlobal, sizeof(XMLCtx.loadRequestGlobal));
	}

	
        XML_ParserFree(XMLCtx.p);
	edg_wll_freeXMLCtx(&XMLCtx);
	return ret;
}




/* parse Stats request from client */
int parseStatsRequest(edg_wll_Context ctx, char *messageBody, char **function, edg_wll_QueryRec ***conditions, edg_wll_JobStatCode *base_state, edg_wll_JobStatCode *final_state, int *minor, time_t *from, time_t *to) 
{
	int	ret;
	edg_wll_XML_ctx 	XMLCtx;
	XML_Char *encoding = "ISO-8859-1";

	
	/* returns emty variables as default; only some variables will be filled in */
	/* depending on vaule of XMLCtx.notifFunction */
	*function = NULL;
	*base_state = EDG_WLL_JOB_UNDEF;
	*final_state = EDG_WLL_JOB_UNDEF;
	*minor = 0;
	*from = 0;
	*to = 0;

	errno = 0;
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.ctx = ctx;
	XMLCtx.message_body = messageBody;
	edg_wll_ResetError(ctx);


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startStatsRequest, endStatsRequest);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p)));

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((ret = edg_wll_Error(ctx,NULL,NULL))) {
		int i,j;

		if (XMLCtx.statsConditions) {
			for (j = 0; XMLCtx.statsConditions[j]; j++) {
				for (i = 0; (XMLCtx.statsConditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
					edg_wll_QueryRecFree(&XMLCtx.statsConditions[j][i]);
				free(XMLCtx.statsConditions[j]);
			}
			free(XMLCtx.statsConditions);
		}
		free(XMLCtx.notifFunction);

		*function = NULL;
		*conditions = NULL;
		*base_state = EDG_WLL_JOB_UNDEF;
		*final_state = EDG_WLL_JOB_UNDEF;
		*minor = 0;
		*from = 0;
		*to = 0;
	} 
	else {
		*function = XMLCtx.statsFunction;
		*conditions = XMLCtx.statsConditions;
		*base_state = XMLCtx.statsBaseState;
		*final_state = XMLCtx.statsFinalState;
		*minor = XMLCtx.statsMinor;
		*from = XMLCtx.statsFrom;
		*to = XMLCtx.statsTo;
	}

	
        XML_ParserFree(XMLCtx.p);
	edg_wll_freeXMLCtx(&XMLCtx);
	return ret;
}
/* parse Notif request from client */
int parseNotifRequest(edg_wll_Context ctx, char *messageBody, char **function, edg_wll_NotifId *notifId, char **address, edg_wll_NotifChangeOp *op, time_t *validity, edg_wll_QueryRec ***conditions, int *flags) 
{
	int	ret;
	edg_wll_XML_ctx 	XMLCtx;
	XML_Char *encoding = "ISO-8859-1";

	
	/* returns emty variables as default; only some variables will be filled in */
	/* depending on vaule of XMLCtx.notifFunction */
	*function = NULL;
	*notifId = NULL;
	*address = NULL;
	*op = EDG_WLL_NOTIF_NOOP;
	*conditions = NULL;

	errno = 0;
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.ctx = ctx;
	XMLCtx.message_body = messageBody;
	edg_wll_ResetError(ctx);


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startNotifRequest, endNotifRequest);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p)));

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((ret = edg_wll_Error(ctx,NULL,NULL))) {
		int i,j;

		if (XMLCtx.job_conditions) {
			for (j = 0; XMLCtx.job_conditions[j]; j++) {
				for (i = 0; (XMLCtx.job_conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
					edg_wll_QueryRecFree(&XMLCtx.job_conditions[j][i]);
				free(XMLCtx.job_conditions[j]);
			}
			free(XMLCtx.job_conditions);
		}
		free(XMLCtx.notifFunction);
		edg_wll_NotifIdFree(XMLCtx.notifId);
		free(XMLCtx.notifClientAddress);

		*function = NULL;
		*notifId = NULL;
		*address = NULL;
		*op = EDG_WLL_NOTIF_NOOP;
		*validity = 0;
		*conditions = NULL;
		*flags = 0;
	} else {
		*function = XMLCtx.notifFunction;
		*notifId = XMLCtx.notifId;
		*address = XMLCtx.notifClientAddress;
		*op = XMLCtx.notifChangeOp;
		*validity = XMLCtx.notifValidity;
		*conditions = XMLCtx.job_conditions;
		*flags = XMLCtx.flags;
	}

	
        XML_ParserFree(XMLCtx.p);
	edg_wll_freeXMLCtx(&XMLCtx);
	return ret;
}



/* parse Sequence Code request from client */
int parseQuerySequenceCodeRequest(edg_wll_Context ctx, char *messageBody, edg_wlc_JobId *jobId, char **source)
{
	int	ret;
	edg_wll_XML_ctx 	XMLCtx;
	XML_Char *encoding = "ISO-8859-1";

	errno = 0;
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.ctx = ctx;
	edg_wll_ResetError(ctx);


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startQuerySequenceCodeRequest, endQuerySequenceCodeRequest);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p)));

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((ret = edg_wll_Error(ctx,NULL,NULL))) {
		if (XMLCtx.jobId)
			 edg_wlc_JobIdFree(XMLCtx.jobId);

		*jobId = NULL;
		*source = NULL;
	}
	else {
		*jobId = XMLCtx.jobId;
		*source = XMLCtx.source;
	}
	
	
        XML_ParserFree(XMLCtx.p);
	edg_wll_freeXMLCtx(&XMLCtx);
	return ret;
}




int edg_wll_QueryEventsToXML(edg_wll_Context ctx, edg_wll_Event *eventsOut, char **message)
{
        char *pomA, *pomB;
	char **list = NULL;
        int i = 0, len, tot_len = 0;
	int *len_list = NULL;


        while (eventsOut && eventsOut[i].any.type != EDG_WLL_EVENT_UNDEF) {
		pomB = edg_wll_EventToString(eventsOut[i].any.type);
                trio_asprintf(&pomA,"      <edg_wll_Event name=\"%|Xs\">\r\n", pomB);
		free(pomB);
		pomB = pomA;


@@@{
        selectType $event '_common_';
        for (getFieldsOrdered $event) {
                my $f = selectField $event $_;
                my $ft = $f->{type};
		my $n = $f->{null};
                gen "\tedg_wll_add_$ft\_to_XMLBody(&pomB, eventsOut[i].any.$_, \"$_\", $n);\n";
        }

        gen "\tswitch (eventsOut[i].any.type) {\n";
        for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} } getTypes $event) {
                if ($t ne 'any') {
                        selectType $event $t;
                        my $u = uc $t;
                        gen "\t    case EDG_WLL_EVENT_$u :\n";
                        for (getFieldsOrdered $event) {
                                my $f = selectField $event $_;
                                my $ft = $f->{type};
				my $n = $f->{null};
				$t = lcfirst $t;
				if ($t =~ m/^pBS/) { $t = ucfirst $t; }
				if ($t =~ m/^cREAM/) { $t = ucfirst $t; }
				if ($t =~ m/^condor/) { $t = ucfirst $t; }
                                gen "\t\tedg_wll_add_$ft\_to_XMLBody(&pomB, eventsOut[i].$t.$_, \"$_\", $n);\n";
                        }
                	gen "\t\tbreak;\n";
		}
        }
        gen "\t    default : break;\n";
        gen "\t}\n";
@@@}



                len = asprintf(&pomA,"%s      </edg_wll_Event>\r\n", pomB);
                free(pomB);
                i++;
		tot_len += len;

		list      = (char **) realloc(list, i * sizeof(*list));
		list[i-1] = pomA;
		len_list      = (int *) realloc(len_list, i * sizeof(*len_list));
		len_list[i-1] = len;
		
        }

	/* list termination */
	list = (char **) realloc(list, (i+1) * sizeof(*list));
        list[i] = NULL;

	/* test errors */
	if (ctx->errDesc || ctx->errCode)
		len = trio_asprintf(&pomB," code=\"%d\" desc=\"%|Xs\">\r\n",ctx->errCode,ctx->errDesc);
	else
		len = asprintf(&pomB,">\r\n");

	/* glueing all list fields together & freeing the list */
	pomA = (char *) malloc(tot_len  * sizeof(char) + 
				sizeof(QUERY_EVENTS_BEGIN) + len + sizeof(QUERY_EVENTS_END) - 1);
	
	memcpy(pomA, QUERY_EVENTS_BEGIN, sizeof(QUERY_EVENTS_BEGIN));
	memcpy(pomA + sizeof(QUERY_EVENTS_BEGIN) - 1, pomB, len);
	free(pomB);
	pomB = pomA + sizeof(QUERY_EVENTS_BEGIN) + len - 1;	

	i = 0;
	while (list[i]) {
                memcpy(pomB, list[i], len_list[i] );
		pomB += len_list[i];

		/* freeing the list */
		free(list[i]);

                i++;
        }

	strcpy(pomB, QUERY_EVENTS_END);


	free(list);
	free(len_list);


        *message = pomA;

        return 0;
}

int edg_wll_QueryJobsToXML(edg_wll_Context ctx, edg_wlc_JobId *jobsIn, edg_wll_JobStat *statesIn, char **message) 
{
        char *pomA, *pomB, *pomC;
        char **list = NULL;
        int i = 0, len, tot_len = 0, nres = 0;
        int *len_list = NULL;

	
	if (jobsIn) for (nres = 0; jobsIn[nres]; nres++);
	else if (statesIn) for (nres = 0; statesIn[nres].state; nres++);

        while (i < nres) {
                trio_asprintf(&pomA,"\t<edg_wll_Job>\r\n");
                pomB = pomA;
		
		if (jobsIn) {
			trio_asprintf(&pomA,"%s\t\t<jobId>%|Xs</jobId>\r\n",
        	                pomB, pomC=edg_wlc_JobIdUnparse(jobsIn[i]));
			free(pomC);
			free(pomB);
			pomB = pomA;
		}

		if (statesIn) {
			edg_wll_JobStatusToXML(ctx, statesIn[i], &pomC);	
			trio_asprintf(&pomA,"%s\t\t%s",pomB,pomC);
		}
		else {
			pomC = edg_wll_StatToString(EDG_WLL_JOB_UNKNOWN);
			trio_asprintf(&pomA,"%s\t\t<jobStat name=\"%|Xs\">\r\n\r\n\t\t</jobStat>\r\n",
				pomB, pomC);
		}

        	free(pomB);
		free(pomC);
		pomB = pomA;
       

                len = asprintf(&pomA,"%s\t</edg_wll_Job>\r\n", pomB);
                free(pomB);
                i++;
                tot_len += len;

                list      = (char **) realloc(list, i * sizeof(*list));
                list[i-1] = pomA;
                len_list      = (int *) realloc(len_list, i * sizeof(*len_list));
                len_list[i-1] = len;	
	}
	
	/* list termination */
	list = (char **) realloc(list, (i+1) * sizeof(*list));
        list[i] = NULL;

        /* test errors */
        if (ctx->errDesc || ctx->errCode)
                len = trio_asprintf(&pomB," code=\"%d\" desc=\"%|Xs\">\r\n",ctx->errCode,ctx->errDesc);
        else
                len = asprintf(&pomB,">\r\n");

	/* glueing all list fields together & freeing the list */
	pomA = (char *) malloc(tot_len  * sizeof(char) + 
				sizeof(QUERY_JOBS_BEGIN) + len + sizeof(QUERY_JOBS_END) - 1);
	
	memcpy(pomA, QUERY_JOBS_BEGIN, sizeof(QUERY_JOBS_BEGIN));	
        memcpy((pomA + sizeof(QUERY_JOBS_BEGIN) - 1), pomB, len);
        free(pomB);
	pomB = pomA + sizeof(QUERY_JOBS_BEGIN) + len - 1;	

	i = 0;
	while (list[i]) {
                memcpy(pomB, list[i], len_list[i] );
		pomB += len_list[i];

		/* freeing the list */
		free(list[i]);

                i++;
        }

	strcpy(pomB, QUERY_JOBS_END);


	free(list);
	free(len_list);


        *message = pomA;

	return 0;
}

/* construct Message-Body of Response-Line for edg_wll_UserJobs */
int edg_wll_UserJobsToXML(edg_wll_Context ctx, edg_wlc_JobId *jobsOut, char **message)
{
        char *pomA, *pomB;
        char **list = NULL;
        int i = 0, len, tot_len = 0;
        int *len_list = NULL;


        while (jobsOut[i]) {
		len = trio_asprintf(&pomA,"      <jobId>%|Xs</jobId>\r\n",
			pomB=edg_wlc_JobIdUnparse(jobsOut[i]));

                free(pomB);

                i++;
                tot_len += len;

                list      = (char **) realloc(list, i * sizeof(*list));
                list[i-1] = pomA;
		pomA = NULL;
                len_list      = (int *) realloc(len_list, i * sizeof(*len_list));
                len_list[i-1] = len;

        }

        /* list termination */
        list = (char **) realloc(list, (i+1) * sizeof(*list));
        list[i] = NULL;

        /* test errors */
        if (ctx->errDesc || ctx->errCode)
                len = trio_asprintf(&pomB," code=\"%d\" desc=\"%|Xs\">\r\n",ctx->errCode,ctx->errDesc);
        else
                len = asprintf(&pomB,">\r\n");

        /* glueing all list fields together & freeing the list */
        pomA = (char *) malloc(tot_len  * sizeof(char) +
                                sizeof(USERJOBS_BEGIN) + len + sizeof(USERJOBS_END) - 1);

        memcpy(pomA, USERJOBS_BEGIN, sizeof(USERJOBS_BEGIN));
        memcpy((pomA + sizeof(USERJOBS_BEGIN) - 1), pomB, len);
        free(pomB);
        pomB = pomA + sizeof(USERJOBS_BEGIN) + len - 1;

        i = 0;
        while (list[i]) {
                memcpy(pomB, list[i], len_list[i] );
                pomB += len_list[i];

                /* freeing the list */
                free(list[i]);

                i++;
        }

        strcpy(pomB, USERJOBS_END);


        free(list);
        free(len_list);


        *message = pomA;

        return 0;
}

static void edg_wll_add_stslist_to_XMLBody(edg_wll_Context ctx, char **body, const edg_wll_JobStat *toAdd, const char *tag, const char *UNUSED_subTag, const int null)
{
        char *pomA, *pomB, *newBody;
        char **list = NULL;
        int i = 0, len, tot_len = 0;
        int *len_list = NULL;


        while (toAdd[i].state != null) {
                edg_wll_JobStatusToXML(ctx, toAdd[i], &pomA);
                len = strlen(pomA);

                i++;
                tot_len += len;

                list      = (char **) realloc(list, i * sizeof(*list));
                list[i-1] = pomA;
                pomA = NULL;
                len_list      = (int *) realloc(len_list, i * sizeof(*len_list));
                len_list[i-1] = len;
        }

        /* list termination */
        list = (char **) realloc(list, (i+1) * sizeof(*list));
        list[i] = NULL;

        /* glueing all list fields together & freeing the list */
        pomA = (char *) malloc(tot_len  * sizeof(char) + 1);
	pomB = pomA;

        i = 0;
        while (list[i]) {
                memcpy(pomB, list[i], len_list[i] );
                pomB += len_list[i];

                /* freeing the list */
                free(list[i]);

                i++;
        }
	*pomB = '\0';
        free(list);
        free(len_list);

        asprintf(&newBody,"%s\t\t\t<%s>\r\n%s\t\t\t</%s>\r\n", *body, tag, pomA, tag);
        free(*body);
        free(pomA);
        *body = newBody;
}



/* construct Message-Body of Response-Line for edg_wll_JobStatus */
int edg_wll_JobStatusToXML(edg_wll_Context ctx, edg_wll_JobStat stat, char **message)
{
        char *pomA, *pomB, *pomC;

	
	pomB = strdup("");

@@@{
	selectType $status '_common_';
        for (getFieldsOrdered $status) {
                my $f = selectField $status $_;
		next if defined($f->{special}) && $f->{special} eq 'XMLstructured';
                my $ft = $f->{type};
		my $n = $f->{null};
                gen "edg_wll_add_$ft\_to_XMLBody(&pomB, stat.$_, \"$_\", $n);\n";
        }
@@@}
	if (stat.possible_destinations) edg_wll_add_strlist_to_XMLBody(&pomB, stat.possible_destinations, "possible_destinations", "name", "\t\t\t", NULL);
	if (stat.possible_ce_nodes) edg_wll_add_strlist_to_XMLBody(&pomB, stat.possible_ce_nodes, "possible_ce_nodes", "name", "\t\t\t", NULL);
	if (stat.children) edg_wll_add_strlist_to_XMLBody(&pomB, stat.children, "children", "jobId", "\t\t\t", NULL);
	if (stat.children_hist) edg_wll_add_intlist_to_XMLBody(&pomB, stat.children_hist, "children_hist", edg_wll_StatToString, "\t\t\t", 1, stat.children_hist[0]);
	if (stat.children_states) edg_wll_add_stslist_to_XMLBody(ctx, &pomB, stat.children_states, "children_states", "", EDG_WLL_JOB_UNDEF);
	if (stat.user_tags) edg_wll_add_usertag_to_XMLBody(&pomB, stat.user_tags, "user_tags", "tag", "name", "\t\t\t", NULL);
	if (stat.stateEnterTimes) edg_wll_add_intlist_to_XMLBody(&pomB, stat.stateEnterTimes, "stateEnterTimes", edg_wll_StatToString, "\t\t\t",1, stat.stateEnterTimes[0]);

	pomC = edg_wll_StatToString(stat.state);

	if (ctx->errDesc || ctx->errCode)
        	trio_asprintf(&pomA,"<jobStat name=\"%|Xs\" code=\"%d\" desc=\"%|Xs\">\r\n%s</jobStat>",
                	pomC, ctx->errCode,ctx->errDesc, pomB);
	else
		trio_asprintf(&pomA,"<jobStat name=\"%|Xs\">\r\n%s</jobStat>",
			pomC, pomB);

        free(pomB);
	free(pomC);

        *message = pomA;

        return 0;
}



/* construct Message-Body of Request-Line for edg_wll_Purge */
int edg_wll_PurgeResultToXML(
                edg_wll_Context ctx,
		edg_wll_PurgeResult *result,
                char **message)
{
        char *pomA, *pomB;


	if (!result) { *message = NULL; return(-1); }

	pomA = strdup("");
	edg_wll_add_strlist_to_XMLBody(&pomA, result->jobs, "jobs", "jobId", "\t", NULL);
	edg_wll_add_string_to_XMLBody(&pomA, result->server_file, "server_file", NULL); 

	if (ctx->errDesc || ctx->errCode)
                trio_asprintf(&pomB,"%s code=\"%d\" desc=\"%|Xs\">\r\n%s%s",
			PURGE_RESULT_BEGIN, ctx->errCode, ctx->errDesc, pomA, PURGE_RESULT_END);
	else
		trio_asprintf(&pomB,"%s>\r\n%s%s", PURGE_RESULT_BEGIN, pomA, PURGE_RESULT_END);
	free(pomA);

        *message = pomB;
        return 0;
}



/* construct Message-Body of Request-Line for edg_wll_Dump */
int edg_wll_DumpResultToXML(
                edg_wll_Context ctx,
		edg_wll_DumpResult *result,
                char **message)
{
        char *pomA, *pomB;


	if (!result) { *message = NULL; return(-1); }

	pomA = strdup("");
	edg_wll_add_string_to_XMLBody(&pomA, result->server_file, "server_file", NULL);
        if (result->from < 0)
                edg_wll_add_string_to_XMLBody(&pomA,
                        edg_wll_DumpConstToString(result->from), "from", NULL);
        else
	edg_wll_add_time_t_to_XMLBody(&pomA, result->from, "from", 0);
        if (result->to < 0)
                edg_wll_add_string_to_XMLBody(&pomA,
                        edg_wll_DumpConstToString(result->to), "to", NULL);
        else
	edg_wll_add_time_t_to_XMLBody(&pomA, result->to, "to", 0);

	if (ctx->errDesc || ctx->errCode)
		trio_asprintf(&pomB,"%s code=\"%d\" desc=\"%|Xs\">\r\n%s%s",
			DUMP_RESULT_BEGIN, ctx->errCode, ctx->errDesc, pomA, DUMP_RESULT_END);
	else
		trio_asprintf(&pomB,"%s>\r\n%s%s", DUMP_RESULT_BEGIN, pomA, DUMP_RESULT_END);
	free(pomA);

        *message = pomB;
        return 0;
}


/* construct Message-Body of Request-Line for edg_wll_Load */
int edg_wll_LoadResultToXML(
                edg_wll_Context ctx,
		edg_wll_LoadResult *result,
                char **message)
{
        char *pomA, *pomB;


	if (!result) { *message = NULL; return(-1); }

	pomA = strdup("");
	edg_wll_add_string_to_XMLBody(&pomA, result->server_file, "server_file", NULL);
	edg_wll_add_time_t_to_XMLBody(&pomA, result->from, "from", 0);
	edg_wll_add_time_t_to_XMLBody(&pomA, result->to, "to", 0);

	if (ctx->errDesc || ctx->errCode)
		trio_asprintf(&pomB,"%s code=\"%d\" desc=\"%|Xs\">\r\n%s%s",
			LOAD_RESULT_BEGIN, ctx->errCode, ctx->errDesc, pomA, LOAD_RESULT_END);
	else
		trio_asprintf(&pomB,"%s>\r\n%s%s", LOAD_RESULT_BEGIN, pomA, LOAD_RESULT_END);
	free(pomA);

        *message = pomB;
        return 0;
}


/* construct Message-Body of Request-Line for edg_wll_GetIndexedAttrs */
int edg_wll_IndexedAttrsToXML(
                edg_wll_Context ctx,
                char **message)
{
	int i,j;
	char *pomA, *pomB;

	pomA = strdup("");
	if (ctx->job_index) {
		for (i=0; ctx->job_index[i]; i++) {
			asprintf(&pomB, "%s\t%s\r\n",pomA,"<index>");
			free(pomA);
			pomA = pomB;

			for (j=0; ctx->job_index[i][j].attr != EDG_WLL_QUERY_ATTR_UNDEF; j++) {
				asprintf(&pomB, "%s\t\t%s\r\n",pomA,"<QueryRec>");
				free(pomA);
				pomA = pomB;

				edg_wll_add_string_to_XMLBody(&pomA, 
					edg_wll_query_attrToString(ctx->job_index[i][j].attr), 
					"attribute", NULL);

				if ( ctx->job_index[i][j].attr == EDG_WLL_QUERY_ATTR_TIME )
					edg_wll_add_string_to_XMLBody(&pomA,
						edg_wll_StatToString(ctx->job_index[i][j].attr_id.state),
						"state", NULL);

				if (ctx->job_index[i][j].attr == EDG_WLL_QUERY_ATTR_USERTAG)
					 edg_wll_add_string_to_XMLBody(&pomA,
						ctx->job_index[i][j].attr_id.tag,
						"name", NULL);

				if (ctx->job_index[i][j].attr == EDG_WLL_QUERY_ATTR_JDL_ATTR)
					 edg_wll_add_string_to_XMLBody(&pomA,
						ctx->job_index[i][j].attr_id.tag,
						"name", NULL);

				asprintf(&pomB, "%s\t\t%s\r\n",pomA,"</QueryRec>");
				free(pomA);
				pomA = pomB;
			}
			
			asprintf(&pomB, "%s\t%s\r\n",pomA,"</index>");
			free(pomA);
			pomA = pomB;
		}
	}

	if (ctx->errDesc || ctx->errCode)
		trio_asprintf(&pomB,"%s code=\"%d\" desc=\"%|Xs\">\r\n%s%s",
			INDEXED_ATTRS_BEGIN, ctx->errCode, ctx->errDesc, pomA, INDEXED_ATTRS_END);
	else
		trio_asprintf(&pomB,"%s>\r\n%s%s", INDEXED_ATTRS_BEGIN, pomA, INDEXED_ATTRS_END);
	free(pomA);


        *message = pomB;
        return 0;
}



/* construct Message-Body of Request-Line for edg_wll_Notif */
int edg_wll_NotifResultToXML(
                edg_wll_Context ctx,
		time_t validity,
                char **message)
{
        char *pomA, *pomB;


	pomA = strdup("");
	edg_wll_add_time_t_to_XMLBody(&pomA, validity, "validity", -1);

	if (ctx->errDesc || ctx->errCode)
		trio_asprintf(&pomB,"%s code=\"%d\" desc=\"%|Xs\">\r\n%s%s",
			NOTIF_RESULT_BEGIN, ctx->errCode, ctx->errDesc, pomA, NOTIF_RESULT_END);
	else
		trio_asprintf(&pomB,"%s>\r\n%s%s", NOTIF_RESULT_BEGIN, pomA, NOTIF_RESULT_END);
	free(pomA);

        *message = pomB;
        return 0;
}



/* construct Message-Body of Request-Line for edg_wll_Notif */
int edg_wll_QuerySequenceCodeResultToXML(
                edg_wll_Context ctx,
		char *seqCode,
                char **message)
{
        char *pomA, *pomB;


	pomA = strdup("");
	edg_wll_add_string_to_XMLBody(&pomA, seqCode, "sequence_code", NULL);

	if (ctx->errDesc || ctx->errCode)
		trio_asprintf(&pomB,"%s code=\"%d\" desc=\"%|Xs\">\r\n%s%s",
			QUERY_SEQUENCE_CODE_RESULT_BEGIN, ctx->errCode, 
			ctx->errDesc, pomA, QUERY_SEQUENCE_CODE_RESULT_END);
	else
		trio_asprintf(&pomB,"%s>\r\n%s%s", QUERY_SEQUENCE_CODE_RESULT_BEGIN, 
			pomA, QUERY_SEQUENCE_CODE_RESULT_END);
	free(pomA);

        *message = pomB;
        return 0;
}



/* construct Message-Body of Request-Line for edg_wll_Stats* */
int edg_wll_StatsResultToXML(
                edg_wll_Context ctx,
		time_t from,
		time_t to,
		float *rates,
		float *durations,
		float *dispersions,
		char  **groups,
		int res_from,
		int res_to,
                char **message)
{
        char *pomA, *pomB;
	int i;

	pomA = strdup("");
	edg_wll_add_time_t_to_XMLBody(&pomA, from, "from", -1);
	edg_wll_add_time_t_to_XMLBody(&pomA, to, "to", -1);
	edg_wll_add_int_to_XMLBody(&pomA, res_from, "res_from", -1);
        edg_wll_add_int_to_XMLBody(&pomA, res_to, "res_to", -1);

	if (groups) for (i = 0; groups[i]; i++){
		asprintf(&pomB, "%s\t\t<stat>\n", pomA);
		pomA = pomB;
		edg_wll_add_string_to_XMLBody(&pomA, groups[i], "group", NULL);
		if (rates)
			edg_wll_add_float_to_XMLBody(&pomA, rates[i], "rate", -1.0f);
		if (durations)
			edg_wll_add_float_to_XMLBody(&pomA, durations[i], "duration", -1.0f);
		if (dispersions)
			edg_wll_add_float_to_XMLBody(&pomA, dispersions[i], "dispersion", -1.0f);
		asprintf(&pomB, "%s\t\t</stat>\n", pomA);
		pomA = pomB;
	}

	if (ctx->errDesc || ctx->errCode)
		trio_asprintf(&pomB,"%s code=\"%d\" desc=\"%|Xs\">\r\n%s%s",
			STATS_RESULT_BEGIN, ctx->errCode, ctx->errDesc, pomA, STATS_RESULT_END);
	else
		trio_asprintf(&pomB,"%s>\r\n%s%s", STATS_RESULT_BEGIN, pomA, STATS_RESULT_END);
	free(pomA);
printf("XXX %s XXX\n", pomB);
        *message = pomB;
        return 0;
}


