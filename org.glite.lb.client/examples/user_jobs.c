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
#include <getopt.h>

#include "glite/lb/context.h"
#include "glite/lb/xml_conversions.h"
#ifdef BUILDING_LB_CLIENT
#include "consumer.h"
#else
#include "glite/lb/consumer.h"
#endif

int use_proxy = 0;

static const char *get_opt_string = "hxn";

static struct option opts[] = {
	{"help",	0,	NULL,	'h'},
	{"proxy",	0,	NULL,	'x'},
	{"no-states",	0,	NULL,	'n'},
	{NULL,	0,	NULL,	0}
};

int (*user_jobs)(edg_wll_Context, edg_wlc_JobId **, edg_wll_JobStat **);


void
usage(char *me)
{
	fprintf(stderr,"usage: %s [options] [userid]\n"
		"\t-h, --help     	show this help\n"
		"\t-x, --proxy    	contact proxy\n"
		"\t-n, --no-states	do not show job states"
		"\n",
		me);
}

int main(int argc,char **argv)
{
	edg_wll_Context	ctx;
	char		*errt,*errd;
	edg_wlc_JobId		*jobs = NULL;
	edg_wll_JobStat		*states = NULL;
	int		opt,i,j;
	char 		*owner = NULL;
	int		show_states = 1;

	user_jobs = edg_wll_UserJobs;

	while ((opt = getopt_long(argc, argv, get_opt_string, opts, NULL)) != EOF) {
		switch (opt) {
		case 'h':
			usage(argv[0]);
			exit(0);
		case 'x':
			user_jobs = edg_wll_UserJobsProxy;
			break;
		case 'n':
			show_states = 0;
			break;
		default:
			if (owner || !optarg) {
				usage(argv[0]);
				exit(1);
			}
			owner = strdup(optarg);
			break;
		}
	}

	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		return 1;
	}
	if ( user_jobs == edg_wll_UserJobsProxy  && owner )
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_LBPROXY_USER, owner);

	if (user_jobs(ctx,&jobs,show_states ? &states : NULL)) goto err;
 	for (i=0; states && states[i].state != EDG_WLL_JOB_UNDEF; i++) {
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
	if (!show_states) {
		for (i = 0; jobs[i]; i++) {
			char *id = edg_wlc_JobIdUnparse(jobs[i]);
			printf("	%s\n", id);
			free(id);
		}
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

