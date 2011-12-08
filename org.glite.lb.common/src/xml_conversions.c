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


#include <string.h>
#include <stdlib.h>
#include <expat.h>

#include "glite/lbu/trio.h"
#include "glite/lbu/escape.h"

#include "xml_conversions.h"
#include "jobstat.h"


extern char *edg_wll_TagListToString(edg_wll_TagValue *);

static const struct timeval null_timeval = {0,0};


/************************************************************************/
/* Context manipulation functions 					*/	


void edg_wll_initXMLCtx(edg_wll_XML_ctx *c) {
	c->ctx = NULL;
        c->p = NULL;
	c->message_body = NULL;
        c->eventCode = EDG_WLL_EVENT_UNDEF;
        c->position = 0;
        c->position2 = 0;
	c->max_index = -1;
	c->row = 0;
	c->row2 = 0;
        c->level = 0;
        memset(&(c->element), 0, sizeof(c->element));
        c->char_buf = NULL;
        c->char_buf_len = 0;
        c->XML_tag = NULL;
        c->XML_tag2 = NULL;
	c->job_conditions = NULL;
	c->event_conditions = NULL;
	c->type = EDG_WLL_QUERY_TYPE_UNDEF;
	c->conditions = NULL;
	c->flags = 0;
        c->jobsOutGlobal = NULL;
        c->eventsOutGlobal = NULL;
        c->jobStatGlobal = NULL;
	memset(&(c->jobStatSingleGlobal),0,sizeof(c->jobStatSingleGlobal));
	c->strListGlobal = NULL;
	c->intListGlobal = NULL;
	c->indexToTag	 = NULL;
	c->tagToIndex	 = NULL;
	c->tagListGlobal = NULL;
	c->stsListGlobal = NULL;
	memset(&(c->purgeRequestGlobal),0,sizeof(c->purgeRequestGlobal));
	memset(&(c->purgeResultGlobal),0,sizeof(c->purgeResultGlobal));
	memset(&(c->dumpRequestGlobal),0,sizeof(c->dumpRequestGlobal));
	memset(&(c->dumpResultGlobal),0,sizeof(c->dumpResultGlobal));
	memset(&(c->loadRequestGlobal),0,sizeof(c->loadRequestGlobal));
	memset(&(c->loadResultGlobal),0,sizeof(c->loadResultGlobal));
	c->notifFunction = NULL;
	c->notifClientAddress = NULL;
	c->notifId = NULL;
	c->notifChangeOp = EDG_WLL_NOTIF_NOOP;
	c->notifValidity = -1;
	c->jobId = NULL;
	c->source = NULL;
	c->seqCode = NULL;
	c->attrsGlobal	= NULL;
	c->errCode = 0;
	c->bound = 0;
	c->statsFunction = NULL;
	c->statsConditions = NULL;
	c->statsBaseState = EDG_WLL_JOB_UNDEF;
	c->statsFinalState = EDG_WLL_JOB_UNDEF;
	c->statsMinor = 0;
	c->statsRate = NULL;
	c->statsDuration = NULL;
	c->statsDispersion = NULL;
	c->statsGroup = NULL; 
	c->statsFrom = 0;
	c->statsTo = 0;
	c->statsResFrom = 0;
	c->statsResTo = 0;
	c->errDesc = NULL;
	c->stat_begin = 0;
	c->jobQueryRec_begin = 0;
        c->errtxt = NULL;
	c->warntxt = NULL;
}


void edg_wll_freeXMLCtx(edg_wll_XML_ctx *c) {
	int i;
        if (c->char_buf) free(c->char_buf);
        if (c->errtxt) free(c->errtxt);
        if (c->warntxt) free(c->warntxt);
	if (c->XML_tag) free(c->XML_tag);
	if (c->XML_tag2) free(c->XML_tag2);
	if (c->statsRate) free(c->statsRate);
	if (c->statsDuration) free(c->statsDuration);
	if (c->statsDispersion) free(c->statsDispersion);
	if (c->statsGroup){
		for (i = 0; c->statsGroup[i]; i++)
			free(c->statsGroup[i]);
		free(c->statsGroup);
	}
}


