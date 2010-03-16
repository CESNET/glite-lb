#ident "$Header$"
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



#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <globus_common.h>

#define CLIENT_SBIN_PROG

#include "glite/lb/consumer.h"
#include "glite/lb/context-int.h"
#include "glite/lb/load.h"
#include "glite/lb/ulm_parse.h"
#include "glite/lb/xml_parse.h"
#include "glite/lb/mini_http.h"

#define dprintf(x) { if (debug) printf x; }

static const char rcsid[] = "@(#)$Id$";

static int debug=0;

static void printerr(edg_wll_Context ctx);

static struct option opts[] = {
	{ "file",		required_argument, NULL, 'f'},
	{ "help",	 	no_argument, NULL, 'h' },
	{ "version", 		no_argument, NULL, 'v' },
	{ "debug", 		no_argument, NULL, 'd' },
	{ "server",	 	required_argument, NULL, 'm' },
	{ NULL,			no_argument, NULL,  0 }
};

static void usage(char *me)
{
	fprintf(stderr,"usage: %s [option]\n"
		"	-m, --server                L&B server machine name\n"
		"	-f, --file filename         file with dumped data to be loaded\n"
		"	-h, --help                  display this help\n"
		"	-v, --version               display version\n"
		"	-d, --debug                 diagnostic output\n",
		me);
}

int main(int argc,char *argv[])
{
	edg_wll_LoadRequest *request;
	edg_wll_LoadResult *result;
	char *server = NULL;
	char date[ULM_DATE_STRING_LENGTH+1];

	char *me;
	int opt;
	edg_wll_Context	ctx;

	/* initialize request to server defaults */
	request = (edg_wll_LoadRequest *) calloc(1,sizeof(edg_wll_LoadRequest));
	request->server_file = NULL;

	/* initialize result */
	result = (edg_wll_LoadResult *) calloc(1,sizeof(edg_wll_LoadResult));

	me = strrchr(argv[0],'/');
	if (me) me++; else me=argv[0];

	/* get arguments */
	while ((opt = getopt_long(argc,argv,"f:t:m:dvh",opts,NULL)) != EOF) {

		switch (opt) {

		case 'f': request->server_file = optarg; break;
		case 'm': server = optarg; break;
		case 'd': debug = 1; break;
		case 'v': fprintf(stdout,"%s:\t%s\n",me,rcsid); exit(0);
		case 'h':
		case '?': usage(me); return 1;
		}
	}

	/* Initialize Globus common module */
	dprintf(("Initializing Globus common module..."));
	if (globus_module_activate(GLOBUS_COMMON_MODULE) != GLOBUS_SUCCESS) {
		dprintf(("no.\n"));
		fprintf(stderr,"Unable to initialize Globus common module\n");
	} else {
		dprintf(("yes.\n"));
	}

	/* initialize context */
	edg_wll_InitContext(&ctx);
	if ( server )
	{
		char *p = strchr(server, ':');
		if ( p )
		{
			edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER_PORT, atoi(p+1));
			*p = 0;
		}
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER, server);
	}

	/* check request */
	if (debug) {
		printf("Load request:\n");
		if (request->server_file) {
			printf("- server_file: %s.\n",request->server_file);
		} else {
			printf("- server_file: not specified.\n");
		}
	}

	/* that is the LoadEvents */
	dprintf(("Running the edg_wll_LoadEvents...\n"));
	if (edg_wll_LoadEvents(ctx, request, result) != 0) {
		fprintf(stderr,"Error running the edg_wll_LoadEvents().\n");
		printerr(ctx);
		if ( !result->server_file )
			goto main_end;
	}

	/* examine the result */
	dprintf(("Examining the result of edg_wll_LoadEvents...\n"));
	printf("Load result:\n");
	if (result->server_file)
		printf("- Unloaded events were stored into the server file '%s'.\n", result->server_file);
	if (edg_wll_ULMTimevalToDate(result->from,0,date) != 0) {
		fprintf(stderr,"Error parsing 'from' argument.\n");
		goto main_end;
	}
	printf("- from: %ld (i.e. %s).\n",result->from,date);
	if (edg_wll_ULMTimevalToDate(result->to,0,date) != 0) {
		fprintf(stderr,"Error parsing 'to' argument.\n");
		goto main_end;
	}
	printf("- to: %ld (i.e. %s).\n",result->to,date);

main_end:
	dprintf(("End.\n"));
	if (request) free(request);
	if (result)
	{
		if (result->server_file)
			free(result->server_file);
		free(result);
	}
	edg_wll_FreeContext(ctx);
	return 0;
}


static void printerr(edg_wll_Context ctx) 
{
	char	*errt,*errd;

	edg_wll_Error(ctx,&errt,&errd);
	fprintf(stderr,"%s (%s)\n",errt,errd);
}


static const char* const request_headers[] = {
	"Cache-Control: no-cache",
	"Accept: application/x-dglb",
	"User-Agent: edg_wll_Api/" PROTO_VERSION "/" COMP_PROTO,
	"Content-Type: application/x-dglb",
	NULL
};

int edg_wll_LoadEvents(
		edg_wll_Context ctx,
		const edg_wll_LoadRequest *request,
		edg_wll_LoadResult *result)
{
	int	error;
	char	*send_mess,
		*response = NULL,
		*recv_mess = NULL;

	edg_wll_LoadRequestToXML(ctx, request, &send_mess);

	ctx->p_tmp_timeout = ctx->p_query_timeout;
	if (ctx->p_tmp_timeout.tv_sec < 600) ctx->p_tmp_timeout.tv_sec = 600;

	if (set_server_name_and_port(ctx, NULL))
		goto edg_wll_loadevents_end;

	error = edg_wll_http_send_recv(ctx,
			"POST /loadRequest HTTP/1.1", request_headers, send_mess,
			&response, NULL, &recv_mess);
	if ( error != 0 )
		goto edg_wll_loadevents_end;

	if (http_check_status(ctx, response, &recv_mess))
		goto edg_wll_loadevents_end;

	edg_wll_ParseLoadResult(ctx, recv_mess, result);

edg_wll_loadevents_end:
	if (response) free(response);
	if (recv_mess) free(recv_mess);
	if (send_mess) free(send_mess);
	return edg_wll_Error(ctx,NULL,NULL);
}
