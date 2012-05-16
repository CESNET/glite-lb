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

#include "lb_xml_parse_V21.h"

#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif

#define QUERY_EVENTS_BEGIN	"<edg_wll_QueryEventsResult"
#define QUERY_EVENTS_END	"</edg_wll_QueryEventsResult>\r\n"
#define QUERY_JOBS_BEGIN	"<edg_wll_QueryJobsResult"
#define QUERY_JOBS_END		"</edg_wll_QueryJobsResult>\r\n"
#define PURGE_RESULT_BEGIN	"<edg_wll_PurgeResult"
#define PURGE_RESULT_END	"</edg_wll_PurgeResult>\r\n"
#define DUMP_RESULT_BEGIN	"<edg_wll_DumpResult"
#define DUMP_RESULT_END		"</edg_wll_DumpResult>\r\n"

// XXX will be redundant soon
#define USERJOBS_BEGIN		"<edg_wll_UserJobs"
#define USERJOBS_END		"</edg_wll_UserJobs>\r\n"


static char	*ops[] = { "equal","less","greater","within" },
		*attrs[] = {	"jobid","owner","status","location","destination",
				"donecode","usertag","time","level","host","source",
				"instance","type","chkpt_tag", "resubmitted", "parent_job", "exitcode" };

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


/* Dummy function, returns only const string "el"; just for compatibility */
static char *return_string_el(edg_wll_JobStatCode statCode) {
	return("el");
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
		case 1: if (strcasecmp(el,"and") && strcasecmp(el,"flags")) unexp()
			break;
		case 2: if (strcasecmp(el,"or")) unexp()
			else {
				XMLCtx->position = -1;
				XMLCtx->conditions = realloc(XMLCtx->conditions,
					(++XMLCtx->row+2)*sizeof(*XMLCtx->conditions));
				XMLCtx->conditions[XMLCtx->row] = NULL;
				XMLCtx->conditions[XMLCtx->row+1] = NULL;
			}
			break;
		case 3:
			for (i=0; i<sizeof(ops)/sizeof(ops[0]); i++)
				if (!strcasecmp(el,ops[i])) break;
			if (i == sizeof(ops)/sizeof(ops[0])) unexp()
			else {
				/* malloc also terminator and set it to 0 (= EDG_WLL_QUERY_ATTR_UNDEF) */
				XMLCtx->conditions[XMLCtx->row] = realloc(XMLCtx->conditions[XMLCtx->row],
					(++XMLCtx->position+2)*sizeof(**XMLCtx->conditions));
				memset(&XMLCtx->conditions[XMLCtx->row][XMLCtx->position],0,2*sizeof(**XMLCtx->conditions));
				XMLCtx->conditions[XMLCtx->row][XMLCtx->position].op = i;
				XMLCtx->bound = 0;
			}
			break;
		case 4: for (i=0; i<sizeof(attrs)/sizeof(attrs[0]) && 
				strcasecmp(el,attrs[i]); i++);
			if (i == sizeof(attrs)/sizeof(attrs[0])) unexp()
			else {
				if ( (i+1) == EDG_WLL_QUERY_ATTR_USERTAG) {
					if (!attr[0] || !attr[1]) { unexp() break;}
	                                if (strcmp(attr[0],"name")) { unexp() break;}
					XMLCtx->conditions[XMLCtx->row][XMLCtx->position].attr_id.tag = strdup(attr[1]);
				} 
				else if ( (i+1) == EDG_WLL_QUERY_ATTR_TIME) {
					if (!attr[0] || !attr[1]) { unexp() break;}
                                        if (attr[0] && strcmp(attr[0],"state")) { unexp() break;}
					XMLCtx->conditions[XMLCtx->row][XMLCtx->position].attr_id.state = edg_wll_StringToStat(attr[1]);
				}
				XMLCtx->conditions[XMLCtx->row][XMLCtx->position].attr = i+1;
			}		
			break;
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
			for (i=0; i<sizeof(ops)/sizeof(ops[0]); i++)
				if (!strcasecmp(el,ops[i])) break;
			if (i == sizeof(ops)/sizeof(ops[0])) unexp()
			else if (XMLCtx->type == EDG_WLL_QUERY_TYPE_JOB_CONDITION) {
				/* malloc also terminator and set it to 0 (= EDG_WLL_QUERY_ATTR_UNDEF) */
				XMLCtx->job_conditions[XMLCtx->row] = realloc(XMLCtx->job_conditions[XMLCtx->row],
					(++XMLCtx->position+2)*sizeof(**XMLCtx->job_conditions));
				memset(&XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position],0,2*sizeof(**XMLCtx->job_conditions));
				XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].op = i;
				XMLCtx->bound = 0;
			}
			else if (XMLCtx->type == EDG_WLL_QUERY_TYPE_EVENT_CONDITION) {
				/* malloc also terminator and set it to 0 (= EDG_WLL_QUERY_ATTR_UNDEF) */
				XMLCtx->event_conditions[XMLCtx->row2] = realloc(XMLCtx->event_conditions[XMLCtx->row2],
					(++XMLCtx->position2+2)*sizeof(**XMLCtx->event_conditions));
				memset(&XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position2],0,2*sizeof(**XMLCtx->event_conditions));
				XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position2].op = i;
				XMLCtx->bound = 0;
			}
			
			break;
		case 4: for (i=0; i<sizeof(attrs)/sizeof(attrs[0]) && 
				strcasecmp(el,attrs[i]); i++);
			if (i == sizeof(attrs)/sizeof(attrs[0])) unexp()
			else if (XMLCtx->type == EDG_WLL_QUERY_TYPE_JOB_CONDITION) { 
				if ( (i+1) == EDG_WLL_QUERY_ATTR_USERTAG) {
					if (!attr[0] || !attr[1]) { unexp() break;}
	                                if (attr[0] && strcmp(attr[0],"name")) { unexp() break;}
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].attr_id.tag = strdup(attr[1]);
				} 
				else if ( (i+1) == EDG_WLL_QUERY_ATTR_TIME) {
					if (!attr[0] || !attr[1]) { unexp() break;}
                                        if (attr[0] && strcmp(attr[0],"state")) { unexp() break;}
					XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].attr_id.state = edg_wll_StringToStat(attr[1]);
					//printf("\nchecking time attr\n%s = %s (%d)\n\n", attr[0], attr[1], edg_wll_StringToStat(attr[1]));
				}
				XMLCtx->job_conditions[XMLCtx->row][XMLCtx->position].attr = i+1;
			}
			else if (XMLCtx->type == EDG_WLL_QUERY_TYPE_EVENT_CONDITION) {
				if ( (i+1) == EDG_WLL_QUERY_ATTR_USERTAG) {
					if (!attr[0] || !attr[1]) { unexp() break;}
	                                if (attr[0] && strcmp(attr[0],"name")) { unexp() break;}
					XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position].attr_id.tag = strdup(attr[1]);
				} 
				else if ( (i+1) == EDG_WLL_QUERY_ATTR_TIME) {
					if (!attr[0] || !attr[1]) { unexp() break;}
                                        if (attr[0] && strcmp(attr[0],"state")) { unexp() break;}
					XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position].attr_id.state = edg_wll_StringToStat(attr[1]);
				}
				XMLCtx->event_conditions[XMLCtx->row2][XMLCtx->position2].attr = i+1;
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
				&& strcasecmp(el,"flags")) unexp()
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
			else if (!strcasecmp(el,"time")) {
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



static void endQueryJobsRequest(void *data, const char *el UNUSED_VAR)
{
	edg_wll_XML_ctx *XMLCtx = data;
	char *e, *s;


	if (XMLCtx->level == 2) {
		if (!strcmp(XMLCtx->element,"flags") && XMLCtx->char_buf) {
			s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
// XXX: check if it works
			XMLCtx->flags = edg_wll_string_to_stat_flags(s);
			free(s);
		}
	}
	else if (XMLCtx->level == 5) {
		switch (XMLCtx->conditions[XMLCtx->row][XMLCtx->position].attr) {
			case EDG_WLL_QUERY_ATTR_JOBID:
			case EDG_WLL_QUERY_ATTR_PARENT:
				if ( (XMLCtx->conditions[XMLCtx->row][XMLCtx->position].value.j = 
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
					XMLCtx->conditions[XMLCtx->row][XMLCtx->position].value.c = NULL; 
					break; 
				}
				else /* fall through */
			case EDG_WLL_QUERY_ATTR_LOCATION:
			case EDG_WLL_QUERY_ATTR_DESTINATION:
			case EDG_WLL_QUERY_ATTR_USERTAG:
				XMLCtx->conditions[XMLCtx->row][XMLCtx->position].value.c = 
					edg_wll_from_string_to_string(XMLCtx);
				break;
			case EDG_WLL_QUERY_ATTR_STATUS:
			case EDG_WLL_QUERY_ATTR_DONECODE:
			case EDG_WLL_QUERY_ATTR_EXITCODE:
			case EDG_WLL_QUERY_ATTR_RESUBMITTED:
				if ( !XMLCtx->bound )
				{
					XMLCtx->conditions[XMLCtx->row][XMLCtx->position].value.i = 
						edg_wll_from_string_to_int(XMLCtx);
					XMLCtx->bound++;
				}
				else
					XMLCtx->conditions[XMLCtx->row][XMLCtx->position].value2.i = 
						edg_wll_from_string_to_int(XMLCtx);

				break;
			case EDG_WLL_QUERY_ATTR_TIME:
				if ( !XMLCtx->bound )
				{
					XMLCtx->conditions[XMLCtx->row][XMLCtx->position].value.t.tv_sec = 
						edg_wll_from_string_to_time_t(XMLCtx);
					XMLCtx->bound++;
				}
				else
					XMLCtx->conditions[XMLCtx->row][XMLCtx->position].value2.t.tv_sec =
						edg_wll_from_string_to_time_t(XMLCtx);
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
        XMLCtx->char_buf = NULL;
        XMLCtx->char_buf_len = 0;
	XMLCtx->level--;
}


static void endQueryEventsRequest(void *data, const char *el UNUSED_VAR)
{
	edg_wll_XML_ctx *XMLCtx = data;
	char *e, *s;

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
		if (XMLCtx->type == EDG_WLL_QUERY_TYPE_JOB_CONDITION) {
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
		else if (XMLCtx->type == EDG_WLL_QUERY_TYPE_EVENT_CONDITION) {
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
	
        	XMLCtx->char_buf = NULL;
	        XMLCtx->char_buf_len = 0;
	}
	XMLCtx->level--;
}


static void endPurgeRequest(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;
        char *e;

        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"flags")) 
			XMLCtx->purgeRequestGlobal.flags = edg_wll_from_string_to_int(XMLCtx);
	}
	else if (XMLCtx->level == 3) {
		if (!strcmp(XMLCtx->element,"jobId")) {
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
		else if (!strcmp(XMLCtx->element,"time") && XMLCtx->char_buf) {
			XMLCtx->purgeRequestGlobal.timeout[XMLCtx->position++] = 
				edg_wll_from_string_to_time_t(XMLCtx);
		}
	}

	XMLCtx->char_buf = NULL;
        XMLCtx->char_buf_len = 0;
	XMLCtx->level--;
}


static void endDumpRequest(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;

        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"from")) 
			XMLCtx->dumpRequestGlobal.from = edg_wll_from_string_to_time_t(XMLCtx);
		else if (!strcmp(XMLCtx->element,"to")) 
                        XMLCtx->dumpRequestGlobal.to = edg_wll_from_string_to_time_t(XMLCtx);
	}

	XMLCtx->char_buf = NULL;
        XMLCtx->char_buf_len = 0;
	XMLCtx->level--;
}

