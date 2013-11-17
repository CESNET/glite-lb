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
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <expat.h> 	// Expat header file

#include "glite/lbu/trio.h"
#include "glite/lbu/escape.h"
#include "glite/jobid/cjobid.h"

#include "context-int.h"
#include "xml_parse.h"
#include "xml_conversions.h"    


#ifdef __GNUC__ 
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR 
#endif

#define QUERY_EVENTS_REQUEST_BEGIN		"<edg_wll_QueryEventsRequest"
#define QUERY_EVENTS_REQUEST_END		"\t</and>\r\n</edg_wll_QueryEventsRequest>"
#define QUERY_EVENTS_OREC_BEGIN			"\t\t<orEventConditions>\r\n"
#define QUERY_EVENTS_OREC_END			"\t\t</orEventConditions>\r\n"
#define QUERY_EVENTS_ORJC_BEGIN			"\t\t<orJobConditions>\r\n"
#define QUERY_EVENTS_ORJC_END			"\t\t</orJobConditions>\r\n"
#define QUERY_JOBS_REQUEST_BEGIN		"<edg_wll_QueryJobsRequest"
#define QUERY_JOBS_REQUEST_END			"\t</and>\r\n</edg_wll_QueryJobsRequest>"
#define QUERY_JOBS_OR_BEGIN			"\t\t<orJobConditions>\r\n"
#define QUERY_JOBS_OR_END			"\t\t</orJobConditions>\r\n"
#define PURGE_REQUEST_BEGIN			"<edg_wll_PurgeRequest>\r\n"
#define PURGE_REQUEST_END			"</edg_wll_PurgeRequest>\r\n"
#define DUMP_REQUEST_BEGIN			"<edg_wll_DumpRequest>\r\n"
#define DUMP_REQUEST_END			"</edg_wll_DumpRequest>\r\n"
#define LOAD_REQUEST_BEGIN			"<edg_wll_LoadRequest>\r\n"
#define LOAD_REQUEST_END			"</edg_wll_LoadRequest>\r\n"
#define INDEXED_ATTRS_REQUEST_BEGIN		"<edg_wll_IndexedAttrsRequest>\r\n"
#define INDEXED_ATTRS_REQUEST_END		"</edg_wll_IndexedAttrsRequest>\r\n"
#define NOTIF_REQUEST_BEGIN			"<edg_wll_NotifRequest"
#define NOTIF_REQUEST_END			"</edg_wll_NotifRequest>\r\n"
#define STATS_REQUEST_BEGIN           "<edg_wll_StatsRequest"
#define STATS_REQUEST_END             "</edg_wll_StatsRequest>\r\n"
#define QUERY_SEQUENCE_CODE_REQUEST_BEGIN	"<edg_wll_QuerySequenceCodeRequest>\r\n"
#define QUERY_SEQUENCE_CODE_REQUEST_END		"</edg_wll_QuerySequenceCodeRequest>\r\n"


/* lists of accepted tags */
static const char * const jobStatTags[] = {
@@@{
	selectType $status '_common_';
        for (getFieldsOrdered $status) {
                gen "\t\"$_\",\n";
        }
@@@}
        NULL
};


@@@{
                gen "static const char * const eventJobCommon\[] = {";
                selectType $event '_common_';
                for (getFieldsOrdered $event) {
                        my $f = selectField $event $_;
                        my $fn = $f->{name};
                        gen "\"$fn\",  ";
                }
                gen "NULL };\n";
@@@}
@@@{
        for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
                        $event->getTypes) {
                gen "static const char * const event$t\[] = {";
                selectType $event $t;
                for (getFieldsOrdered $event) {
                        my $f = selectField $event $_;
                        my $fn = $f->{name};
                        gen "\"$fn\",  "
                }
                gen "NULL };\n";
        }
@@@}



static const char * const * const eventTags[] = {
        eventJobCommon,
@@@{
        for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
                        $event->getTypes) {
                gen "\tevent$t,\n";
        }
        gen "\tNULL\n};\n";
@@@}



/* used when error unrecoverable*/
#define unexpError() {\
        char    *e;\
	int      ret; \
\
        if (XMLCtx->errtxt) {\
                ret = asprintf(&e,"%s\nunexpected <%s> at line %d",XMLCtx->errtxt,\
                        XMLCtx->element,(int)XML_GetCurrentLineNumber(XMLCtx->p));\
                free(XMLCtx->errtxt);\
        } else ret = asprintf(&e,"unexpected <%s> at line %d",\
                XMLCtx->element,(int)XML_GetCurrentLineNumber(XMLCtx->p));\
        XMLCtx->errtxt = (ret == -1 ? NULL : e);\
}



static void emptyCall(void){
}

/* used when error recoverable	*/
// XXX quadratic complexity - problematic when lots of errors in long outputs (10.000's)
#define unexpWarning() {\
        char    *e;\
        int      ret;\
\
emptyCall(); \
        if (XMLCtx->warntxt) {\
                ret = asprintf(&e,"%s\nunexpected <%s> at line %d",XMLCtx->warntxt,\
                        XMLCtx->element,(int)XML_GetCurrentLineNumber(XMLCtx->p));\
                free(XMLCtx->warntxt);\
        } else ret = asprintf(&e,"unexpected <%s> at line %d",\
                XMLCtx->element,(int)XML_GetCurrentLineNumber(XMLCtx->p));\
        XMLCtx->warntxt = (ret == -1 ? NULL : e);\
}




static void startQueryJobs(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;

        
	if (XMLCtx->char_buf) edg_wll_freeBuf(XMLCtx);

        strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
                case 0 : if (strcmp("edg_wll_QueryJobsResult", el)) { unexpError() break;}
			 if (attr[0] && attr[1] && attr[2] && attr[3]) {
			 	if (strcmp(attr[0],"code")) { unexpError() break;}
                                else XMLCtx->errCode = atoi(attr[1]);

                                if (strcmp(attr[2],"desc")) { unexpError() break;}
                                else XMLCtx->errDesc = strdup(attr[3]);
                         }
			 break;  

		case 1 : if (strcmp("edg_wll_Job", el)) unexpError()
                  	 break;

		case 2 : if (!strcmp("jobId", el)) {
				/* allocates space only for pointers to structures edg_wlc_jobid_t */
				/* space for structures allocate characterUserJobs         */
   				XMLCtx->jobsOutGlobal = realloc(XMLCtx->jobsOutGlobal,
                                                        (XMLCtx->position+1)*sizeof(*XMLCtx->jobsOutGlobal));
				if (!XMLCtx->jobsOutGlobal) { edg_wll_SetError(XMLCtx->ctx, ENOMEM, NULL); unexpError() return; }
				XMLCtx->jobsOutGlobal[XMLCtx->position] = NULL;
			 }
			 else if (!strcmp("jobStat", el)) {
				XMLCtx->stat_begin = XML_GetCurrentByteIndex(XMLCtx->p);
				XMLCtx->jobStatGlobal = (edg_wll_JobStat *) realloc(XMLCtx->jobStatGlobal,
								(XMLCtx->position2+1)*sizeof(*XMLCtx->jobStatGlobal));

			 }
			 else unexpWarning()
			 break;

		// XXX ?? this may be usefull with status
		case 3 : /* fall through */
		case 4 : /* do not check xml tags, try to be faul-tolerant 		*/
			 /* if tag not found during unparsing, xmlMalformed flag is set */
			 break;

		default: if (!XMLCtx->stat_begin) unexpWarning()
			 break;
	}

	XMLCtx->level++;
}



static void startQueryEvents(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;

        
	if (XMLCtx->char_buf) edg_wll_freeBuf(XMLCtx);

        strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
                case 0 : if (strcmp("edg_wll_QueryEventsResult", el)) { unexpError() break;}
                         if (attr[0] && attr[1] && attr[2] && attr[3]) {
                                if (strcmp(attr[0],"code")) { unexpError() break;}
                                else XMLCtx->errCode = atoi(attr[1]);

                                if (strcmp(attr[2],"desc")) { unexpError() break;}
                                else XMLCtx->errDesc = strdup(attr[3]);
                         }

			 break;  

		case 1 : if (strcmp("edg_wll_Event", el)) unexpError()
                         else {
                         	XMLCtx->position++;
				
				if (!attr[0] || !attr[1]) { unexpError() break;}
				if (strcmp(attr[0],"name")) { unexpError() break;}
                                if ( (XMLCtx->eventCode = edg_wll_StringToEvent((char *) attr[1])) 
					== EDG_WLL_EVENT_UNDEF ) { unexpWarning() }
                                XMLCtx->eventsOutGlobal = realloc(XMLCtx->eventsOutGlobal,
                                    			  (XMLCtx->position+1)*sizeof(*XMLCtx->eventsOutGlobal));
                               	if (!XMLCtx->eventsOutGlobal) { edg_wll_SetError(XMLCtx->ctx, ENOMEM, NULL); unexpError() return; }
                               	memset(&(XMLCtx->eventsOutGlobal)[XMLCtx->position],0,sizeof(*XMLCtx->eventsOutGlobal));
                                XMLCtx->eventsOutGlobal[XMLCtx->position].any.type  = XMLCtx->eventCode;
                              	}
                  	 break;

		case 2 : /* do not check xml tags, try to be faul-tolerant 		*/
			 /* if tag not found during unparsing, xmlMalformed flag is set */
			 break;

		default: unexpWarning()
			 break;
	}

	XMLCtx->level++;
}




static void startUserJobs(void *data, const char *el, const char **attr UNUSED_VAR)
{
	edg_wll_XML_ctx *XMLCtx = data;


	if (XMLCtx->char_buf) edg_wll_freeBuf(XMLCtx);

        strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
                case 0 : if (strcmp("edg_wll_UserJobs", el)) { unexpError() break;}
                         if (attr[0] && attr[1] && attr[2] && attr[3]) {
                                if (strcmp(attr[0],"code")) { unexpError() break;}
                                else XMLCtx->errCode = atoi(attr[1]);

                                if (strcmp(attr[2],"desc")) { unexpError() break;}
                              	else XMLCtx->errDesc = strdup(attr[3]);
                         }
			 break;  

		case 1 : if (strcmp("jobId", el)) unexpError()
			 else {
				/* allocates space only for pointers to structures edg_wlc_jobid_t */
				/* space for structures allocate endUserJobs                       */
   				XMLCtx->jobsOutGlobal = realloc(XMLCtx->jobsOutGlobal,
                                                        (XMLCtx->position+1)*sizeof(XMLCtx->jobsOutGlobal));
				if (!XMLCtx->jobsOutGlobal) { edg_wll_SetError(XMLCtx->ctx, ENOMEM, NULL); unexpError() return; }
				XMLCtx->jobsOutGlobal[XMLCtx->position] = NULL;
			 }
			 break;


		default: unexpWarning()
			 break;
	}

	XMLCtx->level++;
}



static void startJobStatus(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;
	edg_wll_JobStatCode	statusCode; 		/* code of status in process */


	if (XMLCtx->char_buf) edg_wll_freeBuf(XMLCtx);

        strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
                case 0 : if (strcmp("jobStat", el)) { unexpError() break;}
			 if (!attr[0] || !attr[1]) { unexpError() break;}
			 if (strcmp(attr[0],"name")) { unexpError() break;}

			 if ( (statusCode = edg_wll_StringToStat(attr[1])) 
				== (edg_wll_JobStatCode)-1 ) 
				/* status unknown, but try to get as much as possible */
				unexpWarning()
			 else {
				if (edg_wll_InitStatus(&XMLCtx->jobStatSingleGlobal)) 
					{ edg_wll_SetError(XMLCtx->ctx, ENOMEM, NULL); unexpError() return; }
				XMLCtx->jobStatSingleGlobal.state = statusCode;
			 }

                         if (attr[2] && attr[3] && attr[4] && attr[5]) {
                                if (strcmp(attr[2],"code")) { unexpError() break;}
                                else XMLCtx->errCode = atoi(attr[3]);

                                if (strcmp(attr[4],"desc")) { unexpError() break;}
                                else XMLCtx->errDesc = strdup(attr[5]);
                         }
                         break;

		case 1 : if (!strcmp("user_tags", el) || !strcmp("user_values", el) 
				|| !strcmp("children_hist", el) ||  !strcmp("stateEnterTimes", el)
				|| !strcmp("possible_destinations", el) 
				|| !strcmp("possible_ce_nodes", el)
				|| !strcmp("children_states", el) || !strcmp("children", el)) {
				XMLCtx->stat_begin = XML_GetCurrentByteIndex(XMLCtx->p);
			 }
			 break;

		case 2 : /* fall through */
		case 3 : /* fall through */
		case 4 : /* do not check xml tags, try to be faul-tolerant 		*/
			 /* if tag not found during unparsing, xmlMalformed flag is set */
			 break;


		default: unexpWarning() 
			 break;
	}

	XMLCtx->level++;
}


