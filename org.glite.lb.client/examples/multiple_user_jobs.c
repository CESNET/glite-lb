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
#include "consumer.h"

int use_proxy = 0;

int (*user_jobs)(edg_wll_Context, glite_jobid_t **, edg_wll_JobStat **);


void
usage(char *me)
{
        fprintf(stderr,"This example demonstrates the use of several user identities provided byi\n"
		"proxy files to access a single bkserver and retrieve appropriate information.\n\n"
		"usage: %s [-h] <proxy files>\n"
                "\t-h, --help\t show this help\n"
		"\t<proxy files>\t A list of proxy files to use to contact the bkserver\n"
		"\t             \t Give \"default\" for default (EDG_WLL_PARAM_X509_PROXY == NULL)\n"
                "\n"
        ,me);

}

int main(int argc,char **argv)
{
	edg_wll_Context	*p_ctx;
	char		*errt,*errd;
	glite_jobid_t		**jobs = NULL;
	edg_wll_JobStat		**states = NULL;
	int		i,j,k;
	int		no_of_runs;

	if ((argc<2) || !strcmp(argv[1], "-h")) {usage(argv[0]); exit(0);}

	no_of_runs = argc-1;

	p_ctx = (edg_wll_Context*) calloc (sizeof(edg_wll_Context), no_of_runs);
	jobs = (glite_jobid_t**) calloc (sizeof(glite_jobid_t*), no_of_runs);
	states = (edg_wll_JobStat**) calloc (sizeof(edg_wll_JobStat*), no_of_runs);

	user_jobs = edg_wll_UserJobs;
	for ( i = 1; i <= no_of_runs; i++ ) {
		printf ("Proxy file No. %d: %s\n",i,argv[i]);

		if (edg_wll_InitContext(&p_ctx[i-1]) != 0) {
			fprintf(stderr, "Couldn't create L&B context.\n");
			return 1;
		}
		if (strcmp(argv[i],"default")) edg_wll_SetParam(p_ctx[i-1], EDG_WLL_PARAM_X509_PROXY, argv[i]);
		if (user_jobs(p_ctx[i-1],&jobs[i-1],&states[i-1])) goto err;

	}

	for (k=0; k < no_of_runs; k++) {
		printf("Jobs retrieved using file No. %d (%s)\n"
			"------------------------------------------\n", k + 1, argv[k + 1]);
		for (i=0; states[k][i].state != EDG_WLL_JOB_UNDEF; i++) {	
			char *id = glite_jobid_unparse(states[k][i].jobId),
			     *st = edg_wll_StatToString(states[k][i].state);
			
			if (!states[k][i].parent_job) {
				if (states[k][i].jobtype == EDG_WLL_STAT_SIMPLE) { 
					printf("      %s .... %s %s\n", id, st, (states[k][i].state==EDG_WLL_JOB_DONE) ? edg_wll_done_codeToString(states[k][i].done_code) : "" );
				}
				else if ((states[k][i].jobtype == EDG_WLL_STAT_DAG) || 
					(states[k][i].jobtype == EDG_WLL_STAT_COLLECTION)) {
					printf("%s  %s .... %s %s\n", (states[k][i].jobtype==EDG_WLL_STAT_DAG)?"DAG ":"COLL",id, st, (states[k][i].state==EDG_WLL_JOB_DONE) ? edg_wll_done_codeToString(states[k][i].done_code) : "");
					for (j=0; states[k][j].state != EDG_WLL_JOB_UNDEF; j++) {
						if (states[k][j].parent_job) {
							char *par_id = glite_jobid_unparse(states[k][j].parent_job);
							
							if (!strcmp(id,par_id)) {
								char *sub_id = glite_jobid_unparse(states[k][j].jobId),
								     *sub_st = edg_wll_StatToString(states[k][j].state);
								
								printf(" `-       %s .... %s %s\n", sub_id, sub_st, (states[k][j].state==EDG_WLL_JOB_DONE) ? edg_wll_done_codeToString(states[k][j].done_code) : "");
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
	}

	printf("\nFound %d jobs\n",i);

err:
	if  (jobs) {
		for (k=0; k < no_of_runs; k++) {
			if (jobs[k])
				for (i=0; jobs[k][i]; i++)  glite_jobid_free(*jobs[i]);	
		}
		free(jobs);
	}

	if  (states) {
		for (k=0; k < no_of_runs; k++) {
			if (states[k])
				for (i=0; states[k][i].state; i++)  edg_wll_FreeStatus(states[i]);	
		}
		free(states);
	}

	for (k=0; k < no_of_runs; k++) {
		if (edg_wll_Error(p_ctx[k],&errt,&errd)) {
			fprintf(stderr,"%s: %s (%s)\n",argv[0],errt,errd);
			return 1;
		}
	}


	for (k=0; k < no_of_runs; k++) {
		edg_wll_FreeContext(p_ctx[k]);
	}

	return 0;
}

