#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/*headers*/
#include "glite/jobid/cjobid.h" 
#include "glite/lb/events.h"
#include "glite/lb/producer.h" 
/*end headers*/


static struct option opts[] = {
	{"help",		0,	NULL,	'h'},
	{"sock",		1,	NULL,	's'},
	{"jobid",		1,	NULL,	'j'},
	{"user",		1,	NULL,	'u'},
	{"seq",			1,	NULL,	'c'},
	{"name",		1,	NULL,	'n'},
	{"value",		1,	NULL,	'v'}
};

static void usage(char *me)
{
	fprintf(stderr, "usage: %s [option]\n"
			"\t-h, --help      Shows this screen.\n"
			"\t-s, --server    LB Proxy socket.\n"
			"\t-j, --jobid     ID of requested job.\n"
			"\t-u, --user      User DN.\n"
			"\t-c, --seq       Sequence code.\n"
			"\t-n, --name      Name of the tag.\n"
			"\t-v, --value     Value of the tag.\n"
			, me);
}


int main(int argc, char *argv[])
{
	char			   *server, *seq_code, *jobid_s, *user, *name, *value;
	int					opt, err = 0;


	server = seq_code = jobid_s = name = value = NULL;
	while ( (opt = getopt_long(argc, argv, "hs:j:u:c:n:v:", opts, NULL)) != EOF)
		switch (opt) {
		case 'h': usage(argv[0]); return 0;
		case 's': server = strdup(optarg); break;
		case 'j': jobid_s = strdup(optarg); break;
		case 'u': user = strdup(optarg); break;
		case 'c': seq_code = strdup(optarg); break;
		case 'n': name = strdup(optarg); break;
		case 'v': value = strdup(optarg); break;
		case '?': usage(argv[0]); return 1;
		}

	if ( !jobid_s ) { fprintf(stderr, "JobId not given\n"); return 1; }
	if ( !server ) { fprintf(stderr, "LB proxy socket not given\n"); return 1; }
	if ( !name ) { fprintf(stderr, "Tag name not given\n"); return 1; }
	if ( !value ) { fprintf(stderr, "Tag value not given\n"); return 1; }


	edg_wll_Context		ctx;
	edg_wlc_JobId		jobid = NULL;

	if ( (errno = edg_wlc_JobIdParse(jobid_s, &jobid)) ) { perror(jobid_s); return 1; }

	/*context*/
	edg_wll_InitContext(&ctx);
	
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_SOURCE, EDG_WLL_SOURCE_USER_INTERFACE);
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_HOST, server);
	//edg_wll_SetParam(ctx, EDG_WLL_PARAM_PORT, port);
	/*end context*/

	/*sequence*/
	if (edg_wll_SetLoggingJob(ctx, jobid, seq_code, EDG_WLL_SEQ_NORMAL)) {
		char 	*et,*ed;
		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"SetLoggingJob(%s,%s): %s (%s)\n",jobid_s,seq_code,et,ed);
		exit(1);
	}
	/*end sequence*/

	/*log*/
	err = edg_wll_LogEvent(ctx, //* \label{l:logevent}
			       EDG_WLL_EVENT_USERTAG, 
			       EDG_WLL_FORMAT_USERTAG,
			       name, value);
	if (err) {
	    char	*et,*ed;

	    edg_wll_Error(ctx,&et,&ed);
	    fprintf(stderr,"%s: edg_wll_LogEvent*(): %s (%s)\n",
		    argv[0],et,ed);
	    free(et); free(ed);
	}
	/*end log*/

	seq_code = edg_wll_GetSequenceCode(ctx);
	puts(seq_code);
	free(seq_code);

	edg_wll_FreeContext(ctx);

	return err;
}
