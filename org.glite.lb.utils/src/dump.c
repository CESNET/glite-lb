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

#define CLIENT_SBIN_PROG

#include "glite/lb/context-int.h"
#include "glite/lb/ulm_parse.h"
#include "glite/lb/xml_parse.h"
#include "glite/lb/mini_http.h"
#include "glite/lb/consumer.h"

#define dprintf(x) { if (debug) printf x; }

static const char rcsid[] = "@(#)$Id$";

static int debug=0;

static int edg_wll_DumpEvents(
                edg_wll_Context ctx,
                const edg_wll_DumpRequest *request,
                edg_wll_DumpResult *result);
static void printerr(edg_wll_Context ctx);

static struct option opts[] = {
	{ "from",		required_argument, NULL, 'f'},
	{ "to",			required_argument, NULL, 't'},
	{ "help",	 	no_argument, NULL, 'h' },
	{ "version", 		no_argument, NULL, 'v' },
	{ "debug", 		no_argument, NULL, 'd' },
	{ "server",	 	required_argument, NULL, 'm' },
	{ NULL,			no_argument, NULL,  0 }
};

static void usage(char *me)
{
	fprintf(stderr,"usage: %s [option]\n"
		"	-f, --from YYYYMMDDHHmmss   beginning of the time interval for events to be dumped*\n"
		"	-t, --to YYYYMMDDHHmmss     end of the time interval for events to be dumped*\n"
		"	-h, --help                  display this help\n"
		"	-v, --version               display version\n"
		"	-d, --debug                 diagnostic output\n"
		"	-m, --server                L&B server machine name\n\n"
		"	* Special values for temporal arguments '-f' and '-t':\n"
		"		'now'		    this exact moment\n"
		"		'last_start'	    last dump starting time\n"
		"		'last_end'	    last dump ending time\n",
		me);
}

double ParseDate(const char *s) {
	if (!strcmp(s,"now")) return EDG_WLL_DUMP_NOW;
	if (!strcmp(s,"last_start")) return EDG_WLL_DUMP_LAST_START;
	if (!strcmp(s,"last_end")) return EDG_WLL_DUMP_LAST_END;

	if(strlen(s)<14) return -16;

	int i;
	for (i = 0; i<14; i++) if((s[i]<'0')||(s[i]>'9')) return -16;
	
	return edg_wll_ULMDateToDouble(s);	
} 

int main(int argc,char *argv[])
{
	edg_wll_DumpRequest *request;
	edg_wll_DumpResult *result;
	char *server = NULL;
	char date[ULM_DATE_STRING_LENGTH+1];

	char *me;
	int opt;
	edg_wll_Context	ctx = NULL;

	/* initialize request to server defaults */
	request = (edg_wll_DumpRequest *) calloc(1,sizeof(edg_wll_DumpRequest));
	request->from = EDG_WLL_DUMP_LAST_END;
	request->to = EDG_WLL_DUMP_NOW;

	/* initialize result */
	result = (edg_wll_DumpResult *) calloc(1,sizeof(edg_wll_DumpResult));

	me = strrchr(argv[0],'/');
	if (me) me++; else me=argv[0];

	/* get arguments */
	while ((opt = getopt_long(argc,argv,"f:t:m:dvh",opts,NULL)) != EOF) {

		switch (opt) {

		case 'f': request->from = (time_t) ParseDate(optarg); break;
		case 't': request->to = (time_t) ParseDate(optarg); break;
		case 'm': server = optarg; break;
		case 'd': debug = 1; break;
		case 'v': fprintf(stdout,"%s:\t%s\n",me,rcsid); exit(0);
		case 'h':
		case '?': usage(me); return 1;
		}
	}

	if(request->from < -15) {
		fprintf(stdout, "Invalid 'from' date\n");
		goto main_end; }
	if(request->to < -15) {
		fprintf(stdout, "Invalid 'to' date\n");
		goto main_end; }

	/* check request */
	if (debug) {
		printf("Dump request:\n");
		if (request->from < 0) {
			printf("- from: %ld.\n",(long) request->from);
		} else {
			if (edg_wll_ULMTimevalToDate(request->from,0,date) != 0) {
				fprintf(stderr,"Error parsing 'from' argument.\n");
				goto main_end;
			}
			printf("- from: %ld (i.e. %s).\n",(long) request->from,date);
		}
		if (request->to < 0) {
			printf("- to: %ld.\n",(long) request->to);
		} else {
			if (edg_wll_ULMTimevalToDate(request->to,0,date) != 0) {
				fprintf(stderr,"Error parsing 'to' argument.\n");
				goto main_end;
			}
			printf("- to: %ld (i.e. %s).\n",(long) request->to,date);
		}
	}

	/* initialize context */
	edg_wll_InitContext(&ctx);
	if ( server )
	{
		char *p = strrchr(server, ':');
		if ( p )
		{
			edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER_PORT, atoi(p+1));
			*p = 0;
		}
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER, server);
	}

	/* that is the DumpEvents */
	dprintf(("Running the edg_wll_DumpEvents...\n"));
	if (edg_wll_DumpEvents(ctx, request, result) != 0) {
		fprintf(stderr,"Error running the edg_wll_DumpEvents().\n");
		printerr(ctx);
		switch ( edg_wll_Error(ctx, NULL, NULL) )
		{
		case ENOENT:
		case EPERM:
		case EINVAL:
			break;
		default:
			goto main_end;
		}
	}

	/* examine the result */
	dprintf(("Examining the result of edg_wll_DumpEvents...\n"));
	printf("Dump result:\n");
	if (result->server_file) {
		printf("- The jobs were dumped to the file '%s' at the server.\n",result->server_file);
	} else {
		printf("- The jobs were not dumped.\n");
	}
	if (edg_wll_ULMTimevalToDate(result->from,0,date) != 0) {
		fprintf(stderr,"Error parsing 'from' argument.\n");
		goto main_end;
	}
	printf("- from: %ld (i.e. %s).\n",(long) result->from,date);
	if (edg_wll_ULMTimevalToDate(result->to,0,date) != 0) {
		fprintf(stderr,"Error parsing 'to' argument.\n");
		goto main_end;
	}
	printf("- to: %ld (i.e. %s).\n",(long) result->to,date);

main_end:
	dprintf(("End.\n"));
	if (request) free(request);
	if (result) free(result);
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

/** Dump events in a given time interval
 */

static int edg_wll_DumpEvents(
		edg_wll_Context ctx,
		const edg_wll_DumpRequest *request,
		edg_wll_DumpResult *result)
{
	int	error;
	char	*send_mess,
		*response = NULL,
		*recv_mess = NULL;

	edg_wll_DumpRequestToXML(ctx, request, &send_mess);

	ctx->p_tmp_timeout = ctx->p_query_timeout;
	if (ctx->p_tmp_timeout.tv_sec < 600) ctx->p_tmp_timeout.tv_sec = 600;

	if (set_server_name_and_port(ctx, NULL))
		goto edg_wll_dumpevents_end;

	error = edg_wll_http_send_recv(ctx,
			"POST /dumpRequest HTTP/1.1", request_headers, send_mess,
			&response, NULL, &recv_mess);
	if ( error != 0 )
		goto edg_wll_dumpevents_end;

	if (http_check_status(ctx, response))
		goto edg_wll_dumpevents_end;

	edg_wll_ParseDumpResult(ctx, recv_mess, result);

edg_wll_dumpevents_end:
	if (response) free(response);
	if (recv_mess) free(recv_mess);
	if (send_mess) free(send_mess);
	return edg_wll_Error(ctx,NULL,NULL);
}
