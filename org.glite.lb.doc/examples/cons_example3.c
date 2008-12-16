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
			"\t-s, --server    LB server.\n"
			"\t-p, --port      LB server port.\n"
			"\t-j, --jobid     ID of requested job.\n"
			"\t-u, --user      User DN.\n"
			, me);
}


int main(int argc, char *argv[])
{
	char			   *server, *jobid_s, *user;
	int					opt, err = 0;
	edg_wlc_JobId                   jobid = NULL;
	long 				i;
	int port = 0;


	server = jobid_s = NULL;
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

	/*variables*/
	edg_wll_Context     ctx;
	edg_wll_Event      *eventsOut;
	edg_wll_QueryRec    jc[2];
	edg_wll_QueryRec    ec[2];
	/*end variables*/

	if ( (errno = edg_wlc_JobIdParse(jobid_s, &jobid)) ) { perror(jobid_s); return 1; }

	/*context*/
	edg_wll_InitContext(&ctx);
	
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER, server);
	if (port) edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER_PORT, port);
	/*end context*/

	/*queryrec*/
	jc[0].attr = EDG_WLL_QUERY_ATTR_USERTAG;
	jc[0].op = EDG_WLL_QUERY_OP_EQUAL;
	jc[0].attr_id.tag = "color";
	jc[0].value.c = "red";
	jc[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;
	ec[0].attr = EDG_WLL_QUERY_ATTR_USERTAG;
	ec[0].op = EDG_WLL_QUERY_OP_EQUAL;
	ec[0].attr_id.tag = "color";
	ec[0].value.c = "green";
	ec[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;
	/*end queryrec*/

	/*query*/
	err = edg_wll_QueryEvents(ctx, jc, ec, &eventsOut); 
	/*end query*/

	if ( err == E2BIG ) {
		fprintf(stderr,"Warning: only limited result returned!\n");
		return 0;
	} else if (err) {
		char	*et,*ed;
		
		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"%s: edg_wll_QueryEvents(): %s (%s)\n",argv[0],et,ed);

		free(et); free(ed);
	}

	if ( err == ENOENT ) return err;

	/*printevents*/
	for (i = 0; eventsOut && (eventsOut[i].type); i++ ) {
		//printf("jobId : %s\n", edg_wlc_JobIdUnparse(eventsOut[i].jobId));
		printf("event : %s\n\n", edg_wll_EventToString(eventsOut[i].type));
	}
	/*end printevents*/

	if ( eventsOut ) {
		for (i=0; &(eventsOut[i]); i++) edg_wll_FreeEvent(&(eventsOut[i]));
		free(eventsOut);
	}

	edg_wll_FreeContext(ctx);

	return err;
}