void edg_wll_freeBuf(edg_wll_XML_ctx *c) {
        if (c->char_buf) {
		free(c->char_buf);
	        c->char_buf = NULL;
	}
        c->char_buf_len = 0;
}



/************************************************************************/
/* type to XML conversion functions					*/


/* edg_wll_add_string_to_XMLBody(&body, eventsOut[i].jobMatch.destination, "destination", NULL) */

void edg_wll_add_string_to_XMLBody(char **body, const char *toAdd, const char *tag, const char *null)
{
	if ( toAdd != null ) {
		char *newBody;
		
		trio_asprintf(&newBody,"%s\t\t\t<%s>%|Xs</%s>\r\n", *body, tag, toAdd, tag);

		free(*body);
        	*body = newBody;
	}
}

void edg_wll_add_tagged_string_to_XMLBody(char **body, const char *toAdd, const char *tag, 
					  const char *name, const char *tag2, const char *null)
{
	if ( toAdd != null ) {
		char *newBody;
		
		trio_asprintf(&newBody,"%s\t\t\t<%s %s=\"%|Xs\">%|Xs</%s>\r\n", 
			*body, tag, tag2, name, toAdd, tag);

		free(*body);
        	*body = newBody;
	}
}

/* edg_wll_add_int_to_XMLBody(&body, eventsOut[i].jobMatch.code, "code", 0) */

void edg_wll_add_int_to_XMLBody(char **body, const int toAdd, const char *tag, const int null)
{
	if (toAdd != null) {
                char *newBody;

                trio_asprintf(&newBody,"%s\t\t\t<%s>%|Xd</%s>\r\n", *body, tag, toAdd, tag);

                free(*body);
                *body = newBody;
	}
}


/* edg_wll_add_float_to_XMLBody(&body, rate, "rate", 0) */

void edg_wll_add_float_to_XMLBody(char **body, const float toAdd, const char *tag, const float null)
{
	if (toAdd != null) {
                char *newBody;

                trio_asprintf(&newBody,"%s\t\t\t<%s>%|Xf</%s>\r\n", *body, tag, toAdd, tag);

                free(*body);
                *body = newBody;
	}
}

void edg_wll_add_double_to_XMLBody(char **body, const double toAdd, const char *tag, const double null)
{
	if (toAdd != null) {
                char *newBody;

                trio_asprintf(&newBody,"%s\t\t\t<%s>%|Xf</%s>\r\n", *body, tag, toAdd, tag);

                free(*body);
                *body = newBody;
	}
}


/* edg_wll_add_timeval_to_XMLBody(&body, eventsOut[i].any.tv, "timestamp", -1) */

void edg_wll_add_timeval_to_XMLBody(char **body, struct timeval toAdd, const char *tag, const struct timeval null)
{

	if (toAdd.tv_sec != null.tv_sec || toAdd.tv_usec != null.tv_usec) {
                char *newBody;

                trio_asprintf(&newBody,"%s\t\t\t<%s>%ld.%06ld</%s>\r\n", 
		*body, tag, toAdd.tv_sec, toAdd.tv_usec, tag);

                free(*body);
                *body = newBody;
	}
}


/* edg_wll_add_jobid_to_XMLBody(&body, eventsOut[i].any.jobId, "jobId", NULL) */

void edg_wll_add_jobid_to_XMLBody(char **body, glite_jobid_const_t toAdd, const char *tag, const void *null)
{
	if (toAdd != (edg_wlc_JobId) null) {
                char *newBody, *pom;

                trio_asprintf(&newBody,"%s\t\t\t<%s>%|Xs</%s>\r\n",
                *body, tag, pom = edg_wlc_JobIdUnparse(toAdd), tag);

                free(*body);
		free(pom);
                *body = newBody;
	}
}