static void startStrList(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;


	if (XMLCtx->char_buf) edg_wll_freeBuf(XMLCtx);

        strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
                case 0 : if (strcmp(XMLCtx->XML_tag, el)) unexpError()
			 break;  

		case 1 : if (!strcmp(XMLCtx->XML_tag2, el)) {
				XMLCtx->strListGlobal = realloc(XMLCtx->strListGlobal,
                                                        (XMLCtx->position+1)*sizeof(*XMLCtx->strListGlobal));
                                if (!XMLCtx->strListGlobal) { edg_wll_SetError(XMLCtx->ctx, ENOMEM, NULL); unexpError() return; }
                                XMLCtx->strListGlobal[XMLCtx->position] = NULL;

			 }
			 break;

		default: unexpWarning() 
			 break;
	}

	XMLCtx->level++;
}



static void startIntList(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;
	int index = 0;


	if (XMLCtx->char_buf) edg_wll_freeBuf(XMLCtx);

        strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
                case 0 : if (strcmp(XMLCtx->XML_tag, el)) unexpError()
			 break;  

		case 1 : if ( (index = XMLCtx->tagToIndex(el)) >= 0 ) {
				if (index > XMLCtx->max_index) {
					XMLCtx->max_index = index;
					/* leave zeros' possition for array length */
					XMLCtx->intListGlobal = realloc(XMLCtx->intListGlobal,
                                                        (XMLCtx->max_index+2)*sizeof(*XMLCtx->intListGlobal));
         	                       if (!XMLCtx->intListGlobal) {edg_wll_SetError(XMLCtx->ctx, ENOMEM, NULL); unexpError() return; }
	                                XMLCtx->intListGlobal[XMLCtx->max_index+1] = 0;
				}

			 }
			 break;

		default: unexpWarning() 
			 break;
	}

	XMLCtx->level++;
}


static void startTagList(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;


	if (XMLCtx->char_buf) edg_wll_freeBuf(XMLCtx);

        strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
                case 0 : if (strcmp(XMLCtx->XML_tag, el)) unexpError()
			 break;  

		case 1 : if (!attr[0] || !attr[1]) { unexpError() break;}
			 if (strcmp(attr[0],"name")) { unexpError() break;}

			 if (!strcmp(XMLCtx->XML_tag2, el)) {
				XMLCtx->tagListGlobal = realloc(XMLCtx->tagListGlobal,
                                                        (XMLCtx->position+1)*sizeof(*XMLCtx->tagListGlobal));
                                if (!XMLCtx->tagListGlobal) { edg_wll_SetError(XMLCtx->ctx, ENOMEM, NULL); unexpError() return; }
				XMLCtx->tagListGlobal[XMLCtx->position].tag = strdup(attr[1]);
			 	XMLCtx->stat_begin = XML_GetCurrentByteIndex(XMLCtx->p);
			 }
			 break;

		default: unexpWarning() 
			 break;
	}

	XMLCtx->level++;
}


static void startStsList(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;


	if (XMLCtx->char_buf) edg_wll_freeBuf(XMLCtx);

        strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
                case 0 : if (strcmp(XMLCtx->XML_tag, el)) unexpError()
			 break;  

		case 1 : if (!strcmp(XMLCtx->XML_tag2, el)) {
				XMLCtx->stsListGlobal = realloc(XMLCtx->stsListGlobal,
                                                        (XMLCtx->position+1)*sizeof(*XMLCtx->stsListGlobal));
                                if (!XMLCtx->stsListGlobal) { edg_wll_SetError(XMLCtx->ctx, ENOMEM, NULL); unexpError() return; }
                                // edg_wll_InitStatus(&XMLCtx->stsListGlobal[XMLCtx->position]);
				// initialized in startJobStatus();

			 	XMLCtx->stat_begin = XML_GetCurrentByteIndex(XMLCtx->p);
			 }
			 break;

		case 2 : /* fall through */
		case 3 : /* fall through */
		case 4 : /* fall through */
		case 5 : /* do not check xml tags ut to depth 5 */
			 /* will be checked later		*/
			 break;

		default: unexpWarning() 
			 break;
	}

	XMLCtx->level++;
}


static void startPurgeResult(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;
	int	i;

	
	strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
		case 0:	if (strcasecmp(el,"edg_wll_PurgeResult")) { unexpError() break;}
			for ( i = 0; attr[i] && attr[i+1]; i += 2 ) {
				if (!strcmp(attr[i],"code"))
					XMLCtx->errCode = atoi(attr[i+1]);
				else if (!strcmp(attr[i],"desc"))
					XMLCtx->errDesc = strdup(attr[i+1]);
				else { unexpError() }
			}
			break;
		case 1: if (strcasecmp(el,"jobs") && strcasecmp(el,"server_file")) unexpWarning()
			break;
		case 2: if (!strcasecmp(el,"jobId")) {
				XMLCtx->purgeResultGlobal.jobs = realloc(XMLCtx->purgeResultGlobal.jobs,
						(XMLCtx->position+2) * sizeof(XMLCtx->purgeResultGlobal.jobs));

                                if (!XMLCtx->purgeResultGlobal.jobs) { 
					edg_wll_SetError(XMLCtx->ctx, ENOMEM, NULL); 
					unexpError() return;
				}
                                XMLCtx->purgeResultGlobal.jobs[XMLCtx->position+1] = NULL;
			}
			else 
				unexpWarning()
			break;
		default: unexpWarning() 
			 break;
	}
	XMLCtx->level++;
}



static void startDumpResult(void *data, const char *el, const char **attr)
{
	edg_wll_XML_ctx *XMLCtx = data;
	int	i;

	
	strcpy(XMLCtx->element, el);

	switch (XMLCtx->level) {
		case 0:	if (strcasecmp(el,"edg_wll_DumpResult")) { unexpError() break;}
			for ( i = 0; attr[i] && attr[i+1]; i += 2 ) {
				if (!strcmp(attr[i],"code"))
					XMLCtx->errCode = atoi(attr[i+1]);
				else if (!strcmp(attr[i],"desc"))
					XMLCtx->errDesc = strdup(attr[i+1]);
				else { unexpError() }
			}
			break;
		case 1: if (strcasecmp(el,"from") && strcasecmp(el,"to") 
				&& strcasecmp(el,"server_file")) unexpWarning()
			break;
		default: unexpWarning() 
			 break;
	}
	XMLCtx->level++;
}



static void startLoadResult(void *data, const char *el, const char **attr)
{
        edg_wll_XML_ctx *XMLCtx = data;
        int     i;

        
        strcpy(XMLCtx->element, el);
        
        switch (XMLCtx->level) {
                case 0: if (strcasecmp(el,"edg_wll_LoadResult")) { unexpError() break;}
                        for ( i = 0; attr[i] && attr[i+1]; i += 2 ) {
                                if (!strcmp(attr[i],"code"))
                                        XMLCtx->errCode = atoi(attr[i+1]);
                                else if (!strcmp(attr[i],"desc"))
                                        XMLCtx->errDesc = strdup(attr[i+1]);
                                else { unexpError() }
                        }       
                        break;          
                case 1: if (strcasecmp(el,"from") && strcasecmp(el,"to")
                                && strcasecmp(el,"server_file")) unexpWarning()
                        break;  
                default: unexpWarning()
                         break;
        }       
        XMLCtx->level++;
}



static void startIndexedAttrs(void *data, const char *el, const char **attr)
{
        edg_wll_XML_ctx *XMLCtx = data;
        int     i;

        
        strcpy(XMLCtx->element, el);
        
        switch (XMLCtx->level) {
                case 0: if (strcasecmp(el,"edg_wll_GetIndexedAttributesResult")) { unexpError() break;}
                        for ( i = 0; attr[i] && attr[i+1]; i += 2 ) {
                                if (!strcmp(attr[i],"code"))
                                        XMLCtx->errCode = atoi(attr[i+1]);
                                else if (!strcmp(attr[i],"desc"))
                                        XMLCtx->errDesc = strdup(attr[i+1]);
                                else { unexpError() }
                        }       
                        break;          
                case 1: if (!strcasecmp(el,"index")) {
   				XMLCtx->attrsGlobal = realloc(XMLCtx->attrsGlobal,
                                                        (XMLCtx->position+2)*sizeof(*XMLCtx->attrsGlobal));
				if (!XMLCtx->attrsGlobal) { 
					edg_wll_SetError(XMLCtx->ctx, ENOMEM, NULL); 
					unexpError() 
					return;
				}
				XMLCtx->attrsGlobal[XMLCtx->position] = NULL;
				XMLCtx->attrsGlobal[XMLCtx->position+1] = NULL;
			}
			else
				 unexpWarning()
                        break;  
                case 2: if (!strcasecmp(el,"QueryRec")) {
   				XMLCtx->attrsGlobal[XMLCtx->position] = 
					realloc(XMLCtx->attrsGlobal[XMLCtx->position],
						(XMLCtx->position2+2)*sizeof(**XMLCtx->attrsGlobal));
				if (!XMLCtx->attrsGlobal[XMLCtx->position]) { 
					edg_wll_SetError(XMLCtx->ctx, ENOMEM, NULL);
					unexpError()
					return;
				}
				memset(&(XMLCtx->attrsGlobal[XMLCtx->position][XMLCtx->position2]), 0, 
					2*sizeof(**XMLCtx->attrsGlobal));
			}
			else
				 unexpWarning()
                        break;  
		case 3: if (strcasecmp(el,"attribute") && strcasecmp(el,"state") &&
				strcasecmp(el,"name")) unexpWarning()
			break;	
                default: unexpWarning()
                         break;
        }       
        XMLCtx->level++;
}



static void startNotifResult(void *data, const char *el, const char **attr)
{
        edg_wll_XML_ctx *XMLCtx = data;
        int     i;

        
        strcpy(XMLCtx->element, el);
        
        switch (XMLCtx->level) {
                case 0: if (strcasecmp(el,"edg_wll_NotifResult")) { unexpError() break;}
                        for ( i = 0; attr[i] && attr[i+1]; i += 2 ) {
                                if (!strcmp(attr[i],"code"))
                                        XMLCtx->errCode = atoi(attr[i+1]);
                                else if (!strcmp(attr[i],"desc"))
                                        XMLCtx->errDesc = glite_lbu_EscapeXML(attr[i+1]);
                                else { unexpError() }
                        }       
                        break;          
                case 1: if (strcasecmp(el,"validity")) unexpWarning()
                        break;  
                default: unexpWarning()
                         break;
        }       
        XMLCtx->level++;
}               



static void startQuerySequenceCodeResult(void *data, const char *el, const char **attr)
{
        edg_wll_XML_ctx *XMLCtx = data;
        int     i;

        
        strcpy(XMLCtx->element, el);
        
        switch (XMLCtx->level) {
                case 0: if (strcasecmp(el,"edg_wll_QuerySequenceCodeResult")) { unexpError() break;}
                        for ( i = 0; attr[i] && attr[i+1]; i += 2 ) {
                                if (!strcmp(attr[i],"code"))
                                        XMLCtx->errCode = atoi(attr[i+1]);
                                else if (!strcmp(attr[i],"desc"))
                                        XMLCtx->errDesc = strdup(attr[i+1]);
                                else { unexpError() }
                        }       
                        break;          
                case 1: if (strcasecmp(el,"sequence_code")) unexpWarning()
                        break;  
                default: unexpWarning()
                         break;
        }       
        XMLCtx->level++;
}



static void startStatsResult(void *data, const char *el, const char **attr)
{
        edg_wll_XML_ctx *XMLCtx = data;
        int     i;

        
        strcpy(XMLCtx->element, el);
        
        switch (XMLCtx->level) {
                case 0: if (strcasecmp(el,"edg_wll_StatsResult")) { unexpError() break;}
                        for ( i = 0; attr[i] && attr[i+1]; i += 2 ) {
                                if (!strcmp(attr[i],"code"))
                                        XMLCtx->errCode = atoi(attr[i+1]);
                                else if (!strcmp(attr[i],"desc"))
                                        XMLCtx->errDesc = strdup(attr[i+1]);
                                else { unexpError() }
                        }      
			XMLCtx->statsGroup = (char**)malloc(1*sizeof(char*));
			XMLCtx->statsGroup[0] = NULL;
			XMLCtx->position = -1;
	
                        break;          
		case 1: if (strcasecmp(el,"from") && strcasecmp(el,"to") &&
			strcasecmp(el,"res_from") && strcasecmp(el,"res_to") &&
			strcasecmp(el, "stat"))
				unexpWarning()
			if (strcasecmp(el, "stat") == 0){
				XMLCtx->position++;
				XMLCtx->statsGroup = (char**)realloc(XMLCtx->statsGroup, (XMLCtx->position+2)*sizeof(char*));
                                XMLCtx->statsGroup[XMLCtx->position+1] = NULL;
                                XMLCtx->statsRate = (float*)realloc(XMLCtx->statsRate, (XMLCtx->position+1)*sizeof(float));
                                XMLCtx->statsDuration = (float*)realloc(XMLCtx->statsDuration, (XMLCtx->position+1)*sizeof(float));
                                XMLCtx->statsDispersion = (float*)realloc(XMLCtx->statsDispersion, (XMLCtx->position+1)*sizeof(float));
			}
			break;
		case 2: if (strcasecmp(el,"rate") && strcasecmp(el,"duration") 
			&& strcasecmp(el,"dispersion") && strcasecmp(el,"group"))
				unexpWarning()
			break;
                default: unexpWarning()
                         break;
        }       
        XMLCtx->level++;
}               



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



