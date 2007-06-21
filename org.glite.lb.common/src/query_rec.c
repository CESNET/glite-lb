#ident "$Header$"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

#include "glite/wmsutils/jobid/cjobid.h"
#include "glite/lb/query_rec.h"

/*
 * edg_wll_QueryRec manipulation routines
 */

void edg_wll_QueryRecFree(edg_wll_QueryRec *prec)
{
	if (prec == NULL) {
		fprintf(stderr, "Error: edg_wll_QueryRecFree called with NULL parameter\n");
		return;
	}
	switch (prec->attr) {
		case EDG_WLL_QUERY_ATTR_USERTAG:
			free(prec->attr_id.tag);
		case EDG_WLL_QUERY_ATTR_OWNER: 
		case EDG_WLL_QUERY_ATTR_LOCATION:
		case EDG_WLL_QUERY_ATTR_DESTINATION:
		case EDG_WLL_QUERY_ATTR_HOST:
		case EDG_WLL_QUERY_ATTR_INSTANCE:
			if ( prec->value.c ) free(prec->value.c);
			break;
		case EDG_WLL_QUERY_ATTR_JOBID:
		case EDG_WLL_QUERY_ATTR_PARENT:
			edg_wlc_JobIdFree(prec->value.j);
			break;
		case EDG_WLL_QUERY_ATTR_STATUS:
		case EDG_WLL_QUERY_ATTR_DONECODE:
		case EDG_WLL_QUERY_ATTR_LEVEL:
		case EDG_WLL_QUERY_ATTR_SOURCE:
		case EDG_WLL_QUERY_ATTR_EVENT_TYPE:
		case EDG_WLL_QUERY_ATTR_RESUBMITTED:
		case EDG_WLL_QUERY_ATTR_TIME:
			/* do nothing */
			break;
		default:
			fprintf(stderr,"Error(edg_wll_QueryRecFree): unknown edg_wll_QueryRec.attr=%d\n", prec->attr);
			break;
	}
}

