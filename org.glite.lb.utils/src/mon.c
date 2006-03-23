#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>

#include "glite/lb/consumer.h"

static void usage(char *);
static int query_all(edg_wll_Context, int, struct timeval, edg_wll_JobStat **, edg_wlc_JobId **);
static void dgerr(edg_wll_Context,char *);

static char 	*myname = NULL;
static int	debug = 0, verbose = 0, lbproxy =0;
static const char rcsid[] = "@(#)$Id$";

static struct option const long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 'V' },
        { "verbose", no_argument, 0, 'v' },
        { "debug", no_argument, 0, 'd' },
        { "lbproxy", required_argument, 0, 'x' },
        { NULL, 0, NULL, 0}
};

int main(int argc,char *argv[]) {
	edg_wll_Context		ctx;
	edg_wll_JobStat		*statesOut = NULL;
	edg_wlc_JobId		*jobsOut = NULL;
	struct timeval time_now;
	int state[3] = { EDG_WLL_JOB_CLEARED, EDG_WLL_JOB_ABORTED, EDG_WLL_JOB_CANCELLED };

	int i, j, result, opt, nJobs;
	result = opt = 0;

	myname = argv[0];
	fprintf(stdout,"\n");
	/* get arguments */
	while ((opt = getopt_long(argc,argv,
		"h"  /* help */
		"V"  /* version */
		"v"  /* verbose */
		"d"  /* debug */
		"x", /* lbproxy */
		long_options, (int *) 0)) != EOF) {

		switch (opt) {
			case 'V': fprintf(stdout,"%s:\t%s\n",argv[0],rcsid); exit(0);
			case 'v': verbose = 1; break;
			case 'd': debug = 1; break;
			case 'x': lbproxy = 1; break;
			case 'h':
			default:
				usage(argv[0]); exit(0);
		}
	}
	gettimeofday(&time_now,0);

	if ( edg_wll_InitContext(&ctx) ) {
		fprintf(stderr,"%s: cannot initialize edg_wll_Context\n ",myname);
		exit(1);
	}

	for ( j = 0; j < sizeof(state)/sizeof(state[0]); j++) {
		char *status = edg_wll_StatToString(state[j]);
		nJobs = 0;
		
		fprintf(stdout,"Jobs that entered state %s in the last hour: \n",status);

		if ( (result = query_all(ctx, state[j], time_now, &statesOut, &jobsOut)) ) {
			dgerr(ctx, "edg_wll_QueryJobs");
		} else {

			if ( jobsOut ) {
				for (i=0; jobsOut[i]; i++) edg_wlc_JobIdFree(jobsOut[i]); {
					nJobs++;
					free(jobsOut);
				}
			}
			if ( statesOut ) {
				for (i=0; statesOut[i].state; i++) edg_wll_FreeStatus(&statesOut[i]);
					free(statesOut);
				}

		}
		fprintf(stdout,"number of jobs: %d\n",nJobs);
		fprintf(stdout,"minimum time spent in the system: %d\n",nJobs);
		fprintf(stdout,"average time spent in the system: %d\n",nJobs);
		fprintf(stdout,"maximum time spent in the system: %d\n",nJobs);
		fprintf(stdout,"\n\n");

		if (status) free(status);

	}
	edg_wll_FreeContext(ctx);


	return result;
}

static void
usage(char *name) {
	fprintf(stderr,"Usage: %s [-x]\n", name);
}

static int
query_all(edg_wll_Context ctx, int query_status, struct timeval query_time, edg_wll_JobStat **statesOut, edg_wlc_JobId **jobsOut) {
	edg_wll_QueryRec        jc[3];
	int			ret;

	memset(jc, 0, sizeof jc);

	/* jobs in the state 'query_status' within last hour */
	jc[0].attr = EDG_WLL_QUERY_ATTR_STATUS;
	jc[0].op = EDG_WLL_QUERY_OP_EQUAL;
	jc[0].value.i = query_status;
	jc[1].attr = EDG_WLL_QUERY_ATTR_TIME;
	jc[1].attr_id.state = query_status;
	jc[1].op = EDG_WLL_QUERY_OP_WITHIN;
	jc[1].value.t.tv_sec = query_time.tv_sec - 3600;
	jc[1].value.t.tv_usec = query_time.tv_usec;
	jc[1].value2.t.tv_sec = query_time.tv_sec;
	jc[1].value2.t.tv_usec = query_time.tv_usec;
	jc[2].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	if ( (ret = edg_wll_QueryJobs(ctx, jc, 0, jobsOut, statesOut)) ) {
		if ( ret == E2BIG ) {
			int r;
			if ( edg_wll_GetParam(ctx, EDG_WLL_PARAM_QUERY_RESULTS, &r) ) return ret;
			if ( r != EDG_WLL_QUERYRES_LIMITED ) return ret;

			fprintf(stderr," edg_wll_QueryJobs() Warning: only limited result returned!\n");
			return 0;
		} else return ret;
	}

	return ret;
}

static void
dgerr(edg_wll_Context ctx,char *where) {
	char 	*etxt,*edsc;

	edg_wll_Error(ctx,&etxt,&edsc);
	fprintf(stderr,"%s: %s: %s",myname,where,etxt);
	if (edsc) fprintf(stderr," (%s)",edsc);
	putc('\n',stderr);
	if(etxt) free(etxt); 
	if(edsc) free(edsc);
}
