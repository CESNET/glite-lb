#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "glite/jobid/cjobid.h"
#include "consumer.h"


static struct option opts[] = {
	{"help",		0,	NULL,	'h'},
	{"sock",		1,	NULL,	's'},
	{"jobid",		1,	NULL,	'j'},
};

static void usage(char *me)
{
	fprintf(stderr, "usage: %s [option]\n"
			"\t-h, --help      Shows this screen.\n"
			"\t-s, --server    LB Proxy socket.\n"
			"\t-j, --jobid     ID of requested job.\n"
			, me);
}


int main(int argc, char *argv[])
{
	edg_wll_Context		ctx;
	edg_wlc_JobId		jobid = NULL;
	char			   *server, *jobid_s, *code;
	int					opt, err = 0;


	server = code = jobid_s = NULL;
	while ( (opt = getopt_long(argc, argv, "hs:j:c:n:v:", opts, NULL)) != EOF)
		switch (opt) {
		case 'h': usage(argv[0]); return 0;
		case 's': server = strdup(optarg); break;
		case 'j': jobid_s = strdup(optarg); break;
		case '?': usage(argv[0]); return 1;
		}

	if ( !jobid_s ) { fprintf(stderr, "JobId not given\n"); return 1; }
	if ( !server ) { fprintf(stderr, "LB proxy socket not given\n"); return 1; }

	if ( (errno = edg_wlc_JobIdParse(jobid_s, &jobid)) ) { perror(jobid_s); return 1; }

	edg_wll_InitContext(&ctx);

	edg_wll_SetParam(ctx, EDG_WLL_PARAM_SOURCE, EDG_WLL_SOURCE_USER_INTERFACE);
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_LBPROXY_SERVE_SOCK, server);

/*
	if (edg_wll_SetLoggingJob(ctx, jobid, code, EDG_WLL_SEQ_NORMAL)) {
		char 	*et,*ed;
		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"SetLoggingJob(%s,%s): %s (%s)\n",jobid_s,code,et,ed);
		exit(1);
	}
*/
	if ( (err = edg_wll_QuerySequenceCodeProxy(ctx, jobid, &code)) ) {
	    char	*et,*ed;

	    edg_wll_Error(ctx,&et,&ed);
	    fprintf(stderr,"%s: edg_wll_QuerySequenceCodeProxy(): %s (%s)\n",
		    argv[0],et,ed);
	    free(et); free(ed);
	}

	if ( code ) {
		puts(code);
		free(code);
	}

	edg_wll_FreeContext(ctx);

	return err;
}
