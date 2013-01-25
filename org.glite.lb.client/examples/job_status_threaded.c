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
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "glite/lb/context-int.h"
#include "glite/lb/connpool.h"
#ifdef BUILDING_LB_CLIENT
#include "consumer.h"
#else
#include "glite/lb/consumer.h"
#endif
#include "glite/lb/xml_conversions.h"
#include "glite/lb/jobstat.h"

static void dgerr(edg_wll_Context,char *);
static void printstat_oneline(edg_wll_JobStat,int);

static char 	*myname;

void *thread_meat(char *jobid) {
	edg_wll_Context	ctx;
	int result=0, retries;

	if ( edg_wll_InitContext(&ctx) ) {
		fprintf(stderr,"cannot initialize edg_wll_Context\n");
		exit(1);
	}

	char		*bserver;
	edg_wlc_JobId 	job;		
	edg_wll_JobStat status;

	memset(&status,0,sizeof status);
	
	if (edg_wlc_JobIdParse(jobid,&job)) {
		fprintf(stderr,"%s: %s: cannot parse jobId\n", myname,jobid); goto cleanup;
	}
	bserver = edg_wlc_JobIdGetServer(job);
	if (!bserver) {
		fprintf(stderr,"%s: %s: cannot extract bookkeeping server address\n", myname,jobid);
		edg_wlc_JobIdFree(job); goto cleanup;
	}
	else free(bserver);

	for (retries = 6; retries > 0; retries--) {
		if (edg_wll_JobStatus(ctx, job, EDG_WLL_STAT_CLASSADS | EDG_WLL_STAT_CHILDREN |  EDG_WLL_STAT_CHILDSTAT, &status)) {
			dgerr(ctx,"edg_wll_JobStatus"); result = 1; 
		} else {
			printstat_oneline(status,0);
			edg_wll_FreeStatus(&status);
			break;
		}
		sleep(3); 
	}

	if (job) edg_wlc_JobIdFree(job);

	cleanup:
	
	edg_wll_FreeContext(ctx);

	return NULL;
}

static void
usage(char *name) {
	fprintf(stderr,"Usage: %s [job_id [...]]\n", name);
}

static void
dgerr(edg_wll_Context ctx,char *where) {
	char 	*etxt,*edsc;

	edg_wll_Error(ctx,&etxt,&edsc);
	fprintf(stderr,"%s: %s: %s",myname,where,etxt);
	if (edsc) fprintf(stderr," (%s)",edsc);
	putc('\n',stderr);
	free(etxt); free(edsc);
}

static void printstat_oneline(edg_wll_JobStat stat, int level) {
	char *s, *j1;

	s = edg_wll_StatToString(stat.state); 
	j1 = edg_wlc_JobIdUnparse(stat.jobId);
	printf("%s\t%s\n", j1, s);

	free(j1); free(s);
}

int main(int argc,char *argv[]) {
	int i, rc;
	pthread_t *threads;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	void *jobid_arg;

	myname = argv[0];
	if ( argc < 2 || strcmp(argv[1],"--help") == 0 ) { usage(argv[0]); return 0; }

	threads=(pthread_t*)calloc(sizeof(pthread_t), argc+1);

	for ( i = 1; i < argc; i++ ) {
		jobid_arg = (void *)argv[i];

		rc = pthread_create(&(threads[i-1]), &attr, (void*)thread_meat, (void *)jobid_arg);
                if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	pthread_attr_destroy(&attr);

	for ( i = 1; i < argc; i++ ) {
		pthread_join(threads[i-1],NULL);
	}

	free(threads);
	edg_wll_poolFree(); // for hunting memleaks

	return 0;
}

