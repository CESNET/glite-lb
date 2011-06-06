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
#include "glite/lb/xml_parse.h"
#include "glite/lb/mini_http.h"
#include "glite/lb/query_rec.h"
#include "glite/lb/consumer.h"
#include "glite/lb/connection.h"

#define dprintf(x) { if (debug) printf x; }

#define free_jobs(jobs) {					\
	if (jobs) {						\
		int i;						\
		for ( i = 0; jobs[i]; i++ )			\
			free(jobs[i]);				\
		free(jobs);					\
	}							\
}

static const char rcsid[] = "@(#)$Id$";

static int debug=0;
static char *file;

static int edg_wll_Purge(
                edg_wll_Context ctx,
                edg_wll_PurgeRequest *request,
                edg_wll_PurgeResult *result);
static int read_jobIds(const char *file, char ***jobs_out);
static int get_timeout(const char *arg, int *timeout);
static void printerr(edg_wll_Context ctx);

static struct option opts[] = {
	{ "aborted",		required_argument, NULL, 'a'},
	{ "cleared",		required_argument, NULL, 'c'},
	{ "cancelled",		required_argument, NULL, 'n'},
	{ "done",		required_argument, NULL, 'e'},
	{ "other",		required_argument, NULL, 'o'},
	{ "dry-run",		no_argument, NULL, 'r'},
	{ "jobs",		required_argument, NULL, 'j'},
	{ "return-list",	no_argument, NULL, 'l'},
	{ "server-dump",	no_argument, NULL, 's'},
	{ "client-dump",	no_argument, NULL, 'i'},
	{ "help",	 	no_argument, NULL, 'h' },
	{ "version", 		no_argument, NULL, 'v' },
	{ "debug", 		no_argument, NULL, 'd' },
	{ "server",	 	required_argument, NULL, 'm' },
	{ "proxy",	 	no_argument, NULL, 'x' },
	{ "sock",	 	required_argument, NULL, 'X' },
	{"target-runtime",	required_argument, NULL, 't'},
	{"background",		required_argument, NULL, 'b'},
	{ NULL,			no_argument, NULL,  0 }
};

static void usage(char *me)
{
	fprintf(stderr,"usage: %s [option]\n"
		"	-a, --aborted NNN[smhd]     purge ABORTED jobs older than NNN secs/mins/hours/days\n"
		"	-c, --cleared NNN[smhd]     purge CLEARED jobs older than given time\n"
		"	-n, --cancelled NNN[smhd]   purge CANCELLED jobs older than given time\n"
		"	-e, --done NNN[smhd]	    purge DONE jobs older than given time\n"
		"	-o, --other NNN[smhd]       purge OTHER jobs older than given time\n"
		"	-r, --dry-run               do not really purge\n"
		"	-j, --jobs <filename>       input file with jobIds of jobs to purge\n"
		"	-l, --return-list           return list of jobid matching the purge/dump criteria\n"
		"	-s, --server-dump           dump jobs into any server file\n"
		"	-i, --client-dump           receive stream of dumped jobs\n"
		"	-h, --help                  display this help\n"
		"	-v, --version               display version\n"
		"	-d, --debug                 diagnostic output\n"
		"	-m, --server                L&B server machine name\n"
		"	-x, --proxy		    purge L&B proxy\n"
		"	-X, --sock <path>	    purge L&B proxy using default socket path\n"
		"	-t, --target-runtime NNN[smhd]	throttle purge to the estimated target runtime\n"
		"	-b, --background 	    purge on the background\n",
		me);
}