static void endQueryJobs(void *data, const char *el)
{
	edg_wll_XML_ctx *XMLCtx = data;


	switch (XMLCtx->level) {
		case 3 :
		        if (!strcmp(XMLCtx->element,"jobId")) {
				XMLCtx->jobsOutGlobal[XMLCtx->position] =
		                        edg_wll_from_string_to_jobid(XMLCtx);
				XMLCtx->position++;
			}
			else if (!strcmp(el,"jobStat")) {
				long len = (XML_GetCurrentByteIndex(XMLCtx->p) + XML_GetCurrentByteCount(XMLCtx->p))
						 -  XMLCtx->stat_begin;

				edg_wll_ParseJobStat(XMLCtx->ctx, XMLCtx->message_body + XMLCtx->stat_begin, len, 
							&XMLCtx->jobStatGlobal[XMLCtx->position2]);
				XMLCtx->position2++;
				XMLCtx->stat_begin = 0;
			}
			else {
				/* tag was not found -> either missing in previous code or unknown	*/
				unexpWarning()
				edg_wll_freeBuf(XMLCtx);
			}
			break;
		default:
			if (XMLCtx->char_buf) {
//				unexpWarning()
				edg_wll_freeBuf(XMLCtx);
			}
			break;
	}

	edg_wll_freeBuf(XMLCtx);
	memset(&(XMLCtx->element), 0, sizeof(XMLCtx->element));
        XMLCtx->level--;
}


static void endQueryEvents(void *data, const char *el UNUSED_VAR)
{
	edg_wll_XML_ctx *XMLCtx = data;
	

	switch (XMLCtx->level) {
	  case 3: 
@@@{
        my $i = 1;
	my $bi = "\t   ";
        for my $t (sort {$a cmp $b} getAllFields $event) {
                if ($i == 1)
                        { gen "$bi if (!strcmp(XMLCtx->element,\"$t\"))\n"; }
                else
                        { gen "$bi else if (!strcmp(XMLCtx->element,\"$t\"))\n"; }
                $i++;
                my @fo = sort $event->getFieldOccurence($t);
                if ($#fo == 0) {
                        selectType $event $fo[0];
                        my $f = selectField $event $t;
                        my $ft = $f->{type};
			$fo[0] = $fo[0] eq '_common_' ? 'any' : lcfirst $fo[0];
			if ($fo[0] =~ m/^pBS/) { $fo[0] = ucfirst $fo[0]; }
			if ($fo[0] =~ m/^cREAM/) { $fo[0] = ucfirst $fo[0]; }
			if ($fo[0] =~ m/^condor/) { $fo[0] = ucfirst $fo[0]; }
                        gen "$bi   XMLCtx->eventsOutGlobal[XMLCtx->position].$fo[0].$t =\n";
                        gen "$bi \tedg_wll_from_string_to_$ft(XMLCtx);\n";
                }
                else {
                        gen "\t      switch (XMLCtx->eventsOutGlobal[XMLCtx->position].any.type) {\n";
	                for (@fo) {
        	                selectType $event $_;
                	        my $f = selectField $event $t;
                        	my $ft = $f->{type};
	                        $t = 'any' if $_ eq '_common_';
        	                my $u = uc $_;
				$_ = lcfirst $_;
				if ($_ =~ m/^pBS/) { $_ = ucfirst $_; }
				if ($_ =~ m/^cREAM/) { $_ = ucfirst $_; }
				if ($_ =~ m/^condor/) { $_ = ucfirst $_; }
                	        gen "$bi     case EDG_WLL_EVENT_$u :\n";
                        	gen "$bi \t  XMLCtx->eventsOutGlobal[XMLCtx->position].$_.$t =\n";
	                        gen "$bi \t    edg_wll_from_string_to_$ft(XMLCtx);\n";
	                        gen "$bi \t  break;\n";

        	        }
                	gen "$bi       default : { unexpWarning() edg_wll_freeBuf(XMLCtx); } \n";
	                gen "$bi     }\n";
                }
        }
@@@}

	    else {
		/* tag was not found -> either missing in previous code or unknown	*/
		unexpWarning()
		edg_wll_freeBuf(XMLCtx);
	    }
	    break;
	  case 2: 
		  /* skip events of unknown type */
		  if (XMLCtx->eventCode == EDG_WLL_EVENT_UNDEF) {
			edg_wll_FreeEvent(&(XMLCtx->eventsOutGlobal)[XMLCtx->position]);
		  	XMLCtx->position--;
		  }
		/* fallthough */
	  default:
	    if (XMLCtx->char_buf) {
		unexpWarning()
		edg_wll_freeBuf(XMLCtx);
	    }
	    break; 
	}

	edg_wll_freeBuf(XMLCtx);
	memset(&(XMLCtx->element), 0, sizeof(XMLCtx->element));
        XMLCtx->level--;

}



// XXX: endUserJobs should be unusable soon, delete it
static void endUserJobs(void *data, const char *el UNUSED_VAR)
{
	edg_wll_XML_ctx *XMLCtx = data;
	int i;


	switch (XMLCtx->level) {
		case 2 :
		        if (!strcmp(XMLCtx->element,"jobId")) {
				XMLCtx->jobsOutGlobal[XMLCtx->position] =
		                        edg_wll_from_string_to_jobid(XMLCtx);
				XMLCtx->position++;
			}
			else {
				/* tag was not found -> either missing in previous code or unknown	*/
				unexpWarning()
				edg_wll_freeBuf(XMLCtx);
			}
			break;
		default:
			/* only level 2 tags should contain  text fields    */
			/* XXX is it necessary to unescape the data first? */
	    		for (i=0; i < XMLCtx->char_buf_len; i++) 
				if (!isspace(XMLCtx->char_buf[i])) unexpWarning()
			edg_wll_freeBuf(XMLCtx);
			break;
	}

	memset(&(XMLCtx->element), 0, sizeof(XMLCtx->element));
	// if ( !strcmp(XMLCtx->element,"jobId") ) XMLCtx->position++;
	// uncomment this if you want to remove XMLCtx->position++; 5 lines above
	// it should be useful in case of automatic generation
        XMLCtx->level--;
}



static void endJobStat(void *data, const char *el)
{
	edg_wll_XML_ctx *XMLCtx = data;

	
	switch (XMLCtx->level) {
	  case 2 :

@@@{
        my $i = 1;
	my $bi = "\t   ";
	selectType $status '_common_';
        for my $t (getFieldsOrdered $status) {
	        my $f = selectField $status $t;
	        my $ft = $f->{type};
		next if defined($f->{special}) && $f->{special} eq 'XMLstructured';
                if ($i == 1)
                        { gen "$bi if (!strcmp(XMLCtx->element,\"$t\"))\n"; }
                else
                        { gen "$bi else if (!strcmp(XMLCtx->element,\"$t\"))\n"; }
                $i++;
	        gen "$bi   XMLCtx->jobStatSingleGlobal.$t =\n";
	        gen "$bi     edg_wll_from_string_to_$ft(XMLCtx);\n";
        }
@@@}
	    else if (!strcmp(el,"children_hist")) {
		long len = (XML_GetCurrentByteIndex(XMLCtx->p) + XML_GetCurrentByteCount(XMLCtx->p))
				-  XMLCtx->stat_begin;

		edg_wll_ParseIntList(XMLCtx->ctx, XMLCtx->message_body + XMLCtx->stat_begin, len,
			"children_hist",(int (*)()) edg_wll_StringToStat, &XMLCtx->jobStatSingleGlobal.children_hist);
		XMLCtx->stat_begin = 0;
	    }
       	    else if (!strcmp(el,"possible_destinations")) {
		long len = (XML_GetCurrentByteIndex(XMLCtx->p) + XML_GetCurrentByteCount(XMLCtx->p))
				-  XMLCtx->stat_begin;

		edg_wll_ParseStrList(XMLCtx->ctx, XMLCtx->message_body + XMLCtx->stat_begin, len,
			"possible_destinations", "name", &XMLCtx->jobStatSingleGlobal.possible_destinations);
		XMLCtx->stat_begin = 0;
            }
       	    else if (!strcmp(el,"possible_ce_nodes")) {
		long len = (XML_GetCurrentByteIndex(XMLCtx->p) + XML_GetCurrentByteCount(XMLCtx->p))
				-  XMLCtx->stat_begin;

		edg_wll_ParseStrList(XMLCtx->ctx, XMLCtx->message_body + XMLCtx->stat_begin, len,
			"possible_ce_nodes", "name", &XMLCtx->jobStatSingleGlobal.possible_ce_nodes);
		XMLCtx->stat_begin = 0;
            }
       	    else if (!strcmp(el,"children")) {
		long len = (XML_GetCurrentByteIndex(XMLCtx->p) + XML_GetCurrentByteCount(XMLCtx->p))
				-  XMLCtx->stat_begin;

		edg_wll_ParseStrList(XMLCtx->ctx, XMLCtx->message_body + XMLCtx->stat_begin, len,
			"children", "jobId", &XMLCtx->jobStatSingleGlobal.children);
		XMLCtx->stat_begin = 0;
            }
	    else if (!strcmp(el,"children_states")) {
		long len = (XML_GetCurrentByteIndex(XMLCtx->p) + XML_GetCurrentByteCount(XMLCtx->p))
				-  XMLCtx->stat_begin;

		edg_wll_ParseStsList(XMLCtx->ctx, XMLCtx->message_body + XMLCtx->stat_begin, len,
			"children_states", "jobStat", &XMLCtx->jobStatSingleGlobal.children_states);
		XMLCtx->stat_begin = 0;
            }
	    else if (!strcmp(el,"user_tags")) {
		long len = (XML_GetCurrentByteIndex(XMLCtx->p) + XML_GetCurrentByteCount(XMLCtx->p))
				-  XMLCtx->stat_begin;

		edg_wll_ParseTagList(XMLCtx->ctx, XMLCtx->message_body + XMLCtx->stat_begin, len,
			"user_tags", "tag", &XMLCtx->jobStatSingleGlobal.user_tags);
		XMLCtx->stat_begin = 0;
            }
	    else if (!strcmp(el,"stateEnterTimes")) {
		long len = (XML_GetCurrentByteIndex(XMLCtx->p) + XML_GetCurrentByteCount(XMLCtx->p))
				-  XMLCtx->stat_begin;

		edg_wll_ParseIntList(XMLCtx->ctx, XMLCtx->message_body + XMLCtx->stat_begin, len,
			"stateEnterTimes",(int (*)()) edg_wll_StringToStat, &XMLCtx->jobStatSingleGlobal.stateEnterTimes);
		XMLCtx->stat_begin = 0;
            }
	    else {
	      /* tag was not found -> either missing in previous code or unknown	*/
	      unexpWarning()
	      edg_wll_freeBuf(XMLCtx);
	    }
	    break;
	  default:
	    if (XMLCtx->char_buf) {
//		unexpWarning()
		edg_wll_freeBuf(XMLCtx);
	    }
	    break;
	}

	edg_wll_freeBuf(XMLCtx);
	memset(&(XMLCtx->element), 0, sizeof(XMLCtx->element));
	XMLCtx->level--;
}



static void endStrList(void *data, const char *el UNUSED_VAR)
{
	edg_wll_XML_ctx *XMLCtx = data;

	
	switch (XMLCtx->level) {
	  case 2 :
	    if (!strcmp(XMLCtx->element,XMLCtx->XML_tag2)) {
	      XMLCtx->strListGlobal[XMLCtx->position] =
		edg_wll_from_string_to_string(XMLCtx);
	      XMLCtx->position++;
	    }
	    else {
	      /* tag was not found -> either missing in previous code or unknown	*/
	      unexpWarning()
	      edg_wll_freeBuf(XMLCtx);
	    }
	    break;
	  default:
	    if (XMLCtx->char_buf) {
//		unexpWarning()
		edg_wll_freeBuf(XMLCtx);
	    }
	    break;
	}

	edg_wll_freeBuf(XMLCtx);
	memset(&(XMLCtx->element), 0, sizeof(XMLCtx->element));
	XMLCtx->level--;
}