/* parse queryJobs request from client */
int parseQueryJobsRequestV21(edg_wll_Context ctx, char *messageBody, edg_wll_QueryRec ***conditions, int *flags)
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

		if (XMLCtx.conditions) {
			for (j = 0; XMLCtx.conditions[j]; j++) {
				for (i = 0; (XMLCtx.conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
					edg_wll_QueryRecFree(&XMLCtx.conditions[j][i]);
				free(XMLCtx.conditions[j]);
			}
			free(XMLCtx.conditions);
		}

		/* empty list terminators */
		conditions[0] = NULL;
		*flags = 0;
	} else {
		*conditions = XMLCtx.conditions;
		*flags = XMLCtx.flags;
	}

	
        XML_ParserFree(XMLCtx.p);
	edg_wll_freeXMLCtx(&XMLCtx);
	return ret;
}


/* parse queryEvents request from client */
int parseQueryEventsRequestV21(edg_wll_Context ctx, char *messageBody, edg_wll_QueryRec ***job_conditions, edg_wll_QueryRec ***event_conditions)
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
int parsePurgeRequestV21(edg_wll_Context ctx, char *messageBody, edg_wll_PurgeRequest *request)
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
int parseDumpRequestV21(edg_wll_Context ctx, char *messageBody, edg_wll_DumpRequest *request)
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



