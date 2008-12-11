#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/*headers*/
#include "glite/jobid/cjobid.h" 
#include "glite/lb/events.h"
#include "glite/lb/consumer.h" 
/*end headers*/


static struct option opts[] = {
	{"help",		0,	NULL,	'h'},
	{"sock",		1,	NULL,	's'},
	{"jobid",		1,	NULL,	'j'},
	{"user",		1,	NULL,	'u'},
};

static void usage(char *me)
{
	fprintf(stderr, "usage: %s [option]\n"
			"\t-h, --help      Shows this screen.\n"
			"\t-s, --server    LB Proxy socket.\n"
			"\t-j, --jobid     ID of requested job.\n"
			"\t-u, --user      User DN.\n"
			, me);
}


int main(int argc, char *argv[])
{
	char			   *server, *jobid_s, *user;
	int					opt, err = 0;

	server = code = jobid_s = name = value = NULL;
	while ( (opt = getopt_long(argc, argv, "hs:j:u:c:n:v:", opts, NULL)) != EOF)
		switch (opt) {
		case 'h': usage(name); return 0;
		case 's': server = strdup(optarg); break;
		case 'j': jobid_s = strdup(optarg); break;
		case 'u': user = strdup(optarg); break;
		case '?': usage(name); return 1;
		}

	if ( !jobid_s ) { fprintf(stderr, "JobId not given\n"); return 1; }
	if ( !server ) { fprintf(stderr, "LB proxy socket not given\n"); return 1; }

	/*variables*/
	edg_wll_Context		ctx;
	edg_wll_QueryRec	*jc[4];
	edg_wll_JobStat		*statesOut = NULL;
	edg_wlc_JobId		*jobsOut = NULL;
	/*end variables*/

	if ( (errno = edg_wlc_JobIdParse(jobid_s, &jobid)) ) { perror(jobid_s); return 1; }

	/*context*/
	edg_wll_InitContext(&ctx);
	
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER, server);
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER_PORT, port);
	/*end context*/

	/*queryrec*/
	jc[0] = (edg_wll_QueryRec *) malloc(2*sizeof(edg_wll_QueryRec));
	jc[0][0].attr = EDG_WLL_QUERY_ATTR_OWNER;
	jc[0][0].op = EDG_WLL_QUERY_OP_EQUAL;
	jc[0][0].value.c = NULL;
	jc[0][1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	jc[1] = (edg_wll_QueryRec *) malloc(2*sizeof(edg_wll_QueryRec));
	jc[1][0].attr = EDG_WLL_QUERY_ATTR_STATUS;
	jc[1][0].op = EDG_WLL_QUERY_OP_EQUAL;
	jc[1][0].value.i = EDG_WLL_JOB_RUNNING;
	jc[1][1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	jc[2] = (edg_wll_QueryRec *) malloc(3*sizeof(edg_wll_QueryRec));
	jc[2][0].attr = EDG_WLL_QUERY_ATTR_DESTINATION;
	jc[2][0].op = EDG_WLL_QUERY_OP_EQUAL;
	jc[2][0].value.c = "XXX";
	jc[2][1].attr = EDG_WLL_QUERY_ATTR_DESTINATION;
	jc[2][1].op = EDG_WLL_QUERY_OP_EQUAL;
	jc[2][1].value.c = "YYY";
	jc[2][2].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	jc[3] = NULL;
	/*end queryrec*/

	/*query*/
	err = edg_wll_QueryJobsExt(ctx, (const edg_wll_QueryRec **)jc, 
				   0, &jobsOut, &statesOut); 
	/*end query*/

	if ( err == E2BIG ) {
		fprintf(stderr,"Warning: only limited result returned!\n");
		return 0;
	} elseif (err) {
		char	*et,*ed;
		
		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"%s: edg_wll_QueryJobs(): %s (%s)\n",argv[0],et,ed);

		free(et); free(ed);
	}

	/*printstates*/
	for (i = 0; statesOut[i].state; i++ ) {
		printf("jobId : %s\n", edg_wlc_JobIdUnparse(statesOut[i].jobId));
		printf("state : %s\n\n", edg_wll_StatToString(statesOut[i].state));
	}
	/*end printstates*/

	if ( jobsOut ) {
		for (i=0; jobsOut[i]; i++) edg_wlc_JobIdFree(jobsOut[i]);
		free(jobsOut);
	}
	if ( statesOut ) {
		for (i=0; statesOut[i].state; i++) edg_wll_FreeStatus(&statesOut[i]);
		free(statesOut);
	}
	free(jc[0]); free(jc[1]); free(jc[2]);

	edg_wll_FreeContext(ctx);

	return err;
}
