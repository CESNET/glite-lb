#ident "$Header$"

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
			case EDG_WLL_QUERY_ATTR_JOBID: GS("jobId");
				break;
			case EDG_WLL_QUERY_ATTR_OWNER: GS("owner");
				break;
			case EDG_WLL_QUERY_ATTR_STATUS: GS("status");
				break;
			case EDG_WLL_QUERY_ATTR_LOCATION: GS("location");
				break;
                        case EDG_WLL_QUERY_ATTR_DESTINATION: GS("destination");
				break;
			case EDG_WLL_QUERY_ATTR_DONECODE: GS("donecode");
				break;
			case EDG_WLL_QUERY_ATTR_USERTAG: GS("usertag");
				break;
			case EDG_WLL_QUERY_ATTR_TIME: GS("time");
				break;
			case EDG_WLL_QUERY_ATTR_LEVEL: GS("level");
				break;
			case EDG_WLL_QUERY_ATTR_HOST: GS("host");
				break;
                        case EDG_WLL_QUERY_ATTR_SOURCE: GS("source");
				break;
                        case EDG_WLL_QUERY_ATTR_INSTANCE: GS("instance");
				break;
                        case EDG_WLL_QUERY_ATTR_EVENT_TYPE: GS("eventtype");
				break;
			case EDG_WLL_QUERY_ATTR_CHKPT_TAG: GS("chkpttag");
				break;
			case EDG_WLL_QUERY_ATTR_RESUBMITTED: GS("resubmitted");
				break;
			case EDG_WLL_QUERY_ATTR_PARENT: GS("parent_job");
				break;
			case EDG_WLL_QUERY_ATTR_EXITCODE: GS("exitcode");
				break;
			case EDG_WLL_QUERY_ATTR_JDL_ATTR: 
				GS(l2->attr_id.tag);  //get JDL attribute name
				break;
			case EDG_WLL_QUERY_ATTR_STATEENTERTIME: GS("stateentertime");
				break;
			case EDG_WLL_QUERY_ATTR_LASTUPDATETIME: GS("lastupdatetime");
				break;
			case EDG_WLL_QUERY_ATTR_NETWORK_SERVER: GS("networkserver");
				break;
			default:
				assert(! "Unknown attribute!");
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
                                                asprintf(&buf, "%i and %i",
                                                        edg_wll_StatToString((edg_wll_JobStatCode)l2->value.i), 
							edg_wll_StatToString((edg_wll_JobStatCode)l2->value2.i));
					else
						asprintf(&buf, "%i", edg_wll_StatToString((edg_wll_JobStatCode)l2->value.i));
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
						asprintf(&buf, "%i", 
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
					buf = ctime(&(l2->value.t.tv_sec));
					if (l2->op == EDG_WLL_QUERY_OP_WITHIN){
						buf[strlen(buf)-1] = 0; // cut out '\n'
						asprintf(&buf, " and %s", ctime(&(l2->value2.t.tv_sec)));
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

