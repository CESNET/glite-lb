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
#include "glite/lb/notifid.h"
#include "glite/lb/events.h"

#ifdef BUILDING_LB_CLIENT
#include "producer.h"
#else
#include "glite/lb/producer.h"
#endif

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
	edg_wll_Context		ctx;
	edg_wlc_JobId		jobid = NULL;
	char			   *server, *code, *jobid_s, *user, *name, *value;
	int					opt, err = 0;


	server = code = jobid_s = name = value = NULL;
	while ( (opt = getopt_long(argc, argv, "hs:j:u:c:n:v:", opts, NULL)) != EOF)
		switch (opt) {
		case 'h': usage(name); return 0;
		case 's': server = strdup(optarg); break;
		case 'j': jobid_s = strdup(optarg); break;
		case 'u': user = strdup(optarg); break;
		case 'c': code = strdup(optarg); break;
		case 'n': name = strdup(optarg); break;
		case 'v': value = strdup(optarg); break;
		case '?': usage(name); return 1;
		}

	if ( !jobid_s ) { fprintf(stderr, "JobId not given\n"); return 1; }
	if ( !server ) { fprintf(stderr, "LB proxy socket not given\n"); return 1; }
	if ( !name ) { fprintf(stderr, "Tag name not given\n"); return 1; }
	if ( !value ) { fprintf(stderr, "Tag value not given\n"); return 1; }

	if ( (errno = edg_wlc_JobIdParse(jobid_s, &jobid)) ) { perror(jobid_s); return 1; }

	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		return 1;
	}

	if ( !user ) {
		/*
		edg_wll_GssStatus	gss_stat;

		if ( edg_wll_gss_acquire_cred_gsi(
				ctx->p_proxy_filename ? : ctx->p_cert_filename,
				ctx->p_proxy_filename ? : ctx->p_key_filename,
				NULL, &gss_stat) ) {
			fprintf(stderr, "failed to load GSI credentials\n");
			retrun 1;
		}
		*/
	}

	edg_wll_SetParam(ctx, EDG_WLL_PARAM_SOURCE, EDG_WLL_SOURCE_USER_INTERFACE);
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_LBPROXY_STORE_SOCK, server);

	if (edg_wll_SetLoggingJobProxy(ctx, jobid, code, user, EDG_WLL_SEQ_NORMAL)) {
		char 	*et,*ed;
		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"SetLoggingJob(%s,%s): %s (%s)\n",jobid_s,code,et,ed);
		exit(1);
	}

	err = edg_wll_LogEventProxy(ctx,
				EDG_WLL_EVENT_USERTAG, EDG_WLL_FORMAT_USERTAG,
				name, value);

	if (err) {
	    char	*et,*ed;

	    edg_wll_Error(ctx,&et,&ed);
	    fprintf(stderr,"%s: edg_wll_LogEvent*(): %s (%s)\n",
		    argv[0],et,ed);
	    free(et); free(ed);
	}

	code = edg_wll_GetSequenceCode(ctx);
	puts(code);
	free(code);

	edg_wll_FreeContext(ctx);

	return err;
}
