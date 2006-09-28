#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <expat.h>

#include <pthread.h>
#include <globus_common.h>

#include "glite/lb/context.h"
#include "glite/lb/xml_conversions.h"
#include "glite/lb/consumer.h"

int use_proxy = 0;

int (*user_jobs)(edg_wll_Context, edg_wlc_JobId **, edg_wll_JobStat **);


void
usage(char *me)
{
	fprintf(stderr,"usage: %s [-h] [-x] [userid]\n", me);
}

typedef struct {
    char *owner;
    int proxy;
    char *argv_0;} thread_code_args;

void *thread_code(thread_code_args *arguments) {

	edg_wll_Context	ctx;
	char		*errt,*errd;
	edg_wlc_JobId		*jobs = NULL;
	edg_wll_JobStat		*states = NULL;
	int		i,j;

	user_jobs = edg_wll_UserJobs;

       if (arguments->proxy) {
            user_jobs = edg_wll_UserJobsProxy;
        }

	edg_wll_InitContext(&ctx);
	if ( user_jobs == edg_wll_UserJobsProxy  && arguments->owner )
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_LBPROXY_USER, arguments->owner);

	if (user_jobs(ctx,&jobs,&states)) goto err;
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

        printf("Thread %d exitting\n",(int)pthread_self());

        pthread_exit(NULL);

        printf("Thread %d out !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",(int)pthread_self());
//        return 0;
}

int main(int argc,char **argv)
{

        #define NUM_THREADS 10

        thread_code_args arguments = { NULL , 0 , NULL };
	int	i,rc,status;
        pthread_t threads[NUM_THREADS];
	pthread_attr_t attr;

	if (globus_module_activate(GLOBUS_COMMON_MODULE) != GLOBUS_SUCCESS)   {
		fputs("globus_module_activate()\n",stderr);
		return 1;
	}

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for ( i = 1; i < argc; i++ ) {
		if ( !strcmp(argv[i], "-h") || !strcmp(argv[i], "--help") ) {
			usage(argv[0]);
			exit(0);
		}
                else if ( !strcmp(argv[i], "-x") ) {
                    arguments.proxy = 1;
         	}

		arguments.owner = strdup(argv[i]);
	}

        arguments.argv_0 = argv[0];


        for (i=0;i<NUM_THREADS;i++) {
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

        for (i=0;i<NUM_THREADS;i++) {
            printf("*** Joining thread %d.\n",i);
            rc=pthread_join(threads[i],(void **)&status);
            printf("*** Thread %d joined. (status %d ret. code %d)\n",i,status,rc);

        }

        printf("Threaded example main() exitting\n");


//        exit(0);
//        pthread_exit(NULL);
}

