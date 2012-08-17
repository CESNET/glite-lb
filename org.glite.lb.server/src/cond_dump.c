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


#include "glite/lb/context-int.h"
#include "lb_proto.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#define TR(name,type,field)             \
        if (field) {            \
                asprintf(&pomA,"%s<tr><th align=\"left\">" name ":</th>"        \
                        "<td>" type "</td></tr>",pomB,(field)); \
                free(pomB);                                     \
                pomB = pomA;                                    \
        }

#define GS(string){	\
	asprintf(&pomA, "%s %s", pomB, (string)); \
	free(pomB); \
	pomB = pomA; \
}

int edg_wll_Condition_Dump(notifInfo *ni, char **output, int oneline){
	if (! ni->conditions){
		*output = strdup("");
		return -1;
	}

	char *pomA = NULL, *pomB;
	pomB = strdup("");

	edg_wll_QueryRec **l1;
	edg_wll_QueryRec *l2;
	for (l1 = ni->conditions; *l1; l1++){
		if (l1 != ni->conditions)
			GS ("and");
		if (oneline)
			GS("(");
		l2 = *l1;
		switch (l2->attr){
			case EDG_WLL_QUERY_ATTR_USERTAG:
				GS(l2->attr_id.tag);
				break;
			case EDG_WLL_QUERY_ATTR_JDL_ATTR: 
				GS(l2->attr_id.tag);  //get JDL attribute name
				break;
			default:
				GS(edg_wll_QueryAttrNames[l2->attr]);
				break;
		}
		for (l2 = *l1; l2->attr; l2++){
			if (l2 != *l1 && !oneline) GS ("	or");
			if (l2 != *l1 && oneline) GS("or");
			switch(l2->op){
				case EDG_WLL_QUERY_OP_EQUAL: GS("=");
					break;
				case EDG_WLL_QUERY_OP_LESS: GS ("<");
					break;
				case EDG_WLL_QUERY_OP_GREATER: GS ("<");
					break;
				case EDG_WLL_QUERY_OP_WITHIN: GS ("within");
					break;
				case EDG_WLL_QUERY_OP_UNEQUAL: GS ("!=");
					break;
				case EDG_WLL_QUERY_OP_CHANGED: GS ("->");
					break;
			}
			char *buf;
			switch (l2->attr){
		                case EDG_WLL_QUERY_ATTR_JOBID:
				case EDG_WLL_QUERY_ATTR_PARENT:
					GS(edg_wlc_JobIdUnparse(l2->value.j));
        	                        break;
				case EDG_WLL_QUERY_ATTR_DESTINATION:
				case EDG_WLL_QUERY_ATTR_LOCATION:
                	        case EDG_WLL_QUERY_ATTR_OWNER:
				case EDG_WLL_QUERY_ATTR_HOST:
				case EDG_WLL_QUERY_ATTR_INSTANCE:
				case EDG_WLL_QUERY_ATTR_JDL_ATTR:
				case EDG_WLL_QUERY_ATTR_NETWORK_SERVER:
					GS(l2->value.c);
                        	        break;
	                        case EDG_WLL_QUERY_ATTR_STATUS:
					if (l2->op == EDG_WLL_QUERY_OP_WITHIN)
                                                asprintf(&buf, "%s and %s",
                                                        edg_wll_StatToString((edg_wll_JobStatCode)l2->value.i), 
							edg_wll_StatToString((edg_wll_JobStatCode)l2->value2.i));
					else
						asprintf(&buf, "%s", edg_wll_StatToString((edg_wll_JobStatCode)l2->value.i));
					GS(buf);
					free(buf);
        	                        break;
                	        case EDG_WLL_QUERY_ATTR_DONECODE:
				case EDG_WLL_QUERY_ATTR_EXITCODE:
					if (l2->op == EDG_WLL_QUERY_OP_WITHIN)
						asprintf(&buf, "%i and %i",
                                                        l2->value.i, l2->value2.i);
					else
	                                        asprintf(&buf, "%i", l2->value.i);
                                        GS(buf);
                                        free(buf);
                        	        break;
				case EDG_WLL_QUERY_ATTR_EVENT_TYPE:
					if (l2->op == EDG_WLL_QUERY_OP_WITHIN)
						asprintf(&buf, "%s and %s", 
							edg_wll_EventToString((edg_wll_EventCode)l2->value.i),
							edg_wll_EventToString((edg_wll_EventCode)l2->value2.i));
					else
						asprintf(&buf, "%s", 
							edg_wll_EventToString((edg_wll_EventCode)l2->value.i));
					GS(buf);
					free(buf);
                                        break;
	                        case EDG_WLL_QUERY_ATTR_USERTAG:
					GS(l2->attr_id.tag);
        	                        break;
                	        case EDG_WLL_QUERY_ATTR_TIME:
				case EDG_WLL_QUERY_ATTR_STATEENTERTIME:
                                case EDG_WLL_QUERY_ATTR_LASTUPDATETIME:
					buf = strdup(ctime(&(l2->value.t.tv_sec)));
					buf[strlen(buf)-1] = 0; // cut out '\n'
					if (l2->op == EDG_WLL_QUERY_OP_WITHIN){
						char *buf_ptr = buf;
						asprintf(&buf, "%s and %s", buf_ptr, ctime(&(l2->value2.t.tv_sec)));
						free(buf_ptr);
						buf[strlen(buf)-1] = 0;
						GS(buf);
					}
					else
						GS(buf);
					free(buf);
                        	        break;
	                        case EDG_WLL_QUERY_ATTR_LEVEL:
					if (l2->op == EDG_WLL_QUERY_OP_WITHIN)
						asprintf(&buf, "%i and %i",
							l2->value.i, l2->value2.i);
					else
						asprintf(&buf, "%i", l2->value.i);
					GS(buf);	
					free(buf);
        	                        break;
	                        case EDG_WLL_QUERY_ATTR_SOURCE:
					buf = edg_wll_SourceToString(l2->value.i);
					GS(buf);
					free(buf);
					if (l2->op == EDG_WLL_QUERY_OP_WITHIN){
						GS("and");
						buf = edg_wll_SourceToString(l2->value2.i);
	                                        GS(buf);
        	                                free(buf);
					}
        	                        break;
                	        case EDG_WLL_QUERY_ATTR_CHKPT_TAG:
					//XXX: what kind of data is it?
                        	        break;
	                        case EDG_WLL_QUERY_ATTR_RESUBMITTED:
					asprintf(&buf, "%i", l2->value.i);
					GS(buf);
                                        free(buf);
        	                        break;
	                        case EDG_WLL_QUERY_ATTR_JOB_TYPE:
					asprintf(&buf, "%s", edg_wll_StatusJobtypeNames[l2->value.i]);
					GS(buf);
					free(buf);
        	                        break;
				default:
					assert(! "Unknown condition attribute!");
					break;
	                }
			if (! oneline)
				GS("\n");
		}
		if (oneline)
			GS(")");
	}

	*output = pomA;

	return 0;
}