static void endIntList(void *data, const char *el UNUSED_VAR)
{
	edg_wll_XML_ctx *XMLCtx = data;
	int index;

	
	switch (XMLCtx->level) {
	  case 2 :
	    if ((index = XMLCtx->tagToIndex(XMLCtx->element)) >= 0 ) {
	      XMLCtx->intListGlobal[index+1] =
		edg_wll_from_string_to_int(XMLCtx);
	    }
	    else {
	      /* tag was not found -> either missing in previous code or unknown	*/
	      unexpWarning()
	      edg_wll_freeBuf(XMLCtx);
	    }
	    break;
	  default:
	    if (XMLCtx->char_buf) {
//		unexpWarning()
		edg_wll_freeBuf(XMLCtx);
	    }
	    break;
	}

	edg_wll_freeBuf(XMLCtx);
	memset(&(XMLCtx->element), 0, sizeof(XMLCtx->element));
	XMLCtx->level--;
}


static void endTagList(void *data, const char *el UNUSED_VAR)
{
	edg_wll_XML_ctx *XMLCtx = data;

	
	switch (XMLCtx->level) {
	  case 2 :
	    if (!strcmp(XMLCtx->element,XMLCtx->XML_tag2)) {
	      XMLCtx->tagListGlobal[XMLCtx->position].value =
		edg_wll_from_string_to_string(XMLCtx);
	      XMLCtx->position++;
	    }
	    else {
	      /* tag was not found -> either missing in previous code or unknown	*/
	      unexpWarning()
	      edg_wll_freeBuf(XMLCtx);
	    }
	    break;
	  default:
	    if (XMLCtx->char_buf) {
//		unexpWarning()
		edg_wll_freeBuf(XMLCtx);
	    }
	    break;
	}

	edg_wll_freeBuf(XMLCtx);
	memset(&(XMLCtx->element), 0, sizeof(XMLCtx->element));
	XMLCtx->level--;
}


static void endStsList(void *data, const char *el)
{
	edg_wll_XML_ctx *XMLCtx = data;

	
	switch (XMLCtx->level) {
	  case 2 :
	    if (!strcmp(el,XMLCtx->XML_tag2)) {
		long len = (XML_GetCurrentByteIndex(XMLCtx->p) + XML_GetCurrentByteCount(XMLCtx->p))
				 -  XMLCtx->stat_begin;

		edg_wll_ParseJobStat(XMLCtx->ctx, XMLCtx->message_body + XMLCtx->stat_begin, len, 
				&XMLCtx->stsListGlobal[XMLCtx->position]);
		XMLCtx->stat_begin = 0;
		XMLCtx->position++;
	    }
	    else {
	      /* tag was not found -> either missing in previous code or unknown	*/
	      unexpWarning()
	      edg_wll_freeBuf(XMLCtx);
	    }
	    break;
	  default:
	    if (XMLCtx->char_buf) {
//		unexpWarning()
		edg_wll_freeBuf(XMLCtx);
	    }
	    break;
	}

	edg_wll_freeBuf(XMLCtx);
	memset(&(XMLCtx->element), 0, sizeof(XMLCtx->element));
	XMLCtx->level--;
}



static void endPurgeResult(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;
        char *e;
	int ret;

        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"server_file")) 
			XMLCtx->purgeResultGlobal.server_file = edg_wll_from_string_to_string(XMLCtx);
	}
	else if (XMLCtx->level == 3) {
		if (!strcmp(XMLCtx->element,"jobId")) {
			if ( (XMLCtx->purgeResultGlobal.jobs[XMLCtx->position++] = 
				edg_wll_from_string_to_string(XMLCtx)) == NULL )
			{
				/* XXX char_buf contains an escaped value, unescaping is done within edg_wll_from_string_to_string(), which failed */
				if (XMLCtx->errtxt) {
					ret = asprintf(&e,"%s\n%s: invalid JobId at line %d",
						XMLCtx->errtxt, XMLCtx->char_buf,
						(int)XML_GetCurrentLineNumber(XMLCtx->p));
					free(XMLCtx->errtxt);
				} else ret = asprintf(&e,"%s: invalid JobId at line %d",
					XMLCtx->char_buf,(int)XML_GetCurrentLineNumber(XMLCtx->p));
				XMLCtx->errtxt = (ret == -1 ? NULL : e);
			}
		}
	}

	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}



static void endDumpResult(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;

        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"server_file")) 
			XMLCtx->dumpResultGlobal.server_file = edg_wll_from_string_to_string(XMLCtx);
		else if (!strcmp(XMLCtx->element,"from")) {
			if (isdigit(XMLCtx->char_buf[0]))
			XMLCtx->dumpResultGlobal.from = edg_wll_from_string_to_time_t(XMLCtx);
			else
				XMLCtx->dumpResultGlobal.from = edg_wll_StringToDumpConst(XMLCtx->char_buf);
		}
		else if (!strcmp(XMLCtx->element,"to")) {
			if (isdigit(XMLCtx->char_buf[0]))
			XMLCtx->dumpResultGlobal.to = edg_wll_from_string_to_time_t(XMLCtx);
			else
				XMLCtx->dumpResultGlobal.to = edg_wll_StringToDumpConst(XMLCtx->char_buf);
		}
	}

	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}



static void endLoadResult(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;

        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"server_file"))
                        XMLCtx->loadResultGlobal.server_file = edg_wll_from_string_to_string(XMLCtx);
                else if (!strcmp(XMLCtx->element,"from"))
                        XMLCtx->loadResultGlobal.from = edg_wll_from_string_to_time_t(XMLCtx);
                else if (!strcmp(XMLCtx->element,"to"))
                        XMLCtx->loadResultGlobal.to = edg_wll_from_string_to_time_t(XMLCtx);
		
	}

	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}



static void endIndexedAttrs(void *data, const char *el)
{
        edg_wll_XML_ctx *XMLCtx = data;

        if (XMLCtx->level == 2) {
		if (!strcmp(el,"index"))
			XMLCtx->position++;
			XMLCtx->position2 = 0;
	}
        if (XMLCtx->level == 3) {
		if (!strcmp(el,"QueryRec"))
			XMLCtx->position2++;
	}
	if (XMLCtx->level == 4) {
                if (!strcmp(XMLCtx->element,"attribute")) {
                        XMLCtx->attrsGlobal[XMLCtx->position][XMLCtx->position2].attr = 
				edg_wll_StringToquery_attr(edg_wll_from_string_to_string(XMLCtx));
		}
		else if (!strcmp(XMLCtx->element,"state")) {
			XMLCtx->attrsGlobal[XMLCtx->position][XMLCtx->position2].attr_id.state =
				edg_wll_StringToStat(edg_wll_from_string_to_string(XMLCtx));
		}
		else if (!strcmp(XMLCtx->element,"name")) {
			XMLCtx->attrsGlobal[XMLCtx->position][XMLCtx->position2].attr_id.tag =
				edg_wll_from_string_to_string(XMLCtx);
		}	
        }

	edg_wll_freeBuf(XMLCtx);
        XMLCtx->level--;
}


static void endNotifResult(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;


        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"validity"))
                        XMLCtx->notifValidity = edg_wll_from_string_to_time_t(XMLCtx);
	}

	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}



static void endQuerySequenceCodeResult(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;

        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"sequence_code"))
                        XMLCtx->seqCode = edg_wll_from_string_to_string(XMLCtx);
	}

	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}






static void endStatsResult(void *data, const char *el UNUSED_VAR)
{
        edg_wll_XML_ctx *XMLCtx = data;


        if (XMLCtx->level == 2) {
                if (!strcmp(XMLCtx->element,"from"))
                        XMLCtx->statsFrom = edg_wll_from_string_to_time_t(XMLCtx);
                else if (!strcmp(XMLCtx->element,"to"))
                        XMLCtx->statsTo = edg_wll_from_string_to_time_t(XMLCtx);
                else if (!strcmp(XMLCtx->element,"res_from"))
                        XMLCtx->statsResFrom = edg_wll_from_string_to_int(XMLCtx);
                else if (!strcmp(XMLCtx->element,"res_to"))
                        XMLCtx->statsResTo = edg_wll_from_string_to_int(XMLCtx);
	}
	else if (XMLCtx->level == 3){
		if (!strcmp(XMLCtx->element,"rate"))
                        XMLCtx->statsRate[XMLCtx->position] = 
				edg_wll_from_string_to_float(XMLCtx);
                else if (!strcmp(XMLCtx->element,"duration"))
                        XMLCtx->statsDuration[XMLCtx->position] = 
				edg_wll_from_string_to_float(XMLCtx);
                else if (!strcmp(XMLCtx->element,"dispersion"))
                        XMLCtx->statsDispersion[XMLCtx->position] = 
				edg_wll_from_string_to_float(XMLCtx);
                else if (!strcmp(XMLCtx->element,"group"))
                        XMLCtx->statsGroup[XMLCtx->position] = 
				edg_wll_from_string_to_string(XMLCtx);
	}

	edg_wll_freeBuf(XMLCtx);
	XMLCtx->level--;
}



#undef unexpError
#undef unexpWarning



edg_wll_ErrorCode edg_wll_ParseQueryJobs(edg_wll_Context ctx, char *messageBody, edg_wlc_JobId **jobsOut, edg_wll_JobStat **statesOut)
{
	int i;
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode = 0;
	XML_Char *encoding = "ISO-8859-1";


	edg_wll_initXMLCtx(&XMLCtx);
	edg_wll_ResetError(ctx);	
	XMLCtx.message_body = messageBody;
	XMLCtx.ctx = ctx;

	/* initialize parser */
	XMLCtx.p = XML_ParserCreate(encoding);
 	XML_SetElementHandler(XMLCtx.p, startQueryJobs, endQueryJobs);
 	XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
	XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);

	/* parse messageBody */
	if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
		char *errorMessage;

		if (asprintf(&errorMessage, "XML parse error at line %d:\n%s\n",
			(int)XML_GetCurrentLineNumber(XMLCtx.p),
			XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

		edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);		
	}  else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);

	if (!(errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		if (XMLCtx.errDesc || XMLCtx.errCode) {
			ctx->errDesc = strdup(XMLCtx.errDesc);
			ctx->errCode = XMLCtx.errCode;
			errorCode = XMLCtx.errCode;
		}
	}
        if (errorCode) {
		for (i=0; i<XMLCtx.position; i++)  edg_wlc_JobIdFree( (XMLCtx.jobsOutGlobal)[i] );
		free(XMLCtx.jobsOutGlobal);
		XMLCtx.jobsOutGlobal = NULL;
		XMLCtx.position = 0;
		if (jobsOut) *jobsOut = NULL;	

		for (i=0; i<XMLCtx.position2; i++) edg_wll_FreeStatus( &(XMLCtx.jobStatGlobal[i]) );
		free(XMLCtx.jobStatGlobal);
		XMLCtx.jobStatGlobal = NULL;
		XMLCtx.position2 = 0;
		if (statesOut) *statesOut = NULL;

                if (XMLCtx.errDesc) {
                        free(XMLCtx.errDesc);
                        XMLCtx.errDesc = NULL;
                        XMLCtx.errCode = 0;
                }

		goto end;
        }

	/* malloc-ate one more row for NULL list termination */
        XMLCtx.jobsOutGlobal = realloc(XMLCtx.jobsOutGlobal,
                               (XMLCtx.position+1)*sizeof(*XMLCtx.jobsOutGlobal));

        if (!XMLCtx.jobsOutGlobal) { 
		errorCode = (edg_wll_ErrorCode) ENOMEM;
		if (jobsOut) *jobsOut = NULL;	
	}
 	else {
		/* add NULL to end of list */
		XMLCtx.jobsOutGlobal[XMLCtx.position] = NULL;		

        	/* return results */
		// XXX : may be should not be transfered; need to change protocol and 
		// XXX   then return only non-NULL jobsOut or statesOut
		if (jobsOut) 
	        	*jobsOut = XMLCtx.jobsOutGlobal;
		else {
			for (i=0; i<XMLCtx.position; i++)  edg_wlc_JobIdFree( (XMLCtx.jobsOutGlobal)[i] );
			free(XMLCtx.jobsOutGlobal); 
		}
		XMLCtx.jobsOutGlobal = NULL; 
	}
       
	XMLCtx.jobStatGlobal = realloc(XMLCtx.jobStatGlobal,
				(XMLCtx.position2+1)*sizeof(*XMLCtx.jobStatGlobal));

	if (!XMLCtx.jobStatGlobal) { 
		errorCode = (edg_wll_ErrorCode) ENOMEM;
		if (statesOut) *statesOut = NULL;
	}
 	else {
		/* add NULL to end of list */
		edg_wll_InitStatus(&XMLCtx.jobStatGlobal[XMLCtx.position2]);		

        	/* return results */
		if (statesOut)
	        	*statesOut = XMLCtx.jobStatGlobal;
		else {
			for (i=0; i<XMLCtx.position2; i++) edg_wll_FreeStatus( &(XMLCtx.jobStatGlobal[i]) );
			free(XMLCtx.jobStatGlobal);
		}
	
		XMLCtx.jobStatGlobal = NULL; 
	}
	

end:
	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	

		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

	/* free parser */
	XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);
	return errorCode;
}



