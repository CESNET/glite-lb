#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <expat.h>

#include "glite/lb/context.h"
#include "glite/lb/xml_conversions.h"
#include "glite/lb/consumer.h"

int main(int argc,char **argv)
{
	edg_wll_Context	ctx;
	char		*errt,*errd;
	edg_wlc_JobId		*jobs = NULL;
	edg_wll_JobStat		*states = NULL;
	int		i,j;
	char 		*owner = NULL;

	switch (argc) {
		case 1 : break;
	
		case 2 : /* fprintf(stderr,"\'userid\' option not implemented yet.\n"); */
			 if ( strcmp(argv[1],"--help")) {
				 owner = strdup(argv[1]);
				 break; 	
			 }
			 /* else : fall through */
	
		default: fprintf(stderr,"usage: %s [userid]\n",argv[0]);
         	         return 1;
        }
	

	edg_wll_InitContext(&ctx);

	if (edg_wll_UserJobs(ctx,&jobs,&states)) goto err;
 	for (i=0; states[i].state != EDG_WLL_JOB_UNDEF; i++) {	
		char *id = edg_wlc_JobIdUnparse(states[i].jobId),
		     *st = edg_wll_StatToString(states[i].state);
		
		if (!states[i].parent_job) {
			if (states[i].jobtype == EDG_WLL_STAT_SIMPLE) { 
				printf("      %s .... %s %s\n", id, st, (states[i].state==EDG_WLL_JOB_DONE) ? edg_wll_done_codeToString(states[i].done_code) : "" );
			}
			else if (states[i].jobtype == EDG_WLL_STAT_DAG) {
				printf("DAG   %s .... %s %s\n", id, st, (states[i].state==EDG_WLL_JOB_DONE) ? edg_wll_done_codeToString(states[i].done_code) : "");
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

