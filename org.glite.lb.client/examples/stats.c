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
#include <time.h>
#include <string.h>

#ifdef BUILDING_LB_CLIENT
#include "statistics.h"
#else
#include "glite/lb/statistics.h"
#endif


int main(int argc,char **argv)
{
	edg_wll_Context	ctx;
	edg_wll_QueryRec	group[2];
	time_t	now,from,to;
	char	*cfrom,*cto;
	int	from_res,to_res,argOK = 3, era = 60;
	int	CEidx = 1, MAJidx = 2, MINidx = 3; 
	float	*vals;
	char	**groups;
	int 	i;


	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		return 1;
	}

	if (!strcmp(argv[1],"-n")) {
		era = atoi(argv[2]);
		CEidx = 3; MAJidx = 4; MINidx = 5;
		argOK = 5;
	}


	if (argc < argOK) { 
		fprintf(stderr,"usage: %s [-n sec] CE major [minor]\n",argv[0]);
		return 1;
	}	



/* the only supported grouping for now */
	group[0].attr = EDG_WLL_QUERY_ATTR_DESTINATION;
	group[0].op = EDG_WLL_QUERY_OP_EQUAL;
	if (strcmp(argv[CEidx], "ALL"))
		group[0].value.c = argv[CEidx];
	else
		group[0].value.c = NULL;
	group[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;


	time(&now);
	to = now;
	from = now - (time_t)era;
	
	if (edg_wll_StateRates(ctx,group,atoi(argv[MAJidx]),argc >= (argOK+1) ? atoi(argv[MINidx]) : 0,
				&from,&to,&vals,&groups,&from_res,&to_res))
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

	for (i = 0; groups[i]; i++)
		printf("Average failure rate at \"%s\": %f jobs/s\n"
		       "  Measuered from %s to %s\n"
		       "  With resolution from %d to %d s\n",
	       		/*argv[1]*/groups[i],vals[i],cfrom,cto,from_res,to_res);

	free(vals);
	for (i = 0; groups[i]; i++)
		free(groups[i]);
	free(groups);
	return 0;
}