int edg_wll_QueryEventsToXMLV21(edg_wll_Context ctx UNUSED_VAR, edg_wll_Event *eventsOut, char **message)
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

int edg_wll_QueryJobsToXMLV21(edg_wll_Context ctx, edg_wlc_JobId *jobsIn, edg_wll_JobStat *statesIn, char **message) 
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
			edg_wll_JobStatusToXMLV21(ctx, statesIn[i], &pomC);	
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
int edg_wll_UserJobsToXMLV21(edg_wll_Context ctx UNUSED_VAR, edg_wlc_JobId *jobsOut, char **message)
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

static void edg_wll_add_stslist_to_XMLBodyV21(edg_wll_Context ctx, char **body, const edg_wll_JobStat *toAdd, const char *tag, const char *UNUSED_subTag, const int null)
{
        char *pomA, *pomB, *newBody;
        char **list = NULL;
        int i = 0, len, tot_len = 0;
        int *len_list = NULL;


        while (toAdd[i].state != null) {
                edg_wll_JobStatusToXMLV21(ctx, toAdd[i], &pomA);
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
int edg_wll_JobStatusToXMLV21(edg_wll_Context ctx, edg_wll_JobStat stat, char **message)
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
	if (stat.children) edg_wll_add_strlist_to_XMLBody(&pomB, stat.children, "children", "jobId", "\t\t\t", NULL);
	if (stat.children_hist) edg_wll_add_intlist_to_XMLBody(&pomB, stat.children_hist, "children_hist", return_string_el, "\t\t\t", 1, stat.children_hist[0]);
	if (stat.children_states) edg_wll_add_stslist_to_XMLBodyV21(ctx, &pomB, stat.children_states, "children_states", "", EDG_WLL_JOB_UNDEF);
	if (stat.user_tags) edg_wll_add_usertag_to_XMLBody(&pomB, stat.user_tags, "user_tags", "tag", "name", "\t\t\t", NULL);
	if (stat.stateEnterTimes) edg_wll_add_intlist_to_XMLBody(&pomB, stat.stateEnterTimes, "stateEnterTimes", return_string_el, "\t\t\t",1, stat.stateEnterTimes[0]);

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
int edg_wll_PurgeResultToXMLV21(
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
                trio_asprintf(&pomA,"%s code=\"%d\" desc=\"%|Xs\">\r\n%s%s",
			PURGE_RESULT_BEGIN, ctx->errCode, ctx->errDesc, pomA, PURGE_RESULT_END);
	else
		trio_asprintf(&pomB,"%s>\r\n%s%s", PURGE_RESULT_BEGIN, pomA, PURGE_RESULT_END);
	free(pomA);

        *message = pomB;
        return 0;
}



/* construct Message-Body of Request-Line for edg_wll_Dump */
int edg_wll_DumpResultToXMLV21(
                edg_wll_Context ctx,
		edg_wll_DumpResult *result,
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
			DUMP_RESULT_BEGIN, ctx->errCode, ctx->errDesc, pomA, DUMP_RESULT_END);
	else
		trio_asprintf(&pomB,"%s>\r\n%s%s", DUMP_RESULT_BEGIN, pomA, DUMP_RESULT_END);
	free(pomA);

        *message = pomB;
        return 0;
}
