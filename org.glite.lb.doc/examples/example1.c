#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <expat.h>

#include "glite/lb/context.h"
#include "glite/lb/xml_conversions.h"
#include "glite/lb/consumer.h"

extern void print_jobs(edg_wll_JobStat *);

int main(int argc,char **argv)
{

	edg_wll_Context     ctx;    
	edg_wll_JobStat    *statesOut = NULL;
	edg_wll_QueryRec    jc[2];
	
	edg_wll_InitContext(&ctx);
	
	jc[0].attr = EDG_WLL_QUERY_ATTR_JOBID;
	jc[0].op = EDG_WLL_QUERY_OP_EQUAL;
	if ( edg_wlc_JobIdParse(
		     "https://lhun.ics.muni.cz:9000/OirOgeWh_F9sfMZjnIPYhQ",
		     &jc[0].value.j) )
	{
		edg_wll_FreeContext(ctx);
		exit(1);
	}
	jc[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;
	if (edg_wll_QueryJobs(ctx, jc, 0, NULL, &statesOut)) {
		char    *err_text,*err_desc;
		
		edg_wll_Error(ctx,&err_text,&err_desc);
		fprintf(stderr,"QueryJobs: %s (%s)\n",err_text,err_desc);
		free(err_text);
		free(err_desc);
	}
	else {
		print_jobs(statesOut);	/* process the returned data */
		edg_wll_FreeStatus(statesOut);
		free(statesOut);
	}
	edg_wlc_JobIdFree(jc[0].value.j);
	edg_wll_FreeContext(ctx);
	return 0;
}
