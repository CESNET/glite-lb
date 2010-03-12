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

extern "C" {
#include <getopt.h>
#include <stdlib.h>
}

#include <iostream>
#include <string>

/*headers*/
#include "glite/jobid/JobId.h" 
#include "glite/lb/Event.h"
#include "glite/lb/ServerConnection.h" 
/*end headers*/

using namespace std;

static struct option opts[] = {
	{"help",		0,	NULL,	'h'},
	{"server",		1,	NULL,	's'},
	{"port",		1,	NULL,	'p'},
	{"jobid",		1,	NULL,	'j'},
	{"user",		1,	NULL,	'u'},
};

static void usage(char *me)
{
	cerr << "usage: %s [option]\n"
	     <<	"\t-h, --help      Shows this screen.\n"
	     <<	"\t-s, --server    LB server.\n"
	     <<	"\t-p, --port      LB server port.\n"
	     <<	"\t-j, --jobid     ID of requested job.\n"
	     <<	"\t-u, --user      User DN.\n"
	     <<	me << endl;
}


int main(int argc, char *argv[])
{
	string		server_s, jobid_s, user;
	int		opt, err = 0;
	glite_jobid_t   jobid = NULL;
	long 		i;
	int             port = 0;
	
	
	server = jobid_s = NULL;
	while ( (opt = getopt_long(argc, argv, "hs:p:j:u:c:n:v:", opts, NULL)) != EOF)
		switch (opt) {
		case 'h': usage(argv[0]); return 0;
		case 's': server_s = optarg; break;
		case 'p': port = atoi(optarg); break;
		case 'j': jobid_s = optarg; break;
		case 'u': user = optarg; break;
		case '?': usage(argv[0]); return 1;
		}
	
	if ( jobid_s.empty() ) { 
		cerr <<  "JobId not given\n"; return 1; 
	}

        /*variables*/
	ServerConnection    server;
	Event               *eventsOut;
	vector<ServerConnection::QueryRec>    jc;
	vector<ServerConnection::QueryRec>    ec;
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
