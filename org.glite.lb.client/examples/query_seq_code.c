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

#include "glite/jobid/cjobid.h"
#ifdef BUILDING_LB_CLIENT
#include "consumer.h"
#else
#include "glite/lb/consumer.h"
#endif


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

	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		return 1;
	}

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
