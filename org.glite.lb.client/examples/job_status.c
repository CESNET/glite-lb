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

static void usage(char *);
static int query_all(edg_wll_Context, edg_wll_JobStat **, edg_wlc_JobId **);

int main(int argc,char *argv[])
{
	edg_wll_Context	sctx[MAX_SERVERS];
	char		*servers[MAX_SERVERS];
	int		i, result=0, nsrv=0, wasaflag, histflags = 0;

	
	myname = argv[0];
	printf("\n");

	if ( argc < 2 || strcmp(argv[1],"--help") == 0 ) { usage(argv[0]); return 0; }

	if ( edg_wll_InitContext(&sctx[0]) ) {
		fprintf(stderr,"cannot initialize edg_wll_Context\n");
		exit(1);
	}

	if ( !strcmp(argv[1], "-all" ) ) {
		edg_wll_JobStat		*statesOut;
		edg_wlc_JobId		*jobsOut;

		jobsOut = NULL;
		statesOut = NULL;
		if ( (result = query_all(sctx[0], &statesOut, &jobsOut)) ) dgerr(sctx[0], "edg_wll_QueryJobs");
		else for ( i = 0; statesOut[i].state; i++ ) printstat(statesOut[i],0);

		if ( jobsOut ) {
			for (i=0; jobsOut[i]; i++) edg_wlc_JobIdFree(jobsOut[i]);
			free(jobsOut);
		}
		if ( statesOut ) {
			for (i=0; statesOut[i].state; i++) edg_wll_FreeStatus(&statesOut[i]);
			free(statesOut);
		}
		edg_wll_FreeContext(sctx[0]);

		return result;
	} 

	if ( !strcmp(argv[1], "-x") ) {
		edg_wlc_JobId 	job;		
		edg_wll_JobStat status;

		if ( argc < 3 ) { usage(argv[0]); return 1; }
		if ( edg_wll_InitContext(&sctx[0]) ) {
			fprintf(stderr,"%s: cannot initialize edg_wll_Context\n",myname);
			exit(1);
		}
		edg_wll_SetParam(sctx[0], EDG_WLL_PARAM_LBPROXY_SERVE_SOCK, argv[2]);
		for ( i = 3; i < argc; i++ ) {
			memset(&status, 0, sizeof status);
			if (edg_wlc_JobIdParse(argv[i],&job)) {
				fprintf(stderr,"%s: %s: cannot parse jobId\n", myname, argv[i]);
				continue;
			}
			if ( edg_wll_JobStatusProxy(sctx[0], job, EDG_WLL_STAT_CLASSADS | EDG_WLL_STAT_CHILDREN |  EDG_WLL_STAT_CHILDSTAT, &status)) {
				dgerr(sctx[0], "edg_wll_JobStatusProxy"); result = 1;
			} else printstat(status, 0);

			if ( job ) edg_wlc_JobIdFree(job);
			if ( status.state ) edg_wll_FreeStatus(&status);
		}
		edg_wll_FreeContext(sctx[0]);

		return result;
	}


	for ( i = 1; i < argc; i++ ) {
		int		j;
		char		*bserver;
		edg_wlc_JobId 	job;		
		edg_wll_JobStat status;

		memset(&status,0,sizeof status);

		wasaflag = 0; // Indicates that the argument was a flag

		if ( !strcmp(argv[i], "-fullhist") ) { 
			wasaflag = 1;
			histflags = EDG_WLL_STAT_CHILDHIST_THOROUGH;
		}

		if ( !strcmp(argv[i], "-fasthist") ) { 
			wasaflag = 1;
                        histflags = EDG_WLL_STAT_CHILDHIST_FAST;
		}

		if (!wasaflag) {
			if (edg_wlc_JobIdParse(argv[i],&job)) {
				fprintf(stderr,"%s: %s: cannot parse jobId\n", myname,argv[i]);
				continue;
			}
        	        bserver = edg_wlc_JobIdGetServer(job);
                	if (!bserver) {
                        	fprintf(stderr,"%s: %s: cannot extract bookkeeping server address\n", myname,argv[i]);
				edg_wlc_JobIdFree(job);
        	                continue;
                	}
	                for ( j = 0; j < nsrv && strcmp(bserver, servers[j]); j++ );
        	        if ( j == nsrv ) {
                	        if ( i > 0 ) edg_wll_InitContext(&sctx[j]);
                        	nsrv++;
	                        servers[j] = bserver;
        	        }

			if (edg_wll_JobStatus(sctx[j], job, EDG_WLL_STAT_CLASSADS | EDG_WLL_STAT_CHILDREN |  EDG_WLL_STAT_CHILDSTAT | histflags, &status)) {
				dgerr(sctx[j],"edg_wll_JobStatus"); result = 1; 
			} else printstat(status,0);

			if (job) edg_wlc_JobIdFree(job);
			if (status.state) edg_wll_FreeStatus(&status);
		}
	}
	for ( i = 0; i < nsrv; i++ ) edg_wll_FreeContext(sctx[i]);

	return result;
}

