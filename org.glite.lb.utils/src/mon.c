#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>

#include "glite/lb/consumer.h"

#define DEFAULT_QUERY_TIME	3600

static void usage(char *);
static int query_all(edg_wll_Context, int, struct timeval, edg_wll_JobStat **);
static void dgerr(edg_wll_Context,char *);

static char 	*myname = NULL;
static int	debug = 0, verbose = 0, seconds = DEFAULT_QUERY_TIME, lbproxy = 0;
static const char rcsid[] = "@(#)$Id$";

static struct option const long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 'V' },
        { "verbose", no_argument, 0, 'v' },
        { "debug", no_argument, 0, 'd' },
        { "time", required_argument, 0, 't' },
        { "lbproxy", required_argument, 0, 'x' },
        { NULL, 0, NULL, 0}
};

int main(int argc,char *argv[]) {
	edg_wll_Context		ctx;
	edg_wll_JobStat		*statesOut = NULL;
	struct timeval time_now;
	int state[4] = { EDG_WLL_JOB_CLEARED, EDG_WLL_JOB_ABORTED, EDG_WLL_JOB_CANCELLED, EDG_WLL_JOB_SUBMITTED };

	int i, j, result, opt;
	result = opt = 0;

	myname = argv[0];
	fprintf(stdout,"\n");
	/* get arguments */
	while ((opt = getopt_long(argc,argv,
		"h"  /* help */
		"V"  /* version */
		"v"  /* verbose */
		"d"  /* debug */
		"t:" /* time [in seconds] */
		"x", /* lbproxy */ 
		long_options, (int *) 0)) != EOF) {

		switch (opt) {
			case 'V': fprintf(stdout,"%s:\t%s\n",argv[0],rcsid); exit(0);
			case 'v': verbose = 1; break;
			case 'd': debug = 1; break;
			case 'x': lbproxy = 1; break;
			case 't': seconds = atoi(optarg); break;
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
		int min,avg,max,nJobs,nStates;
		
		avg = max = nJobs = nStates = 0;
		min = INT_MAX;
		
		fprintf(stdout,"Jobs that entered state %s in the last %d seconds: \n",status,seconds);

		if ( (result = query_all(ctx, state[j], time_now, &statesOut)) ) {
			dgerr(ctx, "edg_wll_QueryJobs");
		} else {
			if ( statesOut ) {
				for (i=0; statesOut[i].state; i++) {
					int val = statesOut[0].stateEnterTime.tv_sec - statesOut[0].stateEnterTimes[1+EDG_WLL_JOB_SUBMITTED];
					avg += val;
					if (val < min) min = val;
					if (val > max) max = val;
					
/* FIXME:
					if (statesOut[i].state) edg_wll_FreeStatus(&statesOut[i]);
*/
				}
				nJobs = i;
				free(statesOut);
			}

		}
		if (nJobs > 0) avg = avg / nJobs;
		if (min == INT_MAX) min = 0;
		fprintf(stdout,"number of jobs: %d\n",nJobs);
		if (state[j] != EDG_WLL_JOB_SUBMITTED) {
			fprintf(stdout,"minimum time spent in the system: %d seconds\n",min);
			fprintf(stdout,"average time spent in the system: %d seconds\n",avg);
			fprintf(stdout,"maximum time spent in the system: %d seconds\n",max);
		}
		fprintf(stdout,"\n\n");

		if (status) free(status);

	}
	edg_wll_FreeContext(ctx);


	return result;
}

static void
usage(char *name) {
	fprintf(stdout, "Usage: %s [-x] [-t time]\n"
			"-h, --help		display this help and exit\n"
			"-t, --time		querying time in seconds from now to the past [deault %d]\n",
			name, DEFAULT_QUERY_TIME);
}

static int
query_all(edg_wll_Context ctx, int query_status, struct timeval query_time, edg_wll_JobStat **statesOut) {
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
	jc[1].value.t.tv_sec = query_time.tv_sec - seconds;
	jc[1].value.t.tv_usec = query_time.tv_usec;
	jc[1].value2.t.tv_sec = query_time.tv_sec;
	jc[1].value2.t.tv_usec = query_time.tv_usec;
	jc[2].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	if ( (ret = edg_wll_QueryJobs(ctx, jc, 0, NULL, statesOut)) ) {
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
