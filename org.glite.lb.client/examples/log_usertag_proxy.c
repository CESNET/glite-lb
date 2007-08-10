#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "glite/jobid/cjobid.h"
#include "glite/lb/notifid.h"
#include "glite/lb/events.h"

#include "producer.h"

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

	edg_wll_InitContext(&ctx);

	if ( !user ) {
		/*
		edg_wll_GssStatus	gss_stat;

		if ( edg_wll_gss_acquire_cred_gsi(
				ctx->p_proxy_filename ? : ctx->p_cert_filename,
				ctx->p_proxy_filename ? : ctx->p_key_filename,
				NULL, &user_dn, &gss_stat) ) {
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
