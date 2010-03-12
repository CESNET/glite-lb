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
	{"server",		1,	NULL,	's'},
	{"port",		1,	NULL,	'p'},
	{"jobid",		1,	NULL,	'j'},
	{"user",		1,	NULL,	'u'},
};

static void usage(char *me)
{
	fprintf(stderr, "usage: %s [option]\n"
			"\t-h, --help      Shows this screen.\n"
			"\t-s, --server    Server address.\n"
			"\t-p, --port      Query server port.\n"
			"\t-j, --jobid     ID of requested job.\n"
			"\t-u, --user      User DN.\n"
			, me);
}


int main(int argc, char *argv[])
{
	char			   *server, *jobid_s, *user;
	int					opt, err = 0;
	edg_wlc_JobId           	jobid = NULL;
	long				i;
	int port = 0;

	server = jobid_s = user = NULL;
	while ( (opt = getopt_long(argc, argv, "hs:p:j:u:c:n:v:", opts, NULL)) != EOF)
		switch (opt) {
		case 'h': usage(argv[0]); return 0;
		case 's': server = strdup(optarg); break;
		case 'p': port = atoi(optarg); break;
		case 'j': jobid_s = strdup(optarg); break;
		case 'u': user = strdup(optarg); break;
		case '?': usage(argv[0]); return 1;
		}

	if ( !jobid_s ) { fprintf(stderr, "JobId not given\n"); return 1; }
	//if ( !server ) { fprintf(stderr, "LB proxy socket not given\n"); return 1; }

	/*variables*/
	edg_wll_Context		ctx;
	edg_wll_QueryRec	jc[4];
	edg_wll_JobStat		*statesOut = NULL;
	edg_wlc_JobId		*jobsOut = NULL;
	/*end variables*/

	if ( (errno = edg_wlc_JobIdParse(jobid_s, &jobid)) ) { perror(jobid_s); return 1; }

	/*context*/
	edg_wll_InitContext(&ctx);
	
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER, server);
	if (port) edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER_PORT, port);
	/*end context*/

	/*queryrec*/
	jc[0].attr = EDG_WLL_QUERY_ATTR_OWNER;
	jc[0].op = EDG_WLL_QUERY_OP_EQUAL;
	jc[0].value.c = NULL;
	jc[1].attr = EDG_WLL_QUERY_ATTR_STATUS;
	jc[1].op = EDG_WLL_QUERY_OP_EQUAL;
	jc[1].value.i = EDG_WLL_JOB_RUNNING;
	jc[2].attr = EDG_WLL_QUERY_ATTR_DESTINATION;
	jc[2].op = EDG_WLL_QUERY_OP_EQUAL;
	jc[2].value.c = "XYZ";
	jc[3].attr = EDG_WLL_QUERY_ATTR_UNDEF;
	/*end queryrec*/

	/*query*/
	err = edg_wll_QueryJobs(ctx, jc, 0, &jobsOut, &statesOut); //* \label{l:queryjobs}
	if ( err == E2BIG ) {
		fprintf(stderr,"Warning: only limited result returned!\n");
		return 0;
	} else if (err) {
		char	*et,*ed;
		
		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"%s: edg_wll_QueryJobs(): %s (%s)\n",argv[0],et,ed);

		free(et); free(ed);
	}
	/*end query*/

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

	edg_wll_FreeContext(ctx);

	return err;
}