edg_wll_ErrorCode edg_wll_ParseQueryEvents(edg_wll_Context ctx, char *messageBody, edg_wll_Event **eventsOut)
{
	int i;
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode = 0;
	XML_Char *encoding = "ISO-8859-1";


	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.position = -1;
	edg_wll_ResetError(ctx);	

        /* initialize parser */
	XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startQueryEvents, endQueryEvents);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
	XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);

        /* parse messageBody */
        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;


                if (asprintf(&errorMessage, "XML parse error at line %d:\n%s\n",
                        (int)XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

		edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
		free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);

        if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		*eventsOut = NULL;
		for (i=0; i<= XMLCtx.position; i++) edg_wll_FreeEvent( &((XMLCtx.eventsOutGlobal)[i]) );
               	free(XMLCtx.eventsOutGlobal); 
		XMLCtx.eventsOutGlobal = NULL;

                if (XMLCtx.errDesc) {
                        free(XMLCtx.errDesc);
                        XMLCtx.errDesc = NULL;
                        XMLCtx.errCode = 0;
                }
	}
        else {
		/* malloc-ate one more row for NULL list termination */
                XMLCtx.eventsOutGlobal = realloc(XMLCtx.eventsOutGlobal,
                                          (++XMLCtx.position+1)*sizeof(*XMLCtx.eventsOutGlobal));
                if (!XMLCtx.eventsOutGlobal) { 
			errorCode = (edg_wll_ErrorCode) ENOMEM; 
                	*eventsOut = NULL;
		}
 		else {
        		/* last event in list is EDG_WLL_EVENT_UNDEF for end-of-list detection */
			memset(&XMLCtx.eventsOutGlobal[XMLCtx.position],0,sizeof(*XMLCtx.eventsOutGlobal));
        		XMLCtx.eventsOutGlobal[XMLCtx.position].any.type = EDG_WLL_EVENT_UNDEF;

	        	/* return results */
	        	*eventsOut = XMLCtx.eventsOutGlobal;
			XMLCtx.eventsOutGlobal = NULL; 
		}
        }
	
	if (XMLCtx.errDesc || XMLCtx.errCode) {
                ctx->errDesc = XMLCtx.errDesc;
		ctx->errCode = XMLCtx.errCode;
		errorCode =  XMLCtx.errCode;
        }

	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"------------------------edg_wll_ParseQueryEvents----------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

        /* free parser */
        XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);
        return errorCode;
}




edg_wll_ErrorCode edg_wll_ParseUserJobs(edg_wll_Context ctx, char *messageBody, edg_wlc_JobId **jobsOut)
{
	int i;
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode = 0;
	XML_Char *encoding = "ISO-8859-1";


	edg_wll_initXMLCtx(&XMLCtx);
	edg_wll_ResetError(ctx);	

	/* initialize parser */
	XMLCtx.p = XML_ParserCreate(encoding);
 	XML_SetElementHandler(XMLCtx.p, startUserJobs, endUserJobs);
 	XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
	XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);

	/* parse messageBody */
	if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
		char *errorMessage;


		if (asprintf(&errorMessage, "XML parse error at line %d:\n%s\n",
			(int)XML_GetCurrentLineNumber(XMLCtx.p),
			XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;
		*jobsOut = NULL;	

		edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);		
	}  else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);

        if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
                *jobsOut = NULL;
		for (i=0; i<XMLCtx.position; i++)  edg_wlc_JobIdFree( (XMLCtx.jobsOutGlobal)[i] );
		free(XMLCtx.jobsOutGlobal);
		XMLCtx.jobsOutGlobal = NULL;
        } else {
		/* malloc-ate one more row for NULL list termination */
                XMLCtx.jobsOutGlobal = realloc(XMLCtx.jobsOutGlobal,
                                        (XMLCtx.position+1)*sizeof(*XMLCtx.jobsOutGlobal));
                if (!XMLCtx.jobsOutGlobal) { 
			errorCode = (edg_wll_ErrorCode) ENOMEM;
                	*jobsOut = NULL;
		}
 		else {
			/* add NULL to end of list */
			XMLCtx.jobsOutGlobal[XMLCtx.position] = NULL;		

	        	/* return results */
	        	*jobsOut = XMLCtx.jobsOutGlobal;
			XMLCtx.jobsOutGlobal = NULL; 
		}
        }

	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

	/* free parser */
	XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);
	return errorCode;
}



edg_wll_ErrorCode edg_wll_ParseJobStat(edg_wll_Context ctx, char *messageBody, long len, edg_wll_JobStat *stat)
{
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode;
	XML_Char *encoding = "ISO-8859-1";


	edg_wll_initXMLCtx(&XMLCtx);
	edg_wll_ResetError(ctx);	
	XMLCtx.message_body = messageBody;
	XMLCtx.ctx = ctx;

        /* initialize parser */
	XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startJobStatus, endJobStat);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
	XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);

        /* parse messageBody */
        if (! XML_Parse(XMLCtx.p, messageBody, len, 1)) {
                char *errorMessage;


                if (asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        (int)XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);

        if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		edg_wll_FreeStatus( &XMLCtx.jobStatSingleGlobal );
		memset(stat,0,sizeof(*stat));
		XMLCtx.position = 0;
        }
	else {
		/* return results */
		memcpy(stat, &XMLCtx.jobStatSingleGlobal, sizeof(XMLCtx.jobStatSingleGlobal));
	}

	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

        /* free parser */
        XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);	
	return errorCode;
}


edg_wll_ErrorCode edg_wll_ParseStrList(edg_wll_Context ctx, char *messageBody, long len, char *tag, char *tag2,  char ***strListOut)
{
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode;
	XML_Char *encoding = "ISO-8859-1";


	edg_wll_initXMLCtx(&XMLCtx);
	edg_wll_ResetError(ctx);	
	XMLCtx.message_body = messageBody;
	XMLCtx.ctx = ctx;
	XMLCtx.XML_tag = strdup(tag);
	XMLCtx.XML_tag2 = strdup(tag2);

        /* initialize parser */
	XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startStrList, endStrList);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
	XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);

        /* parse messageBody */
        if (! XML_Parse(XMLCtx.p, messageBody, len, 1)) {
                char *errorMessage;


                if (asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        (int)XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);

        if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		if (XMLCtx.strListGlobal) {
			int i;

			for (i=0; XMLCtx.strListGlobal[i]; i++)
				free(XMLCtx.strListGlobal[i]);
			free(XMLCtx.strListGlobal);
			XMLCtx.strListGlobal = NULL;
		}
		*strListOut = NULL;
		XMLCtx.position = 0;
        }
	else {
	        XMLCtx.strListGlobal = realloc(XMLCtx.strListGlobal,
                               (XMLCtx.position+1)*sizeof(*XMLCtx.strListGlobal));

        	if (!XMLCtx.strListGlobal) {
                	errorCode = (edg_wll_ErrorCode) ENOMEM;
	                if (strListOut) *strListOut = NULL;
        	}
       		else {
                	/* add NULL to end of list */
                	XMLCtx.strListGlobal[XMLCtx.position] = NULL;

			/* return results */
			*strListOut = XMLCtx.strListGlobal;
		}
	}

	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

        /* free parser */
        XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);	
	return errorCode;
}


edg_wll_ErrorCode edg_wll_ParseIntList(edg_wll_Context ctx, char *messageBody, long len, char *tag, int (*tagToIndex)(),  int **intListOut)
{
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode;
	XML_Char *encoding = "ISO-8859-1";


	edg_wll_initXMLCtx(&XMLCtx);
	edg_wll_ResetError(ctx);	
	XMLCtx.message_body = messageBody;
	XMLCtx.ctx = ctx;
	XMLCtx.XML_tag = strdup(tag);
	XMLCtx.tagToIndex = tagToIndex;

        /* initialize parser */
	XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startIntList, endIntList);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
	XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);

        /* parse messageBody */
        if (! XML_Parse(XMLCtx.p, messageBody, len, 1)) {
                char *errorMessage;


                if (asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        (int)XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);

        if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		if (XMLCtx.intListGlobal) free(XMLCtx.intListGlobal);
		*intListOut = NULL;
		XMLCtx.max_index = 0;
        }
	else {
               	/* add array length to zeros' position */
               	if (XMLCtx.intListGlobal) 
			XMLCtx.intListGlobal[0] = XMLCtx.max_index + 1;

		/* return results */
		*intListOut = XMLCtx.intListGlobal;
	}

	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

        /* free parser */
        XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);	
	return errorCode;
}


edg_wll_ErrorCode edg_wll_ParseTagList(edg_wll_Context ctx, char *messageBody, long len, char *tag, char *tag2,  edg_wll_TagValue **tagListOut)
{
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode;
	XML_Char *encoding = "ISO-8859-1";


	edg_wll_initXMLCtx(&XMLCtx);
	edg_wll_ResetError(ctx);	
	XMLCtx.message_body = messageBody;
	XMLCtx.ctx = ctx;
	XMLCtx.XML_tag = strdup(tag);
	XMLCtx.XML_tag2 = strdup(tag2);

        /* initialize parser */
	XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startTagList, endTagList);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
	XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);

        /* parse messageBody */
        if (! XML_Parse(XMLCtx.p, messageBody, len, 1)) {
                char *errorMessage;


                if (asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        (int)XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);

        if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		if (XMLCtx.tagListGlobal) {
			int i;

			for (i=0; XMLCtx.tagListGlobal[i].tag; i++) {
				free(XMLCtx.tagListGlobal[i].tag);
				free(XMLCtx.tagListGlobal[i].value);
			}
			free(XMLCtx.tagListGlobal);
			XMLCtx.tagListGlobal = NULL;
		}
		*tagListOut = NULL;
		XMLCtx.position = 0;
        }
	else {
	        XMLCtx.tagListGlobal = realloc(XMLCtx.tagListGlobal,
                               (XMLCtx.position+1)*sizeof(*XMLCtx.tagListGlobal));

        	if (!XMLCtx.tagListGlobal) {
                	errorCode = (edg_wll_ErrorCode) ENOMEM;
	                if (tagListOut) *tagListOut = NULL;
        	}
       		else {
                	/* add NULL to end of list */
                	XMLCtx.tagListGlobal[XMLCtx.position].tag = NULL;

			/* return results */
			*tagListOut = XMLCtx.tagListGlobal;
		}
	}

	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

        /* free parser */
        XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);	
	return errorCode;
}



edg_wll_ErrorCode edg_wll_ParseStsList(edg_wll_Context ctx, char *messageBody, long len, char *tag, char *tag2,  edg_wll_JobStat **stsListOut)
{
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode;
	XML_Char *encoding = "ISO-8859-1";


	edg_wll_initXMLCtx(&XMLCtx);
	edg_wll_ResetError(ctx);	
	XMLCtx.message_body = messageBody;
	XMLCtx.ctx = ctx;
	XMLCtx.XML_tag = strdup(tag);
	XMLCtx.XML_tag2 = strdup(tag2);

        /* initialize parser */
	XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startStsList, endStsList);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
	XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);

        /* parse messageBody */
        if (! XML_Parse(XMLCtx.p, messageBody, len, 1)) {
                char *errorMessage;


                if (asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        (int)XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);

        if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		if (XMLCtx.stsListGlobal) {
			int i;

			for (i=0; XMLCtx.position; i++)
				edg_wll_FreeStatus(&XMLCtx.stsListGlobal[i]);
			free(XMLCtx.stsListGlobal);
			XMLCtx.stsListGlobal = NULL;
		}
		*stsListOut = NULL;
		XMLCtx.position = 0;
        }
	else {
	        XMLCtx.stsListGlobal = realloc(XMLCtx.stsListGlobal,
                               (XMLCtx.position+1)*sizeof(*XMLCtx.stsListGlobal));

        	if (!XMLCtx.stsListGlobal) {
                	errorCode = (edg_wll_ErrorCode) ENOMEM;
	                if (stsListOut) *stsListOut = NULL;
        	}
       		else {
                	/* add NULL to end of list */
                	edg_wll_InitStatus(&XMLCtx.stsListGlobal[XMLCtx.position]);

			/* return results */
			*stsListOut = XMLCtx.stsListGlobal;
		}
	}

	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

        /* free parser */
        XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);	
	return errorCode;
}