int main(int argc,char *argv[])
{
	edg_wll_PurgeRequest *request;
	edg_wll_PurgeResult *result;
	int	i, timeout, background, err = 1;
	char *server = NULL;

	char *me;
	int opt;
	edg_wll_Context	ctx;

	/* initialize request to server defaults */
	request = (edg_wll_PurgeRequest *) calloc(1,sizeof(edg_wll_PurgeRequest));
	request->jobs = NULL;
	for (i=0; i < EDG_WLL_NUMBER_OF_STATCODES; i++) {
		request->timeout[i]=-1;
	}
	request->flags = EDG_WLL_PURGE_REALLY_PURGE;

	/* initialize result */
	result = (edg_wll_PurgeResult *) calloc(1,sizeof(edg_wll_PurgeResult));

	me = strrchr(argv[0],'/');
	if (me) me++; else me=argv[0];

	/* initialize context */
	edg_wll_InitContext(&ctx);

	/* get arguments */
	while ((opt = getopt_long(argc,argv,"a:c:n:e:o:j:m:rlsidhvxX:t:b:",opts,NULL)) != EOF) {
		timeout=-1;
		background=-1;

		switch (opt) {

		case 'a': 
			if ((get_timeout(optarg,&timeout) != 0 )) {
				printf("Wrong usage of timeout argument.\n");
				usage(me);
				return 1;
			}
			if (timeout >= 0) {
				request->timeout[EDG_WLL_JOB_ABORTED]=timeout; 
			}
			break;

		case 'c':
			if (get_timeout(optarg,&timeout) != 0 ) {
				printf("Wrong usage of timeout argument.\n");
				usage(me);
				return 1;
			}
			if (timeout >= 0) {
				request->timeout[EDG_WLL_JOB_CLEARED]=timeout; 
			}
			break;

		case 'n': 
			if (get_timeout(optarg,&timeout) != 0 ) {
				printf("Wrong usage of timeout argument.\n");
				usage(me);
				return 1;
			}
			if (timeout >= 0) {
				request->timeout[EDG_WLL_JOB_CANCELLED]=timeout; 
			}
			break;
		case 'e': 
			if (get_timeout(optarg,&timeout) != 0 ) {
				printf("Wrong usage of timeout argument.\n");
				usage(me);
				return 1;
			}
			if (timeout >= 0) {
				request->timeout[EDG_WLL_JOB_DONE]=timeout; 
			}
			break;

		case 'o': 
			if (get_timeout(optarg,&timeout) != 0 ) {
				printf("Wrong usage of timeout argument.\n");
				usage(me);
				return 1;
			}
			if (timeout >= 0) 
				for (i = 0 ; i<EDG_WLL_NUMBER_OF_STATCODES ; i++) {
					if ((i == EDG_WLL_JOB_ABORTED) ||
						(i == EDG_WLL_JOB_CLEARED) ||
						(i == EDG_WLL_JOB_CANCELLED) ||
						(i == EDG_WLL_JOB_DONE)) continue;

					request->timeout[i]= timeout;
				}
			break;

		case 'm': server = optarg; break;
		case 'j': file = optarg; break;
		case 'r': request->flags &= (~EDG_WLL_PURGE_REALLY_PURGE); break;
		case 'l': request->flags |= EDG_WLL_PURGE_LIST_JOBS; break;
		case 's': request->flags |= EDG_WLL_PURGE_SERVER_DUMP; break;
		case 'i': request->flags |= EDG_WLL_PURGE_CLIENT_DUMP; break;
		case 'd': debug = 1; break;
		case 'v': fprintf(stdout,"%s:\t%s\n",me,rcsid); exit(0);
		case 'x': ctx->isProxy = 1; break;
		case 'X': 
			ctx->isProxy = 1; 
			edg_wll_SetParam(ctx, EDG_WLL_PARAM_LBPROXY_SERVE_SOCK, optarg); 
			break;
		case 't':
			if ((get_timeout(optarg,&timeout) != 0 )) {
				printf("Wrong usage of timeout argument.\n");
				usage(me);
				return 1;
			}
			if (timeout >= 0) {
				request->target_runtime=timeout; 
			}
			break;
		case 'b':
			background = atoi(optarg);
			break;
		case 'h':
		case '?': usage(me); return 1;
		}

		if ((background == -1 && request->target_runtime) || background > 0)
			request->flags |= EDG_WLL_PURGE_BACKGROUND;
	}

	/* read the jobIds from file, if wanted */
	if (file) {
		char **jobs=NULL;
		dprintf(("Reading jobIds form file \'%s\'...",file));
		if (read_jobIds(file,&jobs) != 0) {
			dprintf(("no.\n"));
			fprintf(stderr,"Unable to read jobIds from file \'%s\'\n",file);
			goto main_end;
		} else {
			dprintf(("yes.\n"));
		}
		if (!jobs) {
			dprintf(("File empty.\n"));
			goto main_end;
		}

		request->jobs = jobs;
	}

	/* check request */
	if (debug) {
		printf("Purge request:\n");
		printf("- flags: %d\n",request->flags);
		printf("- %d timeouts:\n",EDG_WLL_NUMBER_OF_STATCODES);
		for (i=0; i < EDG_WLL_NUMBER_OF_STATCODES; i++) {
			char *stat=edg_wll_StatToString(i);
			printf("\t%s: %ld\n",stat,request->timeout[i]);
			if (stat) free(stat);
		}
		printf("- list of jobs:\n");
		if (!request->jobs) {
			printf("Not specified.\n");
		} else {
			for ( i = 0; request->jobs[i]; i++ )
				printf("%s\n", request->jobs[i]);
		}
		printf("- target runtime: %ld\n", request->target_runtime);
	}

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

	/* that is the Purge */
	dprintf(("Running the edg_wll_Purge...\n"));
	if ((err = edg_wll_Purge(ctx, request, result)) != 0) {
		if (err == EDG_WLL_ERROR_ACCEPTED_OK) err = 0;
		else fprintf(stderr,"Error running the edg_wll_Purge().\n");
		printerr(ctx);

		switch ( edg_wll_Error(ctx, NULL, NULL) )
		{
		case ENOENT:
		case EPERM:
		case EINVAL:
			break;
		case EDG_WLL_ERROR_ACCEPTED_OK:
			/* fall-through */
		default:
			goto main_end;
		}
	}

	/* examine the result */
	dprintf(("Examining the result of edg_wll_Purge...\n"));
	if (result->server_file) {
		printf("Server dump: %s\n",result->server_file);
	} else {
		printf("The jobs were not dumped.\n");
	}
	if (request->flags & EDG_WLL_PURGE_LIST_JOBS) {
		printf("The following jobs %s purged:\n",
			request->flags & EDG_WLL_PURGE_REALLY_PURGE ? "were" : "would be");
		if (!result->jobs) printf("None.\n");
		else {
			int i;
			for ( i = 0; result->jobs[i]; i++ )
				printf("%s\n",result->jobs[i]);
		}
	}

main_end:
	dprintf(("End.\n"));
	if (request)
	{
		free_jobs(request->jobs);
		free(request);
	}
	if (result) free(result);
	edg_wll_FreeContext(ctx);
	return err != 0;
}


