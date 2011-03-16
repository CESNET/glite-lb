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

#include "glite/lb/context.h"
#include "glite/lb/xml_conversions.h"
#ifdef BUILDING_LB_CLIENT
#include "consumer.h"
#else
#include "glite/lb/consumer.h"
#endif

int use_proxy = 0;

int (*user_jobs)(edg_wll_Context, edg_wlc_JobId **, edg_wll_JobStat **);


void
usage(char *me)
{
	fprintf(stderr,"usage: %s [-h] [-x] [userid]\n", me);
}

int main(int argc,char **argv)
{
	edg_wll_Context	ctx;
	char		*errt,*errd;
	edg_wlc_JobId		*jobs = NULL;
	edg_wll_JobStat		*states = NULL;
	int		i,j;
	char 		*owner = NULL;

	user_jobs = edg_wll_UserJobs;
	for ( i = 1; i < argc; i++ ) {
		if ( !strcmp(argv[i], "-h") || !strcmp(argv[i], "--help") ) {
			usage(argv[0]);
			exit(0);
		} else if ( !strcmp(argv[i], "-x") ) {
			user_jobs = edg_wll_UserJobsProxy;
			continue;
		}

		owner = strdup(argv[i]);
		break; 	
	}

	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		return 1;
	}
	if ( user_jobs == edg_wll_UserJobsProxy  && owner )
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_LBPROXY_USER, owner);

	if (user_jobs(ctx,&jobs,&states)) goto err;
 	for (i=0; states[i].state != EDG_WLL_JOB_UNDEF; i++) {	
		char *id = edg_wlc_JobIdUnparse(states[i].jobId),
		     *st = edg_wll_StatToString(states[i].state);
		
		if (!states[i].parent_job) {
			if (states[i].jobtype == EDG_WLL_STAT_SIMPLE) { 
				printf("      %s .... %s %s\n", id, st, (states[i].state==EDG_WLL_JOB_DONE) ? edg_wll_done_codeToString(states[i].done_code) : "" );
			}
			else if ((states[i].jobtype == EDG_WLL_STAT_DAG) || 
				(states[i].jobtype == EDG_WLL_STAT_COLLECTION)) {
				printf("%s  %s .... %s %s\n", (states[i].jobtype==EDG_WLL_STAT_DAG)?"DAG ":"COLL",id, st, (states[i].state==EDG_WLL_JOB_DONE) ? edg_wll_done_codeToString(states[i].done_code) : "");
				for (j=0; states[j].state != EDG_WLL_JOB_UNDEF; j++) {
					if (states[j].parent_job) {
						char *par_id = edg_wlc_JobIdUnparse(states[j].parent_job);
						
						if (!strcmp(id,par_id)) {
							char *sub_id = edg_wlc_JobIdUnparse(states[j].jobId),
							     *sub_st = edg_wll_StatToString(states[j].state);
							
							printf(" `-       %s .... %s %s\n", sub_id, sub_st, (states[j].state==EDG_WLL_JOB_DONE) ? edg_wll_done_codeToString(states[j].done_code) : "");
							free(sub_id);
							free(sub_st);
						}
						free(par_id);
					}	
				}
			}
		}
			
		free(id);
		free(st);
	}

	printf("\nFound %d jobs\n",i);

err:
	free(owner);
	if  (jobs) {
		for (i=0; jobs[i]; i++)  edg_wlc_JobIdFree(jobs[i]);	
		free(jobs);
	}

	if  (states) {
		for (i=0; states[i].state; i++)  edg_wll_FreeStatus(&states[i]);	
		free(states);
	}

	if (edg_wll_Error(ctx,&errt,&errd)) {
		fprintf(stderr,"%s: %s (%s)\n",argv[0],errt,errd);
		edg_wll_FreeContext(ctx);
		return 1;
	}

	edg_wll_FreeContext(ctx);
	return 0;
}

