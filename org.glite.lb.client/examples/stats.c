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
#include <time.h>
#include <string.h>

#include "glite/lb/statistics.h"


int main(int argc,char **argv)
{
	edg_wll_Context	ctx;
	edg_wll_QueryRec	group[2];
	time_t	now,from,to;
	char	*cfrom,*cto;
	int	from_res,to_res;
	float	val;


	edg_wll_InitContext(&ctx);

	if (argc != 2) { 
		fprintf(stderr,"usage: %s <CE name>\n",argv[0]);
		return 1;
	}	

/* the only supported grouping for now */
	group[0].attr = EDG_WLL_QUERY_ATTR_DESTINATION;
	group[0].op = EDG_WLL_QUERY_OP_EQUAL;
	group[0].value.c = argv[1];
	group[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;


	time(&now);
	to = now;
	from = now - 60;
	
/* not implemented yet
	if (edg_wll_StateDuration(ctx,group,EDG_WLL_JOB_SCHEDULED,0,
				&from,&to,&val,&from_res,&to_res))
	{
		char	*et,*ed;
		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"edg_wll_StateDuration(): %s, %s\n",et,ed);
		return 1;
	}

	cfrom = strdup(ctime(&from));
	cto = strdup(ctime(&to));
	cfrom[strlen(cfrom)-1] = 0;
	cto[strlen(cto)-1] = 0;

	printf("Average queue traversal time at \"%s\": %f s\n"
	       "  Measuered from %s to %s\n"
	       "  With resolution from %d to %d s\n",
	       argv[1],val,cfrom,cto,from_res,to_res);

*/

	to = now;
	from = now - 60;

	if (edg_wll_StateRate(ctx,group,EDG_WLL_JOB_DONE,EDG_WLL_STAT_FAILED,
				&from,&to,&val,&from_res,&to_res))
	{
		char	*et,*ed;
		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"edg_wll_StateRate(): %s, %s\n",et,ed);
		return 1;
	}

	cfrom = strdup(ctime(&from));
	cto = strdup(ctime(&to));
	cfrom[strlen(cfrom)-1] = 0;
	cto[strlen(cto)-1] = 0;

	printf("Average failure rate at \"%s\": %f jobs/s\n"
	       "  Measuered from %s to %s\n"
	       "  With resolution from %d to %d s\n",
	       argv[1],val,cfrom,cto,from_res,to_res);

	return 0;
}

