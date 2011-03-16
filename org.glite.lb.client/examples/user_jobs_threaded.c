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
#include <unistd.h>
#include <getopt.h>

#include <pthread.h>

#include "glite/lb/context.h"
#include "glite/lb/xml_conversions.h"
#include "glite/security/glite_gss.h"
#include "consumer.h"

int use_proxy = 0;

int (*user_jobs)(edg_wll_Context, edg_wlc_JobId **, edg_wll_JobStat **);



static const char *get_opt_string = "hxj:t:p:r:s:w:";

static struct option opts[] = {
	{"help",	0, NULL,	'h'},
	{"proxy",       0, NULL,	'x'},
	{"owner",	1, NULL,	'o'},
	{"num-threads",	1, NULL,	't'},
	{"port_range",	1, NULL,	'p'},
	{"repeat",	1, NULL,	'r'},
	{"rand-start",	1, NULL,	's'},
//	{"rand-work",	1, NULL,	'w'},
	{NULL,		0, NULL,	0}
};



static void usage(char *me) 
{
	fprintf(stderr,"usage: %s [option]\n"
		"\t-h, --help\t show this help\n"
		"\t-x, --proxy\t contact proxy (not implemented yet)\n"
		"\t-o, --owner DN\t show jobs of user with this DN\n"
		"\t-t, --num-threads N\t number for threads to create\n"
		"\t-p, --port-range N\t connect to server:port, server:port+10, \n"
		"\t\t\t ... server:port+N*10 bkservers\n"
		"\t-r, --repeat N\t repeat query in each slave N-times \n"
		"\t-s, --rand-start N\t start threads in random interval <0,N> sec\n"
//		"\t-w, --rand-work N\t simulate random server respose time <0,N> sec\n"
		"\n"
	,me);
}


typedef struct {
    char *owner;
    int proxy;
    int rand_start;
    int port_range;
    int repeat;
    char *argv_0;} thread_code_args;

void *thread_code(thread_code_args *arguments) {

	edg_wll_Context	ctx;
	char		*errt,*errd;
	edg_wlc_JobId		*jobs = NULL;
	edg_wll_JobStat		*states = NULL;
	int		i,j,k,port;
	long		sl;


	sl = (unsigned long) ((double) random()/ RAND_MAX * arguments->rand_start * 1000000);
	printf("Thread [%lu] - sleeping for %ld us\n",(unsigned long)pthread_self(),sl);
	usleep( sl );

	user_jobs = edg_wll_UserJobs;

	if (arguments->proxy) {
            user_jobs = edg_wll_UserJobsProxy;
        }

	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
                pthread_exit(NULL);
	}
	if ( user_jobs == edg_wll_UserJobsProxy  && arguments->owner )
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_LBPROXY_USER, arguments->owner);

	edg_wll_GetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER_PORT, &port);
	// pthread_self tend to return even number, so /7 makes some odd numbers...
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER_PORT, 
		port + ((long) pthread_self()/7 % arguments->port_range)*10 );

	for (k=0; k<arguments->repeat; k++) {
		if (user_jobs(ctx,&jobs,&states)) goto err;
		for (i=0; states[i].state != EDG_WLL_JOB_UNDEF; i++) {	
			char *id = edg_wlc_JobIdUnparse(states[i].jobId),
			     *st = edg_wll_StatToString(states[i].state);
			
			if (!states[i].parent_job) {
				if (states[i].jobtype == EDG_WLL_STAT_SIMPLE) { 
					printf("      %s .... %s %s\n", id, st, 
						(states[i].state==EDG_WLL_JOB_DONE) ? 
						edg_wll_done_codeToString(states[i].done_code) : "" );
				}
				else if (states[i].jobtype == EDG_WLL_STAT_DAG) {
					printf("DAG   %s .... %s %s\n", id, st, 
						(states[i].state==EDG_WLL_JOB_DONE) ?
						edg_wll_done_codeToString(states[i].done_code) : "");
					for (j=0; states[j].state != EDG_WLL_JOB_UNDEF; j++) {
						if (states[j].parent_job) {
							char *par_id = edg_wlc_JobIdUnparse(states[j].parent_job);
							
							if (!strcmp(id,par_id)) {
								char *sub_id = edg_wlc_JobIdUnparse(states[j].jobId),
								     *sub_st = edg_wll_StatToString(states[j].state);
								
								printf(" `-       %s .... %s %s\n", sub_id, sub_st, 
									(states[j].state==EDG_WLL_JOB_DONE) ? 
									edg_wll_done_codeToString(states[j].done_code) : "");
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
	}
err:
	free(arguments->owner);
	if  (jobs) {
		for (i=0; jobs[i]; i++)  edg_wlc_JobIdFree(jobs[i]);	
		free(jobs);
	}

	if  (states) {
		for (i=0; states[i].state; i++)  edg_wll_FreeStatus(&states[i]);	
		free(states);
	}

	if (edg_wll_Error(ctx,&errt,&errd)) {
		fprintf(stderr,"%s: %s (%s)\n",arguments->argv_0,errt,errd);
		edg_wll_FreeContext(ctx);
                pthread_exit(NULL);
	}

	edg_wll_FreeContext(ctx);

//        printf("Thread %d exitting\n",(int)pthread_self());

        pthread_exit(NULL);

}

int main(int argc,char **argv)
{
        thread_code_args arguments = { NULL, 0, 0, 1, 1,NULL };
	int	i,rc,status,opt;
	int	thr_num = 10;


        while ((opt = getopt_long(argc,argv,get_opt_string,opts,NULL)) != EOF) switch (opt) {
                case 'x': arguments.proxy = 1; break;
                case 'o': arguments.owner = optarg; break;
                case 't': thr_num = atoi(optarg); break;
                case 'p': arguments.port_range = atoi(optarg); break;
                case 'r': arguments.repeat = atoi(optarg); break;
                case 's': arguments.rand_start = atoi(optarg); break;
                default : usage(argv[0]); exit(0); break;
        }

        arguments.argv_0 = argv[0];

	/* threads && Globus */
	if (edg_wll_gss_initialize()) {
		printf("can't initialize GSS\n");
		return 1;
	}

	/* Do a thready work */
	{
        	pthread_t threads[thr_num];
		pthread_attr_t attr;

        	pthread_attr_init(&attr);
	        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
		for (i=0;i<thr_num;i++) {
		    printf("Running thread %d\n",i);
		    rc = pthread_create(&threads[i], &attr, (void*)thread_code, &arguments );
		    if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		    }
		}



		//        thread_code(&arguments);

		//        pthread_exit(NULL)

		pthread_attr_destroy(&attr);

		for (i=0;i<thr_num;i++) {
		//            printf("*** Joining thread %d.\n",i);
		    rc=pthread_join(threads[i],(void **)&status);
		//            printf("*** Thread %d joined. (status %d ret. code %d)\n",i,status,rc);

		}
	}

        printf("Threaded example main() exitting\n");


//        exit(0);
//        pthread_exit(NULL);
	return 0;
}