void edg_wll_add_notifid_to_XMLBody(char **body, edg_wll_NotifId toAdd, const char *tag, const void *null)
{
	if (toAdd != (edg_wll_NotifId) null) {
                char *newBody, *pom;

                trio_asprintf(&newBody,"%s\t\t\t<%s>%|Xs</%s>\r\n",
                *body, tag, pom = edg_wll_NotifIdUnparse(toAdd), tag);

                free(*body);
		free(pom);
                *body = newBody;
	}
}


/* edg_wll_add_edg_wll_JobStatCode_to_XMLBody(&body, eventsOut[i].any.jobId, "jobId", EDG_WLL_JOB_UNDEF) */

void edg_wll_add_edg_wll_JobStatCode_to_XMLBody(char **body, edg_wll_JobStatCode toAdd, const char *tag, const edg_wll_JobStatCode null)
{
	if (toAdd != null) {
                char *newBody, *pom;

                trio_asprintf(&newBody,"%s\t\t\t<%s>%|Xs</%s>\r\n",
                	*body, tag, pom = edg_wll_StatToString(toAdd), tag);

                free(*body);
		free(pom);
                *body = newBody;
	}
}

void edg_wll_add_edg_wll_EventCode_to_XMLBody(char **body, edg_wll_EventCode toAdd, const char *tag, const edg_wll_EventCode null)
{
	char *newBody, *pom;
	
	if (toAdd != null) {
		trio_asprintf(&newBody,"%s\t\t\t<%s>%|Xs</%s>\r\n",
			*body, tag, pom = edg_wll_EventToString(toAdd), tag);	

		free(*body);
		free(pom);
		*body = newBody;
	}
}


void edg_wll_add_time_t_to_XMLBody(char **body, const time_t toAdd, const char *tag, const time_t null)
{
	if (toAdd != null) {
                char *newBody;

                trio_asprintf(&newBody,"%s\t\t\t<%s>%|Xld</%s>\r\n", *body, tag, toAdd, tag);

                free(*body);
                *body = newBody;
	}
}


void edg_wll_add_tagged_time_t_to_XMLBody(char **body, const time_t toAdd, const char *tag,
                                          const char *name, const char *tag2, const time_t null)
{
        if ( toAdd != null ) {
                char *newBody;

                trio_asprintf(&newBody,"%s\t\t\t<%s %s=\"%|Xs\">%|Xld</%s>\r\n",
                        *body, tag, tag2, name, toAdd, tag);

                free(*body);
                *body = newBody;
        }
}


void edg_wll_add_uint16_t_to_XMLBody(char **body, const uint16_t toAdd, const char *tag, const uint16_t null)
{
	if (toAdd != null) {
                char *newBody;

                trio_asprintf(&newBody,"%s\t\t\t<%s>%|Xu</%s>\r\n", *body, tag, toAdd, tag);

                free(*body);
                *body = newBody;
	}
}


void edg_wll_add_logsrc_to_XMLBody(char **body, const edg_wll_Source toAdd, const char *tag, const edg_wll_Source null)
{
	 if (toAdd != null) {
		char *newBody, *pom;

		trio_asprintf(&newBody,"%s\t\t\t<%s>%|Xs</%s>\r\n",
                        *body, tag, pom = edg_wll_SourceToString(toAdd), tag);
		
		free(*body);
		free(pom);
		*body = newBody;
	}
}