/* parse purge result from client */
edg_wll_ErrorCode edg_wll_ParsePurgeResult(edg_wll_Context ctx, char *messageBody, edg_wll_PurgeResult *result)
{
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode;
	XML_Char *encoding = "ISO-8859-1";

	if (!messageBody)
		return edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, "Parse error: empty response");

	errno = 0;
	edg_wll_ResetError(ctx);
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.ctx = ctx;


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startPurgeResult, endPurgeResult);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                if (asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        (int)XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		int i;

		if (XMLCtx.purgeResultGlobal.jobs) {
			for (i=0; XMLCtx.purgeResultGlobal.jobs[i]; i++) 
				free(XMLCtx.purgeResultGlobal.jobs[i]);
			free(XMLCtx.purgeResultGlobal.jobs);
		}
		memset(result,0,sizeof(*result));
		free(XMLCtx.purgeResultGlobal.server_file);
		
	} else {
		memcpy(result, &XMLCtx.purgeResultGlobal, sizeof(XMLCtx.purgeResultGlobal));
	}

	if (XMLCtx.errDesc || XMLCtx.errCode) {
		ctx->errDesc = XMLCtx.errDesc;
		ctx->errCode = XMLCtx.errCode;
		errorCode =  XMLCtx.errCode;
	}
	
	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

        /* free parser */
        XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);	
	return errorCode;
}



/* parse dump result from client */
edg_wll_ErrorCode edg_wll_ParseDumpResult(edg_wll_Context ctx, char *messageBody, edg_wll_DumpResult *result)
{
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode;
	XML_Char *encoding = "ISO-8859-1";

	if (!messageBody)
		return edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, "Parse error: empty response");

	errno = 0;
	edg_wll_ResetError(ctx);
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.ctx = ctx;


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startDumpResult, endDumpResult);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                if (asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        (int)XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		free(XMLCtx.dumpResultGlobal.server_file);
		memset(result,0,sizeof(*result));
	} else {
		memcpy(result, &XMLCtx.dumpResultGlobal, sizeof(XMLCtx.dumpResultGlobal));
	}

	if (XMLCtx.errDesc || XMLCtx.errCode) {
		ctx->errDesc = XMLCtx.errDesc;
		ctx->errCode = XMLCtx.errCode;
		errorCode =  XMLCtx.errCode;
	}
	
	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

        /* free parser */
        XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);	
	return errorCode;
}



/* parse load result from client */
edg_wll_ErrorCode edg_wll_ParseLoadResult(edg_wll_Context ctx, char *messageBody, edg_wll_LoadResult *result)
{
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode;
	XML_Char *encoding = "ISO-8859-1";

	if (!messageBody)
		return edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, "Parse error: empty response");

	errno = 0;
	edg_wll_ResetError(ctx);
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.ctx = ctx;


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startLoadResult, endLoadResult);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                if (asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        (int)XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		free(XMLCtx.loadResultGlobal.server_file);
		memset(result,0,sizeof(*result));
	} else {
		memcpy(result, &XMLCtx.loadResultGlobal, sizeof(XMLCtx.loadResultGlobal));
	}

	if (XMLCtx.errDesc || XMLCtx.errCode) {
		ctx->errDesc = XMLCtx.errDesc;
		ctx->errCode = XMLCtx.errCode;
		errorCode =  XMLCtx.errCode;
	}
	
	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

        /* free parser */
        XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);	
	return errorCode;
}


edg_wll_ErrorCode edg_wll_ParseIndexedAttrs(edg_wll_Context ctx, char *messageBody, edg_wll_QueryRec ***attrs)
{
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode;
	XML_Char *encoding = "ISO-8859-1";


	edg_wll_initXMLCtx(&XMLCtx);
	edg_wll_ResetError(ctx);	
	XMLCtx.message_body = messageBody;
	XMLCtx.ctx = ctx;

        /* initialize parser */
	XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startIndexedAttrs, endIndexedAttrs);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
	XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);

        /* parse messageBody */
        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;


                if (asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        (int)XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);

        if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		if (XMLCtx.attrsGlobal) {
			int i,j;

			for (i=0; XMLCtx.attrsGlobal[i]; i++) {
				for (j=0; XMLCtx.attrsGlobal[i][j].attr != EDG_WLL_QUERY_ATTR_UNDEF; j++) {
					if (XMLCtx.attrsGlobal[i][j].attr == EDG_WLL_QUERY_ATTR_USERTAG) 
						free(XMLCtx.attrsGlobal[i][j].attr_id.tag);
				}		
				free(XMLCtx.attrsGlobal[i]);
			}
			free(XMLCtx.attrsGlobal);
			XMLCtx.attrsGlobal = NULL;
		}
		XMLCtx.position = 0;
		XMLCtx.position2 = 0;
        }

	/* return results */
	*attrs = XMLCtx.attrsGlobal;

	if (XMLCtx.errDesc || XMLCtx.errCode) {
		ctx->errDesc = XMLCtx.errDesc;
		ctx->errCode = XMLCtx.errCode;
		errorCode =  XMLCtx.errCode;
	}

	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

        /* free parser */
        XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);	
	return errorCode;
}



/* parse notification result from client */
edg_wll_ErrorCode edg_wll_ParseNotifResult(edg_wll_Context ctx, char *messageBody, time_t *validity)
{
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode;
	XML_Char *encoding = "ISO-8859-1";

	if (!messageBody)
		return edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, "Parse error: empty response");

	errno = 0;
	edg_wll_ResetError(ctx);
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.ctx = ctx;


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startNotifResult, endNotifResult);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                if (asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        (int)XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		*validity = -1;
	} else {
		*validity = XMLCtx.notifValidity;
	}

	if (XMLCtx.errDesc || XMLCtx.errCode) {
		ctx->errDesc = XMLCtx.errDesc;
		ctx->errCode = XMLCtx.errCode;
		errorCode =  XMLCtx.errCode;
	}
	
	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

        /* free parser */
        XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);	
	return errorCode;
}



/* parse sequence code result from server */
edg_wll_ErrorCode edg_wll_ParseQuerySequenceCodeResult(edg_wll_Context ctx, char *messageBody, char **seqCode)
{
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode;
	XML_Char *encoding = "ISO-8859-1";

	if (!messageBody)
		return edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, "Parse error: empty response");

	errno = 0;
	edg_wll_ResetError(ctx);
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.ctx = ctx;


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startQuerySequenceCodeResult, endQuerySequenceCodeResult);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                if (asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        (int)XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		free(XMLCtx.seqCode);
		*seqCode = NULL;
	} else {
		*seqCode = XMLCtx.seqCode;
	}

	if (XMLCtx.errDesc || XMLCtx.errCode) {
		ctx->errDesc = XMLCtx.errDesc;
		ctx->errCode = XMLCtx.errCode;
		errorCode =  XMLCtx.errCode;
	}
	
	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

        /* free parser */
        XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);	
	return errorCode;
}



/* parse statistics result from client */

/* this is backward-compatible version, edg_wll_ParseStatsResultFull is prefered*/
edg_wll_ErrorCode edg_wll_ParseStatsResult(edg_wll_Context ctx, char *messageBody, time_t *from, time_t *to, float *rate, float *duration, int *res_from, int *res_to)
{
	float *rates, *durations, *dispersions;
	char **groups;
	edg_wll_ErrorCode ret;
	int i;

	ret = edg_wll_ParseStatsResultFull(ctx, messageBody, from, to, &rates, &durations, &dispersions, &groups, res_from, res_to);

	if (groups && groups[0]){
		if (rates) {
			*rate = rates[0];
			free(rates);
		}
		if (durations) {
			*duration = durations[0];
			free(durations);
		}
		if (dispersions)
			free(dispersions);
		for(i = 0; groups[i]; i++)  
			free(groups[i]);
	}
	if (groups)
		free(groups);

	return ret;
}

edg_wll_ErrorCode edg_wll_ParseStatsResultFull(edg_wll_Context ctx, char *messageBody, time_t *from, time_t *to, float **rate, float **duration, float **dispersion, char ***group, int *res_from, int *res_to)
{
	edg_wll_XML_ctx XMLCtx;
	edg_wll_ErrorCode errorCode;
	XML_Char *encoding = "ISO-8859-1";
	int i;

	if (!messageBody)
		return edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, "Parse error: empty response");

	errno = 0;
	edg_wll_ResetError(ctx);
	edg_wll_initXMLCtx(&XMLCtx);
	XMLCtx.ctx = ctx;


        /* initialize parser */
        XMLCtx.p = XML_ParserCreate(encoding);
        XML_SetElementHandler(XMLCtx.p, startStatsResult, endStatsResult);
        XML_SetCharacterDataHandler(XMLCtx.p, char_handler);
        XML_SetUserData(XMLCtx.p, (void *) &XMLCtx);


        if (! XML_Parse(XMLCtx.p, messageBody, strlen(messageBody), 1)) {
                char *errorMessage;

                if (asprintf(&errorMessage, "Parse error at line %d:\n%s\n",
                        (int)XML_GetCurrentLineNumber(XMLCtx.p),
                        XML_ErrorString(XML_GetErrorCode(XMLCtx.p))) == -1) errorMessage = NULL;

                edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, errorMessage);
                free(errorMessage);
        } else if (XMLCtx.errtxt) edg_wll_SetError(ctx, EDG_WLL_ERROR_XML_PARSE, XMLCtx.errtxt);


	if ((errorCode = edg_wll_Error(ctx,NULL,NULL))) {
		*from = -1;
		*to = -1;
		*rate = NULL;
		*duration = NULL;
		*dispersion = NULL;
		*group = NULL;
		*res_from = -1;
		*res_to = -1;
	} else {
		*from = XMLCtx.statsFrom;
		*to = XMLCtx.statsTo;
		*rate = NULL;
                *duration = NULL;
                *dispersion = NULL;
		*group = NULL;
		for (i = 0; XMLCtx.statsGroup[i]; i++){
			*rate = (float*)realloc(*rate, (i+1)*sizeof(float));
                        (*rate)[i] = XMLCtx.statsRate[i];
                        *duration = (float*)realloc(*duration, (i+1)*sizeof(float));
                        (*duration)[i] = XMLCtx.statsDuration[i];
                        *dispersion = (float*)realloc(*dispersion, (i+1)*sizeof(float));
                        (*dispersion)[i] = XMLCtx.statsDispersion[i];
                        *group = (char**)realloc(*group, (i+2)*sizeof(char*));
                        (*group)[i] = strdup(XMLCtx.statsGroup[i]);
                        (*group)[i+1] = NULL;
		}
		*res_from = XMLCtx.statsResFrom;
		*res_to = XMLCtx.statsResTo;
	}

	if (XMLCtx.errDesc || XMLCtx.errCode) {
		ctx->errDesc = XMLCtx.errDesc;
		ctx->errCode = XMLCtx.errCode;
		errorCode =  XMLCtx.errCode;
	}
	
	/* print all warning if corresponding env variable is set	*/
	if (XMLCtx.warntxt && getenv("EDG_WLL_XML_WARNINGS")) {	
		fprintf(stderr,"----------------------------------------------------\n");
		fprintf(stderr,"%s\n\n",XMLCtx.warntxt);
		fprintf(stderr,"%s\n",messageBody);
		fprintf(stderr,"----------------------------------------------------\n");
	}

        /* free parser */
        XML_ParserFree(XMLCtx.p);

	edg_wll_freeXMLCtx(&XMLCtx);	
	return errorCode;
}