static void
usage(char *name)
{
	fprintf(stderr,"Usage: %s job_id [-x lb_proxy_serve_sock] [-fasthist|-fullhist] [job_id [...]]\n", name);
	fprintf(stderr,"       %s -all\n", name);
}

static int
query_all(edg_wll_Context ctx, edg_wll_JobStat **statesOut, edg_wlc_JobId **jobsOut)
{
	edg_wll_QueryRec        jc[2];
	int			ret;

	memset(jc, 0, sizeof jc);
	jc[0].attr = EDG_WLL_QUERY_ATTR_OWNER;
       	jc[0].op = EDG_WLL_QUERY_OP_EQUAL;
	jc[0].value.c = NULL;	/* is NULL, peerName filled in on server side */
	jc[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	if ( (ret = edg_wll_QueryJobs(ctx, jc, 0, jobsOut, statesOut)) ) {
		if ( ret == E2BIG ) {
			int r;
			if ( edg_wll_GetParam(ctx, EDG_WLL_PARAM_QUERY_RESULTS, &r) ) return ret;
			if ( r != EDG_WLL_QUERYRES_LIMITED ) return ret;

			printf("Warning: only limited result returned!\n");
			return 0;
		} else return ret;
	}

	return ret;
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
	switch (stat.jobtype) {
		case EDG_WLL_STAT_SIMPLE:
			printf("%sjobtype : SIMPLE\n", ind);
			break;
		case EDG_WLL_STAT_DAG:
			printf("%sjobtype : DAG\n", ind);
                        break;
		case EDG_WLL_STAT_COLLECTION:
			printf("%sjobtype : COLLECTION\n", ind);
                        break;
		default:
			break;
	}
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
                for (i=1; i<=stat.stateEnterTimes[0]; i++) {
			time_t	st = stat.stateEnterTimes[i];

			printf("%s%14s  %s", ind, edg_wll_StatToString(i-1), st == 0 ? 
			"    - not available -\n" : ctime(&st));
		}
	printf("%slastUpdateTime : %ld.%06ld\n", ind, stat.lastUpdateTime.tv_sec,stat.lastUpdateTime.tv_usec);
	printf("%sexpectUpdate : %d\n", ind, stat.expectUpdate);
	printf("%sexpectFrom : %s\n", ind, stat.expectFrom);
	printf("%sacl : %s\n", ind, stat.acl);
	printf("%spayload_running: %d\n", ind, stat.payload_running);
	if (stat.possible_destinations) {
		printf("%spossible_destinations : \n", ind);
		for (i=0; stat.possible_destinations[i]; i++) 
			printf("%s\t%s \n", ind, stat.possible_destinations[i]);
	}
	if (stat.possible_ce_nodes) {
		printf("%spossible_ce_nodes : \n", ind);
		for (i=0; stat.possible_ce_nodes[i]; i++) 
			printf("%s\t%s \n", ind, stat.possible_ce_nodes[i]);
	}

	printf("\n");	
	
	free(j);
	free(s);
}

