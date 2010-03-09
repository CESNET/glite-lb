#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "glite/lbu/trio.h"

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/file_plugin.h"
#include "glite/jp/attr.h"
#include "glite/lb/job_attrs.h"

/*
 * free the array of JP attr
 */
static void free_attrs(glite_jp_attrval_t *av) {
        glite_jp_attrval_t *item;

        item = av;
        while (item->name) {
                glite_jp_attrval_free(item++, 0);
        }
        free(av);
}

/*
 * process attributes
 */
void process_attrs(glite_jp_context_t jpctx, glite_jpps_fplug_data_t plugin_data, void *data_handle, FILE *outfile) {

	glite_jp_attrval_t *attrval;

	/* <header> */
	trio_fprintf(outfile,"<?xml version=\"1.0\"?>\n\n");
	trio_fprintf(outfile,"<lbd:jobRecord\n");
	trio_fprintf(outfile,"xmlns:lbd=\"http://glite.org/wsdl/types/lbdump\"\n");
	
	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_jobId, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"jobid=\"%|Xs\"\n", attrval->value);
		free_attrs(attrval);
	} else {
		trio_fprintf(outfile,"jobid=\"default\"\n");
	}
	trio_fprintf(outfile,">\n");
	/* </header> */
	
	/* <body> */
	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_user, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<user>%|Xs</user>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_parent, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<parent>%|Xs</parent>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_VO, &attrval);
	if (attrval) {
		trio_fprintf(stdout,"<VO>%|Xs</VO>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_aTag, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<aTag>%|Xs</aTag>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_rQType, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<rQType>%|Xs</rQType>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_eDuration, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<eDuration>%|Xs</eDuration>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_eNodes, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<eNodes>%|Xs</eNodes>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_eProc, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<eProc>%|Xs</eProc>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_RB, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<RB>%|Xs</RB>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_CE, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<CE>%|Xs</CE>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_host, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<host>%|Xs</host>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_UIHost, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<UIHost>%|Xs</UIHost>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_CPUTime, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<CPUTime>%|Xs</CPUTime>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_NProc, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<NProc>%|Xs</NProc>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_finalStatus, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<finalStatus>%|Xs</finalStatus>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_finalDoneStatus, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<finalDoneStatus>%|Xs</finalDoneStatus>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_finalStatusDate, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<finalStatusDate>%|Xs</finalStatusDate>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_finalStatusReason, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<finalStatusReason>%|Xs</finalStatusReason>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_LRMSDoneStatus, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<LRMSDoneStatus>%|Xs</LRMSDoneStatus>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_LRMSStatusReason, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<LRMSStatusReason>%|Xs</LRMSStatusReason>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_retryCount, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<retryCount>%|Xs</retryCount>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_additionalReason, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<additionalReason>%|Xs</additionalReason>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_jobType, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<jobType>%|Xs</jobType>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_nsubjobs, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<nsubjobs>%|Xs</nsubjobs>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_subjobs, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<subjobs>%|Xs</subjobs>\n", attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_lastStatusHistory, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<lastStatusHistory>%s</lastStatusHistory>\n",attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_fullStatusHistory, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<fullStatusHistory>%s</fullStatusHistory>\n",attrval->value);
		free_attrs(attrval);
	}

	plugin_data.ops.attr(jpctx, data_handle, GLITE_JP_LB_JDL, &attrval);
	if (attrval) {
		trio_fprintf(outfile,"<JDL>%|Xs</JDL>\n", attrval->value);
		free_attrs(attrval);
	}

	/* </body> */

	trio_fprintf(outfile,"</lbd:jobRecord>\n\n");
}