void edg_wll_add_strlist_to_XMLBody(char **body, char * const *toAdd, const char *tag,  const char *subTag, const char *indent,  const  char *null)
{
        char *pomA, *pomB, *newBody;
        char **list = NULL;
        int i = 0, len, tot_len = 0;
        int *len_list = NULL;

	
	if (!toAdd) return;
		
        while (toAdd[i] != null) {
                len = trio_asprintf(&pomA,"%s\t<%s>%|Xs</%s>\r\n",
                        indent,subTag,toAdd[i],subTag);

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

	asprintf(&newBody,"%s%s<%s>\r\n%s%s</%s>\r\n", *body, indent, tag, pomA, indent, tag);
	free(*body);
	free(pomA);
	*body = newBody;	
}

void edg_wll_add_intlist_to_XMLBody(char **body, const int *toAdd, const char *tag, char *(*indexToTag)(), const char *indent, const int from, const int to)
{
        char *pomA, *pomB, *newBody;
        char **list = NULL;
        int i, len, tot_len = 0;
        int *len_list = NULL;
	
	i = from;
        while (i <= to) {
		char *tag2 = indexToTag(i-1);
                len = trio_asprintf(&pomA,"%s\t<%s>%|Xd</%s>\r\n",
                        indent,tag2,toAdd[i],tag2);

		if (tag2) free(tag2);

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

        i = from;
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

	asprintf(&newBody,"%s%s<%s>\r\n%s%s</%s>\r\n", *body, indent, tag, pomA, indent, tag);
        free(*body);
        free(pomA);
        *body = newBody;
}


void edg_wll_add_taglist_to_XMLBody(char **body, const edg_wll_TagValue *toAdd, const char *tag, const edg_wll_TagValue *null)
{
	char *s_taglist;

	s_taglist = edg_wll_TagListToString(toAdd);
	edg_wll_add_string_to_XMLBody(body, s_taglist, tag, NULL);
	free(s_taglist);
}


void edg_wll_add_usertag_to_XMLBody(char **body,  const edg_wll_TagValue *toAdd, const char *tag,  const char *subTag, const char *subTag2, const char *indent, const  char *null)
{
        char *pomA, *pomB, *newBody;
        char **list = NULL;
        int i = 0, len, tot_len = 0;
        int *len_list = NULL;


        while (toAdd && (toAdd[i].tag != null) ) {
                len = trio_asprintf(&pomA,"%s\t<%s %s=\"%|Xs\">%|Xs</%s>\r\n",
                        indent,subTag,subTag2,toAdd[i].tag,toAdd[i].value,subTag);

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

	asprintf(&newBody,"%s%s<%s>\r\n%s%s</%s>\r\n", *body, indent, tag, pomA, indent, tag);
	free(*body);
	free(pomA);
	*body = newBody;	
}


void edg_wll_add_time_t_list_to_XMLBody(char **body, const time_t *toAdd, const char *tag, char *(*indexToTag)(), const char *indent, const int from, const int to)
{
        char *pomA, *pomB, *newBody;
        char **list = NULL;
        int i, len, tot_len = 0;
        int *len_list = NULL;

	
	i = from;
        while (i < to) {
		char *tag2 = indexToTag(i);
                len = trio_asprintf(&pomA,"%s\t<%s>%|Xld</%s>\r\n",
                        indent,tag2,toAdd[i],tag2);

		if (tag2) free(tag2);

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

	asprintf(&newBody,"%s%s<%s>\r\n%s%s</%s>\r\n", *body, indent, tag, pomA, indent, tag);
        free(*body);
        free(pomA);
        *body = newBody;
}

void edg_wll_add_cclassad_to_XMLBody(char **body, void *toAdd, const char *tag, const char *null) {
	//dummy function. This conversion will never be done.
}

// void edg_wll_add_stslist_to_XMLBody(char **body, const edg_wll_JobStat *toAdd, const char *tag, const char *UNUSED_subTag, const int null)
// in lbserver/lb_xml_parse.c.T

/************************************************************************/
/* string to type conversion functions					*/


/* XMLCtx->eventsOutGlobal[XMLCtx->position].any.prog = edg_wll_from_string_to_string(XMLCtx); */
char *edg_wll_from_string_to_string(edg_wll_XML_ctx *XMLCtx)
{
	char *s;

	s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
	edg_wll_freeBuf(XMLCtx);
	return s;
} 


/* XMLCtx->eventsOutGlobal[XMLCtx->position].any.jobId = edg_wll_from_string_to_dgJobId(XMLCtx); */
edg_wlc_JobId edg_wll_from_string_to_jobid(edg_wll_XML_ctx *XMLCtx)
{
	edg_wlc_JobId out = NULL;
	char *s;

	s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
	if (s) {
	   edg_wlc_JobIdParse(s, &out);
	   free(s);
	}
	edg_wll_freeBuf(XMLCtx); 

	return(out);
}



edg_wll_NotifId edg_wll_from_string_to_notifid(edg_wll_XML_ctx *XMLCtx)
{
	edg_wll_NotifId out = NULL;
	char *s;

	s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
	if (s) {
	   edg_wll_NotifIdParse(s, &out);
	   free(s);
	}
	edg_wll_freeBuf(XMLCtx); 

	return(out);
}



edg_wll_JobStatCode edg_wll_from_string_to_edg_wll_JobStatCode(edg_wll_XML_ctx *XMLCtx)
{
	edg_wll_JobStatCode out = EDG_WLL_JOB_UNDEF;
	char *s;
	
	s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
	if (s) {
	   out = edg_wll_StringToStat(s);
	   free(s);
	}
	edg_wll_freeBuf(XMLCtx); 

	return(out);
}




/* XMLCtx->eventsOutGlobal[XMLCtx->position].jobClear.clearReason = 
	edg_wll_from_string_to_int(XMLCtx); 				*/
int edg_wll_from_string_to_int(edg_wll_XML_ctx *XMLCtx)
{
        int out = -1;
	char *s;

	s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
	if (s) {
	   out = atoi(s);
	   free(s);
	}
        edg_wll_freeBuf(XMLCtx);

        return(out);
}



/* XMLCtx->eventsOutGlobal[XMLCtx->position].jobClear.clearReason = 
	edg_wll_from_string_to_int(XMLCtx); 				*/
float edg_wll_from_string_to_float(edg_wll_XML_ctx *XMLCtx)
{
        float out = -1;
	char *s;

	s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
	if (s) {
	   out = strtof(s, (char **) NULL);
	   free(s);
	}
        edg_wll_freeBuf(XMLCtx);

        return(out);
}

double edg_wll_from_string_to_double(edg_wll_XML_ctx *XMLCtx)
{
        double out = -1;
	char *s;

	s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
	if (s) {
	   out = strtod(s, (char **) NULL);
	   free(s);
	}
        edg_wll_freeBuf(XMLCtx);

        return(out);
}


long edg_wll_from_string_to_long(edg_wll_XML_ctx *XMLCtx)
{
        long out = -1;
	char *s;

	s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
	if (s) {
	   out = atol(s);
	   free(s);
	}
        edg_wll_freeBuf(XMLCtx);

        return(out);
}


uint16_t edg_wll_from_string_to_uint16_t(edg_wll_XML_ctx *XMLCtx)
{
        return( (uint16_t) edg_wll_from_string_to_int(XMLCtx) );
}


struct timeval edg_wll_from_string_to_timeval(edg_wll_XML_ctx *XMLCtx)
{
	struct timeval out = {0,0};
	char *needle, *nil;
	char *s;

	s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
	if (s) {
	   out.tv_sec  = strtol(s, &needle, 10);
   	   out.tv_usec = strtol(needle+1, &nil, 10);
	   free(s);
	}
	edg_wll_freeBuf(XMLCtx);

	return(out);
}


time_t edg_wll_from_string_to_time_t(edg_wll_XML_ctx *XMLCtx)
{
        return( (time_t) edg_wll_from_string_to_long(XMLCtx) );
}


edg_wll_Source edg_wll_from_string_to_logsrc(edg_wll_XML_ctx *XMLCtx)
{
	edg_wll_Source	out = EDG_WLL_SOURCE_NONE;
	char *s;

	s = glite_lbu_UnescapeXML((const char *) XMLCtx->char_buf);
	if (s) {
	   out = edg_wll_StringToSource(s);
	   free(s);
	}
	edg_wll_freeBuf(XMLCtx);
	return(out);
}

void *edg_wll_from_string_to_cclassad(edg_wll_XML_ctx *XMLCtx)
{
	//This conversion will never be done
	return (NULL);
}

edg_wll_TagValue *edg_wll_from_string_to_taglist(edg_wll_XML_ctx *XMLCtx)
{
	char *s, *out = NULL;

	s = glite_lbu_UnescapeXML((const char*) XMLCtx->char_buf);
	if(s) {
		edg_wll_TagListParse(s, &out);
	}

	return out;
}

/************************************************************************/
/* various conversion functions					*/


static void append_flag(char **cflags, const char *cflag) {
	char *temp_cflags;

	if (*cflags) {
		asprintf(&temp_cflags, "%s+%s", *cflags, cflag);
		free(*cflags);
		*cflags = temp_cflags;
	} else
		asprintf(cflags, "%s", cflag);
}

char *edg_wll_stat_flags_to_string(int flags)
{
	char *cflags = NULL;

        if (flags & EDG_WLL_STAT_CLASSADS)       append_flag(&cflags, "classadd");
        if (flags & EDG_WLL_STAT_CHILDREN)       append_flag(&cflags, "children");
        if (flags & EDG_WLL_STAT_CHILDSTAT)      append_flag(&cflags, "childstat");
        if (flags & EDG_WLL_STAT_NO_JOBS)        append_flag(&cflags, "no_jobs");
        if (flags & EDG_WLL_STAT_NO_STATES)      append_flag(&cflags, "no_states");
        if (flags & EDG_WLL_STAT_CHILDHIST_FAST) append_flag(&cflags, "childhist_fast");
        if (flags & EDG_WLL_STAT_CHILDHIST_THOROUGH) append_flag(&cflags, "childhist_thorough");
        if (flags & EDG_WLL_NOTIF_BOOTSTRAP)     append_flag(&cflags, "bootstrap");
        if (flags & EDG_WLL_NOTIF_VOLATILE)      append_flag(&cflags, "volatile");
        if (!cflags) cflags = strdup("");

        return(cflags);
}


int edg_wll_string_to_stat_flags(char *cflags)
{
	int flags = 0;
	char *sflag, *last;

	if (cflags == NULL) return 0;

	sflag = strtok_r(cflags, "+", &last);
	while (sflag != NULL) {
		if (!strcmp(sflag,"classadd")) flags = flags | EDG_WLL_STAT_CLASSADS;
		if (!strcmp(sflag,"children")) flags = flags | EDG_WLL_STAT_CHILDREN;
		if (!strcmp(sflag,"childstat")) flags = flags | EDG_WLL_STAT_CHILDSTAT;
		if (!strcmp(sflag,"no_jobs")) flags = flags | EDG_WLL_STAT_NO_JOBS;
		if (!strcmp(sflag,"no_states")) flags = flags | EDG_WLL_STAT_NO_STATES;
                if (!strcmp(sflag,"childhist_fast")) flags = flags | EDG_WLL_STAT_CHILDHIST_FAST;
                if (!strcmp(sflag,"childhist_thorough")) flags = flags | EDG_WLL_STAT_CHILDHIST_THOROUGH;
		if (!strcmp(sflag,"bootstrap")) flags = flags | EDG_WLL_NOTIF_BOOTSTRAP;
		if (!strcmp(sflag,"volatile")) flags = flags | EDG_WLL_NOTIF_VOLATILE;
		sflag = strtok_r(NULL, "+", &last);
	} 

	return(flags);
}


char *edg_wll_purge_flags_to_string(int flags)
{
        char *cflags = NULL, *temp_cflags = NULL;


        if (flags & EDG_WLL_PURGE_REALLY_PURGE) asprintf(&cflags,"%s","really_purge");
        if (flags & EDG_WLL_PURGE_LIST_JOBS) {
                if (cflags) {
                        asprintf(&temp_cflags,"%s+%s",cflags,"list_jobs");
                        free(cflags);
                        cflags=temp_cflags;
                }
                else asprintf(&cflags,"%s","list_jobs");
        }
        if (flags & EDG_WLL_PURGE_SERVER_DUMP) {
                if (cflags) {
                        asprintf(&temp_cflags,"%s+%s",cflags,"server_dump");
                        free(cflags);
                        cflags=temp_cflags;
                }
                else asprintf(&cflags,"%s","server_dump");
        }
        if (flags & EDG_WLL_PURGE_CLIENT_DUMP) {
                if (cflags) {
                        asprintf(&temp_cflags,"%s+%s",cflags,"client_dump");
                        free(cflags);
                        cflags=temp_cflags;
                }
                else asprintf(&cflags,"%s","client_dump");
        }
        if (flags & EDG_WLL_PURGE_BACKGROUND) {
                if (cflags) {
                        asprintf(&temp_cflags,"%s+%s",cflags,"background");
                        free(cflags);
                        cflags=temp_cflags;
                }
                else asprintf(&cflags,"%s","background");
        }

        if (!cflags) cflags = strdup("");

        return(cflags);
}


int edg_wll_string_to_purge_flags(char *cflags)
{
	int flags = 0;
	char *sflag, *last;

	if (cflags == NULL) return 0;

	sflag = strtok_r(cflags, "+", &last);
	while (sflag != NULL) {
		if (!strcmp(sflag,"really_purge")) flags = flags | EDG_WLL_PURGE_REALLY_PURGE;
		if (!strcmp(sflag,"list_jobs")) flags = flags | EDG_WLL_PURGE_LIST_JOBS;
		if (!strcmp(sflag,"server_dump")) flags = flags | EDG_WLL_PURGE_SERVER_DUMP;
		if (!strcmp(sflag,"client_dump")) flags = flags | EDG_WLL_PURGE_CLIENT_DUMP;
		if (!strcmp(sflag,"background")) flags = flags | EDG_WLL_PURGE_BACKGROUND;
		sflag = strtok_r(NULL, "+", &last);
	} 

	return(flags);
}


/* Functions for conversion of DUMP constants */

static const char * const dumpConsts[] = {
	"EDG_WLL_DUMP_NOW",
	"EDG_WLL_DUMP_LAST_START",
	"EDG_WLL_DUMP_LAST_END",
};

int edg_wll_StringToDumpConst(const char *name)

{
        size_t     i;

        for (i=0; i<sizeof(dumpConsts)/sizeof(dumpConsts[0]); i++)
                if (strcasecmp(dumpConsts[i],name) == 0) return -(i+1);
        return 1;
}

char *edg_wll_DumpConstToString(int dumpConst)
{
        if (dumpConst >= 0 || (unsigned int)-(dumpConst) > sizeof(dumpConsts)/sizeof(dumpConsts[0])) return (char *) NULL;
        return strdup(dumpConsts[-(dumpConst+1)]);
}



/* Functions for conversion of DONE CODE */ 

static const char * const done_codeConsts[] = {
	"DONE_CODE_OK",
	"DONE_CODE_FAILED",
	"DONE_CODE_CANCELLED",
};

int edg_wll_StringTodone_code(const char *name)

{
        size_t     i;

        for (i=0; i<sizeof(done_codeConsts)/sizeof(done_codeConsts[0]); i++)
                if (strcasecmp(done_codeConsts[i],name) == 0) return i;
        return 1;
}

char *edg_wll_done_codeToString(int done_codeConst)
{
        if (done_codeConst < 0 || (unsigned int)(done_codeConst) > sizeof(done_codeConsts)/sizeof(done_codeConsts[0])) return (char *) NULL;
        return strdup(done_codeConsts[done_codeConst]);
}



/* Functions for conversion of QUERY ATTRIBUTES */ 

static const char * const query_attrConsts[] = {
        "EDG_WLL_QUERY_ATTR_UNDEF",
        "EDG_WLL_QUERY_ATTR_JOBID",
        "EDG_WLL_QUERY_ATTR_OWNER",
        "EDG_WLL_QUERY_ATTR_STATUS",
        "EDG_WLL_QUERY_ATTR_LOCATION",
        "EDG_WLL_QUERY_ATTR_DESTINATION",
        "EDG_WLL_QUERY_ATTR_DONECODE",
        "EDG_WLL_QUERY_ATTR_USERTAG",
        "EDG_WLL_QUERY_ATTR_TIME",
        "EDG_WLL_QUERY_ATTR_LEVEL",
        "EDG_WLL_QUERY_ATTR_HOST",
        "EDG_WLL_QUERY_ATTR_SOURCE",
        "EDG_WLL_QUERY_ATTR_INSTANCE",
        "EDG_WLL_QUERY_ATTR_EVENT_TYPE",
        "EDG_WLL_QUERY_ATTR_CHKPT_TAG",
        "EDG_WLL_QUERY_ATTR_RESUBMITTED",
        "EDG_WLL_QUERY_ATTR_PARENT",
        "EDG_WLL_QUERY_ATTR_EXITCODE",
	"EDG_WLL_QUERY_ATTR_JDL_ATTR",
	"EDG_WLL_QUERY_ATTR_STATEENTERTIME",
	"EDG_WLL_QUERY_ATTR_LASTUPDATETIME",
        "EDG_WLL_QUERY_ATTR__LAST",
};

edg_wll_QueryAttr edg_wll_StringToquery_attr(const char *name)

{
        size_t     i;

        for (i=0; i<sizeof(query_attrConsts)/sizeof(query_attrConsts[0]); i++)
                if (strcasecmp(query_attrConsts[i],name) == 0) return (edg_wll_QueryAttr) i;
        return EDG_WLL_QUERY_ATTR_UNDEF;
}

char *edg_wll_query_attrToString(edg_wll_QueryAttr query_attrConst)
{
        if (query_attrConst < 0 || (query_attrConst) > sizeof(query_attrConsts)/sizeof(query_attrConsts[0])) return (char *) NULL;
        return strdup(query_attrConsts[(int) query_attrConst]);
}



/* Functions for conversion of NOTIFICATION CHANGE OPERATORS */ 

static const char * const notifChangeOpConsts[] = {
	"EDG_WLL_NOTIF_NOOP",
	"EDG_WLL_NOTIF_REPLACE",
	"EDG_WLL_NOTIF_ADD",
	"EDG_WLL_NOTIF_REMOVE",
};



edg_wll_NotifChangeOp edg_wll_StringToNotifChangeOp(const char *name)
{
        size_t     i;

        for (i=0; i<sizeof(notifChangeOpConsts)/sizeof(notifChangeOpConsts[0]); i++)
                if (strcasecmp(notifChangeOpConsts[i],name) == 0) return (edg_wll_NotifChangeOp) i;
        return EDG_WLL_NOTIF_NOOP;
}



char *edg_wll_NotifChangeOpToString(edg_wll_NotifChangeOp notifChangeOpConst)
{
        if (notifChangeOpConst < 0 || (notifChangeOpConst) > sizeof(notifChangeOpConsts)/sizeof(notifChangeOpConsts[0])) return (char *) NULL;
        return strdup(notifChangeOpConsts[(int) notifChangeOpConst]);
}

