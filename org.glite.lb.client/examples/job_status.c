#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <expat.h>

#include "glite/lb/context-int.h"
#include "glite/lb/consumer.h"
#include "glite/lb/xml_conversions.h"

static void dgerr(edg_wll_Context,char *);
static void printstat(edg_wll_JobStat,int);

#define MAX_SERVERS	20

static char 	*myname;


int main(int argc,char *argv[])
{
	edg_wll_Context	ctx, sctx[MAX_SERVERS];
	int		i, result=0, nsrv=0;
	char		*servers[MAX_SERVERS];
	char 		*errstr = NULL;

	
	myname = argv[0];
	printf("\n");

	if (argc < 2 || strcmp(argv[1],"--help") == 0) {
		fprintf(stderr,"Usage: %s job_id [job_id [...]]\n",argv[0]);
		fprintf(stderr,"       %s -all\n",argv[0]);
                return 1;
        }
        else if (argc >= 2 && strcmp(argv[1],"-all") == 0) {
		edg_wll_JobStat		*statesOut;
		edg_wlc_JobId		*jobsOut;
	        edg_wll_QueryRec        jc[2];

		jobsOut = NULL;
		statesOut = NULL;
/* init context */
		if (edg_wll_InitContext(&ctx)) {
			fprintf(stderr,"%s: cannot initialize edg_wll_Context\n",myname);
			exit(1);
		}
/* retrieve job ID's */
        	memset(jc,0,sizeof jc);

	        jc[0].attr = EDG_WLL_QUERY_ATTR_OWNER;
        	jc[0].op = EDG_WLL_QUERY_OP_EQUAL;
	        jc[0].value.c = NULL;	/* is NULL, peerName filled in on server side */
		jc[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

		//if (edg_wll_QueryJobs(ctx,jc,EDG_WLL_STAT_CLASSADS | EDG_WLL_STAT_CHILDREN |  EDG_WLL_STAT_CHILDSTAT,
		result = edg_wll_QueryJobs(ctx,jc,0,&jobsOut, &statesOut);
		if (result == E2BIG) {
			int r;
			edg_wll_Error(ctx, NULL, &errstr);
			if (edg_wll_GetParam(ctx, EDG_WLL_PARAM_QUERY_RESULTS, &r)) {
				dgerr(ctx,"edg_wll_GetParam(EDG_WLL_PARAM_QUERY_RESULTS)");
				free(errstr);
				result=1; goto cleanup;
			}
			if (r != EDG_WLL_QUERYRES_LIMITED) goto late_error;
		} else if (result) {
			dgerr(ctx,"edg_wll_QueryJobs");
			result=1; goto cleanup;
		}

/* retrieve and print status of each job */
		for (i=0; statesOut[i].state; i++) 
			printstat(statesOut[i],0);

late_error:	if (result) {
			edg_wll_SetError(ctx, result, errstr);
			free(errstr);
			dgerr(ctx,"edg_wll_QueryJobs");
			result=1; 
		}

cleanup:
		if (jobsOut)
		{
			for (i=0; jobsOut[i]; i++) edg_wlc_JobIdFree(jobsOut[i]);
			free(jobsOut);
		}
		if (statesOut)
		{
			for (i=0; statesOut[i].state; i++) edg_wll_FreeStatus(&statesOut[i]);		
			free(statesOut);
		}
		edg_wll_FreeContext(ctx);
	} 
	else {
	    for (i=1; i<argc; i++) {
		int		j;
		char		*bserver;
		edg_wlc_JobId 	job;		
		edg_wll_JobStat status;

		memset(&status,0,sizeof status);

/* parse job ID */
		if (edg_wlc_JobIdParse(argv[i],&job)) {
			fprintf(stderr,"%s: %s: cannot parse jobId\n",
				myname,argv[i]);
			continue;
		}
/* determine bookkeeping server address */
                bserver = edg_wlc_JobIdGetServer(job);
                if (!bserver) {
                        fprintf(stderr,"%s: %s: cannot extract bookkeeping server address\n",
                                myname,argv[i]);
                         edg_wlc_JobIdFree(job);
                        continue;
                }
/* use context database */
                for (j=0; j<nsrv && strcmp(bserver,servers[j]); j++);
                if (j==nsrv) {
                        edg_wll_InitContext(&sctx[j]);
                        nsrv++;
                        servers[j] = bserver;
                }

		if (edg_wll_JobStatus(sctx[j], job, EDG_WLL_STAT_CLASSADS | EDG_WLL_STAT_CHILDREN |  EDG_WLL_STAT_CHILDSTAT, &status)) {
			dgerr(sctx[j],"edg_wll_JobStatus");
			result=1; goto cleanup2;
		}

/* print job status */
		printstat(status,0);

cleanup2:	
		if (job) edg_wlc_JobIdFree(job);
		if (status.state) edg_wll_FreeStatus(&status);
	    }
		for (i=0; i<nsrv; i++) edg_wll_FreeContext(sctx[i]);
	}

	return result;
}

static void
dgerr(edg_wll_Context ctx,char *where)
{
	char 	*etxt,*edsc;

	edg_wll_Error(ctx,&etxt,&edsc);
	fprintf(stderr,"%s: %s: %s",myname,where,etxt);
	if (edsc) fprintf(stderr," (%s)",edsc);
	putc('\n',stderr);
	free(etxt); free(edsc);
}

static void printstat(edg_wll_JobStat stat, int level)
{
	char		*s, *j, ind[10];
	int 		i;


	for (i=0; i < level; i++)
		ind[i]='\t';
	ind[i]='\0';
	
	s = edg_wll_StatToString(stat.state); 
/* print whole flat structure */
	printf("%sstate : %s\n", ind, s);
	printf("%sjobId : %s\n", ind, j = edg_wlc_JobIdUnparse(stat.jobId));
	printf("%sowner : %s\n", ind, stat.owner);
	printf("%sjobtype : %s\n", ind, (stat.jobtype ? "DAG" : "SIMPLE") );
	printf("%sparent_job : %s\n", ind,
			j = edg_wlc_JobIdUnparse(stat.parent_job));
	if (stat.jobtype) {;
		printf("%sseed : %s\n", ind, stat.seed);
		printf("%schildren_num : %d\n", ind, stat.children_num);
		printf("%schildren :\n", ind);
		if (stat.children) 
			for  (i=0; stat.children[i]; i++) 
				printf("%s\tchildren : %s\n", ind, stat.children[i]);
		printf("%schildren_states :\n", ind);
		if (stat.children_states)
		 	for  (i=0; stat.children_states[i].state; i++)
		 		printstat(stat.children_states[i], level+1);
		printf("%schildren_hist :\n",ind);
		if (stat.children_hist) 
			for (i=1; i<=stat.children_hist[0]; i++) 
				printf("%s%14s  %d\n", ind, edg_wll_StatToString(i-1),stat.children_hist[i]);
	}
	printf("%scondorId : %s\n", ind, stat.condorId);
	printf("%sglobusId : %s\n", ind, stat.globusId);
	printf("%slocalId : %s\n", ind, stat.localId);
	printf("%sjdl : %s\n", ind, stat.jdl);
	printf("%smatched_jdl : %s\n", ind, stat.matched_jdl);
	printf("%sdestination : %s\n", ind, stat.destination);
	printf("%snetwork server : %s\n", ind, stat.network_server);
	printf("%scondor_jdl : %s\n", ind, stat.condor_jdl);
	printf("%srsl : %s\n", ind, stat.rsl);
	printf("%sreason : %s\n", ind, stat.reason);
	printf("%slocation : %s\n", ind, stat.location);
	printf("%sce_node : %s\n", ind, stat.ce_node);
	printf("%ssubjob_failed : %d\n", ind, stat.subjob_failed);
	printf("%sdone_code : %s\n", ind, edg_wll_done_codeToString(stat.done_code));
	printf("%sexit_code : %d\n", ind, stat.exit_code);
	printf("%sresubmitted : %d\n", ind, stat.resubmitted);
	printf("%scancelling : %d\n", ind, stat.cancelling);
	printf("%scancelReason : %s\n", ind, stat.cancelReason);
	printf("%scpuTime : %d\n", ind, stat.cpuTime);
	printf("%suser_tags :\n",ind);
	if (stat.user_tags) 
		for (i=0; stat.user_tags[i].tag; i++) printf("%s%14s = \"%s\"\n", ind, 
						      stat.user_tags[i].tag,stat.user_tags[i].value);
	printf("%sstateEnterTime : %ld.%06ld\n", ind, stat.stateEnterTime.tv_sec,stat.stateEnterTime.tv_usec);
	printf("%sstateEnterTimes : \n",ind);
	if (stat.stateEnterTimes)  
                for (i=1; i<=stat.stateEnterTimes[0]; i++) 
			printf("%s%14s  %s", ind, edg_wll_StatToString(i-1), (stat.stateEnterTimes[i] == 0) ? 
			"    - not available -\n" : ctime((time_t *) &stat.stateEnterTimes[i]));
	printf("%slastUpdateTime : %ld.%06ld\n", ind, stat.lastUpdateTime.tv_sec,stat.lastUpdateTime.tv_usec);
	printf("%sexpectUpdate : %d\n", ind, stat.expectUpdate);
	printf("%sexpectFrom : %s\n", ind, stat.expectFrom);
	printf("%sacl : %s\n", ind, stat.acl);
	printf("\n");	
	
	free(j);
	free(s);
}