static void printerr(edg_wll_Context ctx) 
{
	char	*errt,*errd;

	edg_wll_Error(ctx,&errt,&errd);
	fprintf(stderr,"%s (%s)\n",errt,errd);
}


static int read_jobIds(const char *file, char ***jobs_out)
{
        FILE    *jobIds = fopen(file,"r");
        char    buf[256];
		char	**jobs;
        int     cnt = 0;

	jobs = NULL;


        if (!jobIds) {
                perror(file);
                return 1;
        }

        while ( 1 ) {
                char    *nl;
                if ( !fgets(buf,sizeof buf,jobIds) )
		{
			if (feof(jobIds))
				break;

			free_jobs(jobs);
			fprintf(stderr, "Error reading file\n");
			return 1;
		}
                nl = strchr(buf,'\n');
                if (nl) *nl = 0;
		/* TODO: check if it is really jobId, e.g. by edg_wlc_JobIdParse() */

		if ( !(jobs = realloc(jobs, (cnt+2)*sizeof(*jobs))) )
		{
			perror("cond_parse()");
			return(1);
		}
                jobs[cnt++] = strdup(buf);
        }
	if (jobs) jobs[cnt] = NULL;

        fclose(jobIds);
	*jobs_out = jobs;

        return 0;
}

static int get_timeout(const char *arg, int *timeout) 
{
	int t = -1;
	char tunit = '\0';

	if (sscanf(arg,"%d%c",&t,&tunit) > 0) {
		if (tunit) {
			switch (tunit) {
				case 'd': t *= 86400; break; // 24*60*60
				case 'h': t *= 3600; break; // 60*60
				case 'm': t *= 60; break;
				case 's': break;
				default: fprintf(stderr,"Allowed time units are s,m,h,d\n");
					return -1;
			}
		}
	}
	if (t < 0) return -1;
	*timeout = t;
	return 0;
}

static const char* const request_headers[] = {
	"Cache-Control: no-cache",
	"Accept: application/x-dglb",
	"User-Agent: edg_wll_Api/" PROTO_VERSION "/" COMP_PROTO,
	"Content-Type: application/x-dglb",
	NULL
};

/** Client side purge
 * \retval EAGAIN only partial result returned, call repeatedly to get all
 *      output data
 */
static int edg_wll_Purge(
		edg_wll_Context ctx,
		edg_wll_PurgeRequest *request,
		edg_wll_PurgeResult *result)
{
	char	*send_mess,
		*response = NULL,
		*recv_mess = NULL;

	edg_wll_ResetError(ctx);

	if (request->flags & EDG_WLL_PURGE_CLIENT_DUMP)
		return edg_wll_SetError(ctx,ENOSYS,"client dump");

	if (edg_wll_PurgeRequestToXML(ctx, request, &send_mess))
		goto edg_wll_purge_end;

	ctx->p_tmp_timeout = ctx->p_query_timeout;
	if (ctx->p_tmp_timeout.tv_sec < 600) ctx->p_tmp_timeout.tv_sec = 600;

	
	if (ctx->isProxy){
		ctx->isProxy = 0;
		if (edg_wll_http_send_recv_proxy(ctx, "POST /purgeRequest HTTP/1.1",
				request_headers,send_mess,&response,NULL,&recv_mess))
			goto edg_wll_purge_end;
	}
	else {
		if (set_server_name_and_port(ctx, NULL)) 
			goto edg_wll_purge_end;

		if (edg_wll_http_send_recv(ctx,
			"POST /purgeRequest HTTP/1.1", request_headers, send_mess,
			&response, NULL, &recv_mess)) 
			goto edg_wll_purge_end;
	}

	if (http_check_status(ctx, response))
		goto edg_wll_purge_end;

	if (edg_wll_ParsePurgeResult(ctx, recv_mess, result))
		goto edg_wll_purge_end;

edg_wll_purge_end:
	if (response) free(response);
	if (recv_mess) free(recv_mess);
	if (send_mess) free(send_mess);
	return edg_wll_Error(ctx,NULL,NULL);
}
