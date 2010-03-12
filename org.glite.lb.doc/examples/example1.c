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
		     argv[1],
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