/* construct Message-Body of Request-Line for edg_wll_QueryJobs */
int edg_wll_JobQueryRecToXML(
		edg_wll_Context ctx,
		edg_wll_QueryRec const * const *conditions,
                char **message)
{
        char *pomA, *pomB, *pomC;
        int i = 0, len, tot_len = 0, nconditions, row = 0;
        int *len_list;
        char **list;

	pomC = strdup("");

	while (conditions && conditions[row]) {

	        pomA = pomB = NULL;
	
	        for (i=0; conditions[row][i].attr != EDG_WLL_QUERY_ATTR_UNDEF ; i++);
	        nconditions = i;
	
	        list = (char **) malloc ((nconditions) * sizeof(*list));
	        len_list = (int *) malloc ((nconditions) * sizeof(*len_list));
	
	        i=0;
	        while (i < nconditions) {
	                const char *pomOp;
	                char       *pomValue;

			pomValue = strdup("");
	
	                switch (conditions[row][i].attr) {
	                                /* job specific conditions */
	                                case EDG_WLL_QUERY_ATTR_JOBID:
	                                        edg_wll_add_jobid_to_XMLBody(&pomValue, conditions[row][i].value.j, "jobId", NULL);
	                                        break;
					case EDG_WLL_QUERY_ATTR_TIME:
						if (conditions[row][i].op == EDG_WLL_QUERY_OP_WITHIN) {
							edg_wll_add_tagged_time_t_to_XMLBody(&pomValue, conditions[row][i].value.t.tv_sec, "time", edg_wll_StatToString(conditions[row][i].attr_id.state), "state", -1);
							edg_wll_add_tagged_time_t_to_XMLBody(&pomValue, conditions[row][i].value2.t.tv_sec, "time", edg_wll_StatToString(conditions[row][i].attr_id.state), "state", -1);	
						}
						else 
							edg_wll_add_tagged_time_t_to_XMLBody(&pomValue, conditions[row][i].value.t.tv_sec, "time", edg_wll_StatToString(conditions[row][i].attr_id.state), "state", -1);
						break;
				
	                                case EDG_WLL_QUERY_ATTR_PARENT:
	                                        edg_wll_add_jobid_to_XMLBody(&pomValue, conditions[row][i].value.j, "parent_job", NULL);
	                                        break;
	                                case EDG_WLL_QUERY_ATTR_OWNER:
	                                        edg_wll_add_string_to_XMLBody(&pomValue, (conditions[row][i].value.c) ? conditions[row][i].value.c : 
														"NULL", "owner", NULL);
	                                        break;
	                                case EDG_WLL_QUERY_ATTR_STATUS:
						if (conditions[row][i].op != EDG_WLL_QUERY_OP_CHANGED)
							edg_wll_add_edg_wll_JobStatCode_to_XMLBody(&pomValue, conditions[row][i].value.i, "status", EDG_WLL_JOB_UNDEF);
						else {
							char *newBody;
	
							asprintf(&newBody,"%s\t\t\t<status/>\r\n", pomValue);
							free(pomValue);
							pomValue = newBody;
						}
						if (conditions[row][i].op == EDG_WLL_QUERY_OP_WITHIN)
							edg_wll_add_edg_wll_JobStatCode_to_XMLBody(&pomValue, conditions[row][i].value2.i, "status", EDG_WLL_JOB_UNDEF);
	                                        break;
	                                case EDG_WLL_QUERY_ATTR_LOCATION:
	                                        edg_wll_add_string_to_XMLBody(&pomValue, conditions[row][i].value.c, "location", NULL);
	                                        break;
	                                case EDG_WLL_QUERY_ATTR_DESTINATION:
	                                        conditions[row][i].value.c ?
							edg_wll_add_string_to_XMLBody(&pomValue, conditions[row][i].value.c, "destination", NULL) :
							edg_wll_add_string_to_XMLBody(&pomValue, "", "destination", NULL);
	                                        break;
	                                case EDG_WLL_QUERY_ATTR_NETWORK_SERVER:
	                                        edg_wll_add_string_to_XMLBody(&pomValue, conditions[row][i].value.c, "networkserver", NULL);
	                                        break;
	                                case EDG_WLL_QUERY_ATTR_RESUBMITTED:
	                                        edg_wll_add_int_to_XMLBody(&pomValue, conditions[row][i].value.i, "resubmitted", -1);
						if (conditions[row][i].op == EDG_WLL_QUERY_OP_WITHIN)
							edg_wll_add_int_to_XMLBody(&pomValue, conditions[row][i].value2.i, "resubmitted", -1);
	                                        break;
	                                case EDG_WLL_QUERY_ATTR_DONECODE:
						edg_wll_add_int_to_XMLBody(&pomValue, conditions[row][i].value.i, "donecode", -1);
						if (conditions[row][i].op == EDG_WLL_QUERY_OP_WITHIN)
							edg_wll_add_int_to_XMLBody(&pomValue, conditions[row][i].value2.i, "donecode", -1);
	                                        break;
	                                case EDG_WLL_QUERY_ATTR_EXITCODE:
						edg_wll_add_int_to_XMLBody(&pomValue, conditions[row][i].value.i, "exitcode", -1);
						if (conditions[row][i].op == EDG_WLL_QUERY_OP_WITHIN)
							edg_wll_add_int_to_XMLBody(&pomValue, conditions[row][i].value2.i, "exitcode", -1);
	                                        break;
	                                /* common conditions */
	                                case EDG_WLL_QUERY_ATTR_USERTAG:
	                                        edg_wll_add_tagged_string_to_XMLBody(&pomValue, conditions[row][i].value.c, "usertag", conditions[row][i].attr_id.tag, "name", NULL);
	                                        break;
	                                case EDG_WLL_QUERY_ATTR_JDL_ATTR:
						if (conditions[row][i].op != EDG_WLL_QUERY_OP_CHANGED)
	                                        	edg_wll_add_tagged_string_to_XMLBody(&pomValue, conditions[row][i].value.c, "jdl", conditions[row][i].attr_id.tag, "name", NULL);
						else {
							char	*newBody, *tag = conditions[row][i].attr_id.tag;
							if (tag) asprintf(&newBody,"%s\t\t\t<jdl name=\"%s\"/>\r\n",pomValue,tag);
							else asprintf(&newBody,"%s\t\t\t<jdl/>\r\n",pomValue);

							free(pomValue);
							pomValue = newBody;
						}
	                                        break;
					case EDG_WLL_QUERY_ATTR_STATEENTERTIME:
						if (conditions[row][i].op == EDG_WLL_QUERY_OP_WITHIN) {
							edg_wll_add_time_t_to_XMLBody(&pomValue, conditions[row][i].value.t.tv_sec, "stateentertime", -1);
							edg_wll_add_time_t_to_XMLBody(&pomValue, conditions[row][i].value2.t.tv_sec, "stateentertime", -1);	
						}
						else 
							edg_wll_add_time_t_to_XMLBody(&pomValue, conditions[row][i].value.t.tv_sec, "stateentertime", -1);
						break;
					case EDG_WLL_QUERY_ATTR_LASTUPDATETIME:
						if (conditions[row][i].op == EDG_WLL_QUERY_OP_WITHIN) {
							edg_wll_add_time_t_to_XMLBody(&pomValue, conditions[row][i].value.t.tv_sec, "lastupdatetime", -1);
							edg_wll_add_time_t_to_XMLBody(&pomValue, conditions[row][i].value2.t.tv_sec, "lastupdatetime", -1);	
						}
						else 
							edg_wll_add_time_t_to_XMLBody(&pomValue, conditions[row][i].value.t.tv_sec, "lastupdatetime", -1);
						break;
	                                case EDG_WLL_QUERY_ATTR_JOB_TYPE:
	                                        edg_wll_add_string_to_XMLBody(&pomValue, edg_wll_StatusJobtypeNames[conditions[row][i].value.i],
							 "jobtype", NULL);
						break;
					case EDG_WLL_QUERY_ATTR_VM_STATUS:
						/*XXX currently no within or changed implemented */
						edg_wll_add_edg_wll_StatVm_state_to_XMLBody(&pomValue, conditions[row][i].value.i, "vm_status", -1); /* have no undef state*/
	                                        break;

	                        default:
	                                free(pomValue);
	                                return -1;
	                }
	                switch (conditions[row][i].op) {
	                        case EDG_WLL_QUERY_OP_EQUAL: pomOp = "equal"; break;
	                        case EDG_WLL_QUERY_OP_LESS: pomOp = "less"; break;
	                        case EDG_WLL_QUERY_OP_GREATER: pomOp = "greater"; break;
				case EDG_WLL_QUERY_OP_WITHIN: pomOp = "within"; break;
				case EDG_WLL_QUERY_OP_UNEQUAL: pomOp = "unequal"; break;
				case EDG_WLL_QUERY_OP_CHANGED: pomOp = "changed"; break;
	                        default:
	                                return -1;
	                }
	
	                len = asprintf(&list[i],"\t\t\t<%s>\n\t\t%s\t\t\t</%s>\r\n",
	                                pomOp,pomValue,pomOp);
	                tot_len += len;
	                len_list[i] = len;
	
	                free(pomValue);
	
	                i++;
	        }
	
	
	        pomA = (char *) malloc(tot_len  * sizeof(char) +
	                sizeof(QUERY_JOBS_OR_BEGIN) + sizeof(QUERY_JOBS_OR_END) - 1);
	
	        memcpy(pomA, QUERY_JOBS_OR_BEGIN, sizeof(QUERY_JOBS_OR_BEGIN));
	        pomB = pomA + sizeof(QUERY_JOBS_OR_BEGIN) - 1;
	
	        for (i=0; i < nconditions; i++) {
	                memcpy(pomB, list[i], len_list[i] );
	                pomB += len_list[i];
	                free(list[i]);
	        }
	        free(list);
	        free(len_list);
		
		strcpy(pomB, QUERY_JOBS_OR_END);
		asprintf(message,"%s%s", pomC, pomA);
		free(pomA);
		free(pomC);
		pomC = *message;		
		*message = NULL;

		row++;
	}


	asprintf(message,"%s", pomC);	

	free(pomC);

        return 0;
}




/* construct Message-Body of Request-Line for edg_wll_QueryEvents */
int edg_wll_QueryEventsRequestToXML(
		edg_wll_Context ctx,
                const edg_wll_QueryRec **job_conditions,
                const edg_wll_QueryRec **event_conditions,
                char **message)
{
        char *pomA, *pomB, *pomC;
        int i = 0, len, tot_len = 0, row = 0;
        int nevent_conditions;
        char **list;
        int *len_list;

	
	edg_wll_JobQueryRecToXML(ctx, job_conditions, &pomC);

	row = 0;
	while(event_conditions && event_conditions[row]) {	

		pomA = pomB = NULL;

	        for (i=0; event_conditions[row][i].attr != EDG_WLL_QUERY_ATTR_UNDEF ; i++);
	        nevent_conditions = i;
	
                list = (char **) malloc ((nevent_conditions) * sizeof(*list));
                len_list = (int *) malloc ((nevent_conditions) * sizeof(*len_list));

		i=0;
		while (i < nevent_conditions) {
			const char *pomOp;
			char       *pomValue;

			pomValue = strdup("");
	
			switch (event_conditions[row][i].attr) {
				/* event specific conditions */
				case EDG_WLL_QUERY_ATTR_TIME:
					if (event_conditions[row][i].op == EDG_WLL_QUERY_OP_WITHIN) {
						edg_wll_add_tagged_time_t_to_XMLBody(&pomValue, event_conditions[row][i].value.t.tv_sec, "time", edg_wll_StatToString(event_conditions[row][i].attr_id.state), "state", -1);
						edg_wll_add_tagged_time_t_to_XMLBody(&pomValue, event_conditions[row][i].value2.t.tv_sec, "time", edg_wll_StatToString(event_conditions[row][i].attr_id.state), "state", -1);	
					}
					else 
						edg_wll_add_tagged_time_t_to_XMLBody(&pomValue, event_conditions[row][i].value.t.tv_sec, "time", edg_wll_StatToString(event_conditions[row][i].attr_id.state), "state", -1);
					break;
				case EDG_WLL_QUERY_ATTR_STATEENTERTIME:
					if (event_conditions[row][i].op == EDG_WLL_QUERY_OP_WITHIN) {
						edg_wll_add_time_t_to_XMLBody(&pomValue, event_conditions[row][i].value.t.tv_sec, "stateentertime", -1);
						edg_wll_add_time_t_to_XMLBody(&pomValue, event_conditions[row][i].value2.t.tv_sec, "stateentertime", -1);	
					}
					else 
						edg_wll_add_time_t_to_XMLBody(&pomValue, event_conditions[row][i].value.t.tv_sec, "stateentertime", -1);
					break;
				case EDG_WLL_QUERY_ATTR_LEVEL:
					edg_wll_add_int_to_XMLBody(&pomValue, event_conditions[row][i].value.i, "level", -1);
					if (event_conditions[row][i].op == EDG_WLL_QUERY_OP_WITHIN)
						edg_wll_add_int_to_XMLBody(&pomValue, event_conditions[row][i].value2.i, "level", -1);
					break;
				case EDG_WLL_QUERY_ATTR_HOST:
					edg_wll_add_string_to_XMLBody(&pomValue, event_conditions[row][i].value.c, "host", NULL);
					break;
				case EDG_WLL_QUERY_ATTR_SOURCE:
					edg_wll_add_int_to_XMLBody(&pomValue, event_conditions[row][i].value.i, "source", -1);
					break;
				case EDG_WLL_QUERY_ATTR_INSTANCE:
					edg_wll_add_string_to_XMLBody(&pomValue, event_conditions[row][i].value.c, "instance", NULL);
					break;
				case EDG_WLL_QUERY_ATTR_EVENT_TYPE:
					edg_wll_add_int_to_XMLBody(&pomValue, event_conditions[row][i].value.i, "type", EDG_WLL_EVENT_UNDEF);
					if (event_conditions[row][i].op == EDG_WLL_QUERY_OP_WITHIN)
						edg_wll_add_int_to_XMLBody(&pomValue, event_conditions[row][i].value2.i, "type", EDG_WLL_EVENT_UNDEF);
					break;
				/* common conditions */
				case EDG_WLL_QUERY_ATTR_USERTAG:
					edg_wll_add_tagged_string_to_XMLBody(&pomValue, event_conditions[row][i].value.c, "usertag", event_conditions[row][i].attr_id.tag, "name", NULL);
					break;
				case EDG_WLL_QUERY_ATTR_JDL_ATTR:
					edg_wll_add_tagged_string_to_XMLBody(&pomValue, event_conditions[row][i].value.c, "jdl", event_conditions[row][i].attr_id.tag, "name", NULL);
					break;
				default:
					free(pomValue);
					return -1;
			}
			switch (event_conditions[row][i].op) {
				case EDG_WLL_QUERY_OP_EQUAL: pomOp = "equal"; break;
				case EDG_WLL_QUERY_OP_LESS: pomOp = "less"; break;
				case EDG_WLL_QUERY_OP_GREATER: pomOp = "greater"; break;
				case EDG_WLL_QUERY_OP_WITHIN: pomOp = "within"; break;
				case EDG_WLL_QUERY_OP_UNEQUAL: pomOp = "unequal"; break;
				default:
					return -1;
			}
	
			len = asprintf(&list[i],"\t\t\t<%s>\n\t\t%s\t\t\t</%s>\r\n",
					pomOp,pomValue,pomOp);
	
			tot_len += len;
			len_list[i] = len;
	
			free(pomValue);
	
			i++;
		}

                pomA = (char *) malloc(tot_len  * sizeof(char) +
                        sizeof(QUERY_EVENTS_OREC_BEGIN) + sizeof(QUERY_EVENTS_OREC_END) - 1);

                memcpy(pomA, QUERY_EVENTS_OREC_BEGIN, sizeof(QUERY_EVENTS_OREC_BEGIN));
                pomB = pomA + sizeof(QUERY_EVENTS_OREC_BEGIN) - 1;

                for (i=0; i < nevent_conditions; i++) {
                        memcpy(pomB, list[i], len_list[i] );
                        pomB += len_list[i];
                        free(list[i]);
                }
                free(list);
                free(len_list);

                strcpy(pomB, QUERY_EVENTS_OREC_END);
                asprintf(message,"%s%s", pomC, pomA);
                free(pomA);
                free(pomC);
                pomC = *message;
                *message = NULL;

		row++;
	}	
	
	asprintf(message,"%s softLimit=\"%d\" queryRes=\"%d\">\r\n\t<and>\r\n%s%s",
		QUERY_EVENTS_REQUEST_BEGIN, 
		ctx->p_query_events_limit, ctx->p_query_results,
		pomC, QUERY_EVENTS_REQUEST_END);	

	free(pomC);

	return 0;
}



/* construct Message-Body of Request-Line for edg_wll_QueryJobs */
int edg_wll_QueryJobsRequestToXML(
		edg_wll_Context ctx,
		const edg_wll_QueryRec **conditions,
                int flags,
                char **message)
{
        char *pomC, *cflags;

	
	edg_wll_JobQueryRecToXML(ctx, conditions, &pomC);	

	asprintf(message,
		"%s softLimit=\"%d\" queryRes=\"%d\">\r\n\t<flags>%s</flags>\r\n\t<and>\r\n%s%s",
		QUERY_JOBS_REQUEST_BEGIN,
		ctx->p_query_jobs_limit, ctx->p_query_results,
		cflags = edg_wll_stat_flags_to_string(flags), pomC, QUERY_JOBS_REQUEST_END);	

	free(cflags);
	free(pomC);

        return 0;
}



/* construct Message-Body of Request-Line for edg_wll_Purge */
int edg_wll_PurgeRequestToXML(
                edg_wll_Context ctx,
		const edg_wll_PurgeRequest *request,
                char **message)
{
        char *pomA, *pomB, *pomC, *pomMSG;


	if (!request) { *message = NULL; return(-1); }

	pomA = strdup("");
	if (request->jobs) edg_wll_add_strlist_to_XMLBody(&pomA, request->jobs, "jobs",
								"jobId", "\t", NULL);

	/* print timeout for all status codes */
	pomB = strdup("");
	edg_wll_add_time_t_list_to_XMLBody(&pomB, request->timeout, "timeout", edg_wll_StatToString, "\t",
						0, EDG_WLL_NUMBER_OF_STATCODES);

	pomC = strdup("");
	edg_wll_add_time_t_to_XMLBody(&pomC, request->target_runtime, "target_runtime", 0);

	trio_asprintf(&pomMSG,"%s%s%s%s\t<flags>%|Xs</flags>\r\n%s",
			PURGE_REQUEST_BEGIN,pomA,pomB,pomC,edg_wll_purge_flags_to_string(request->flags),
			PURGE_REQUEST_END);

	free(pomA);
	free(pomB);
	free(pomC);

        *message = pomMSG;

        return 0;
}



/* construct Message-Body of Request-Line for edg_wll_Dump */
int edg_wll_DumpRequestToXML(
                edg_wll_Context ctx,
		const edg_wll_DumpRequest *request,
                char **message)
{
        char *pomA, *pomB;


	if (!request) { *message = NULL; return(-1); }

	pomA = strdup("");
	if (request->from < 0)
		edg_wll_add_string_to_XMLBody(&pomA, 
			edg_wll_DumpConstToString(request->from), "from", NULL);
	else
	edg_wll_add_time_t_to_XMLBody(&pomA, request->from, "from", 0);
	if (request->to < 0)
		edg_wll_add_string_to_XMLBody(&pomA,
			edg_wll_DumpConstToString(request->to), "to", NULL);
	else
	edg_wll_add_time_t_to_XMLBody(&pomA, request->to, "to", 0);

	trio_asprintf(&pomB,"%s%s%s",
			DUMP_REQUEST_BEGIN,pomA,DUMP_REQUEST_END);

	free(pomA);


        *message = pomB;

        return 0;
}



/* construct Message-Body of Request-Line for edg_wll_Load */
int edg_wll_LoadRequestToXML(
                edg_wll_Context ctx,
		const edg_wll_LoadRequest *request,
                char **message)
{
        char *pomA, *pomB;


	if (!request) { *message = NULL; return(-1); }

	pomA = strdup("");
	edg_wll_add_string_to_XMLBody(&pomA, request->server_file, "server_file", 0);

	trio_asprintf(&pomB,"%s%s%s",
			LOAD_REQUEST_BEGIN,pomA,LOAD_REQUEST_END);

	free(pomA);


        *message = pomB;

        return 0;
}



/* construct Message-Body of Request-Line for edg_wll_IndexedAttrs */
int edg_wll_IndexedAttrsRequestToXML(
                edg_wll_Context ctx,
                char **message)
{
	char *pomB;


	trio_asprintf(&pomB,"%s%s",
			INDEXED_ATTRS_REQUEST_BEGIN,INDEXED_ATTRS_REQUEST_END);

        *message = pomB;

        return 0;
}



/* construct Message-Body of Request-Line for edg_wll_Notif* functions */
int edg_wll_NotifRequestToXML(
                edg_wll_Context ctx, 
		const char *function,
		const edg_wll_NotifId notifId,
		const char *address,
		edg_wll_NotifChangeOp op,
		time_t validity,
		edg_wll_QueryRec const * const *conditions,
		int flags,
                char **message)
{
	char *pomA=NULL, *pomB=NULL, *pomC=NULL, *cflags=NULL, *nid=NULL, *nop=NULL;


	pomA = strdup("");
	nid = edg_wll_NotifIdUnparse(notifId);
	nop = edg_wll_NotifChangeOpToString(op);
	edg_wll_add_string_to_XMLBody(&pomA, nid, "notifId", NULL);
	edg_wll_add_string_to_XMLBody(&pomA, address, "clientAddress", NULL);
	edg_wll_add_string_to_XMLBody(&pomA, nop, "notifChangeOp", NULL);
	edg_wll_add_time_t_to_XMLBody(&pomA, validity, "requestedValidity", -1);
	if (conditions && conditions[0] && conditions[0][0].attr != EDG_WLL_QUERY_ATTR_UNDEF)
		edg_wll_JobQueryRecToXML(ctx, conditions, &pomB);
	cflags = edg_wll_stat_flags_to_string(flags);

	if (pomB)
		trio_asprintf(&pomC,"%s function=\"%s\">\r\n\t<flags>%s</flags>\r\n%s\t<and>\r\n%s\t</and>\r\n%s",
			NOTIF_REQUEST_BEGIN,function,cflags,pomA,pomB,NOTIF_REQUEST_END);
	else
		trio_asprintf(&pomC,"%s function=\"%s\">\r\n\t<flags>%s</flags>\r\n%s%s",
			NOTIF_REQUEST_BEGIN,function,cflags,pomA,NOTIF_REQUEST_END);


	free(cflags);
	free(nid);
	free(nop);
	free(pomA);
	free(pomB);
        *message = pomC;

        return 0;
}


/* construct Message-Body of Request-Line for edg_wll_QuerySequeceCode function */
int edg_wll_QuerySequenceCodeToXML(
		edg_wll_Context ctx,
		glite_jobid_const_t jobId,
                char **message)
{
	char *pomA=NULL, *pomC=NULL;
	char *pomSource=NULL;

	pomA = strdup("");
	edg_wll_add_jobid_to_XMLBody(&pomA, jobId, "jobId", NULL);
	edg_wll_add_string_to_XMLBody(&pomA, pomSource = edg_wll_SourceToString(ctx->p_source), "source", NULL);

	trio_asprintf(&pomC,"%s%s%s",
		QUERY_SEQUENCE_CODE_REQUEST_BEGIN,pomA,QUERY_SEQUENCE_CODE_REQUEST_END);


	free(pomA);
	free(pomSource);
        *message = pomC;

        return 0;
}


/* construct Message-Body of Request-Line for edg_wll_StateRate function */
int edg_wll_StatsRequestToXML(
                edg_wll_Context         ctx,
                const char              *function,
                const edg_wll_QueryRec  *cond,
                edg_wll_JobStatCode     major,
                int                     minor,
                time_t                  *from,
                time_t                  *to,
                char 			**message)
{
	return edg_wll_StatsDurationFTRequestToXML(ctx, function, cond, major, EDG_WLL_JOB_UNDEF, minor, from, to, message);
}

int edg_wll_StatsDurationFTRequestToXML(
                edg_wll_Context 	ctx, 
		const char		*function,
		const edg_wll_QueryRec 	*cond,
		edg_wll_JobStatCode     base_state,
		edg_wll_JobStatCode     final_state,
		int			minor,
		time_t			*from,
		time_t			*to,
                char 			**message)
{
	char *pomA=NULL, *pomB=NULL, *pomC=NULL;
	const edg_wll_QueryRec **conditions;

	
        conditions = (const edg_wll_QueryRec **) malloc(2 * sizeof(edg_wll_QueryRec *));
        conditions[1] = NULL;

	conditions[0] = cond;
	conditions[1] = NULL;

	pomA = strdup("");
	
	edg_wll_add_edg_wll_JobStatCode_to_XMLBody(&pomA, base_state, "base_state", EDG_WLL_JOB_UNDEF);
	edg_wll_add_edg_wll_JobStatCode_to_XMLBody(&pomA, final_state, "final_state", EDG_WLL_JOB_UNDEF);
	edg_wll_add_int_to_XMLBody(&pomA, minor, "minor", -1);
	edg_wll_add_time_t_to_XMLBody(&pomA, *from, "from", 0); 
	edg_wll_add_time_t_to_XMLBody(&pomA, *to, "to", 0); 

	if (conditions && conditions[0] && conditions[0][0].attr != EDG_WLL_QUERY_ATTR_UNDEF)
		edg_wll_JobQueryRecToXML(ctx, (edg_wll_QueryRec const * const *) conditions, &pomB);
		

	if (pomB)
		trio_asprintf(&pomC,"%s function=\"%s\">\r\n%s\t<and>\r\n%s\t</and>\r\n%s",
			STATS_REQUEST_BEGIN,function,pomA,pomB,STATS_REQUEST_END);
	else
		trio_asprintf(&pomC,"%s function=\"%s\">\r\n%s%s",
			STATS_REQUEST_BEGIN,function,pomA,STATS_REQUEST_END);


	free(pomA);
	free(pomB);
	free(conditions);
        *message = pomC;

        return 0;
}
