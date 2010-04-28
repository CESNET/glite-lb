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
	char 		*server_s, *jobid_s, *user;
	int		opt, err = 0;
	glite::jobid::JobId   jobid;
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
	
	if ( !jobid_s ) { 
		cerr <<  "JobId not given\n"; return 1; 
	}

        /*variables*/
	ServerConnection       lb_server;
	vector<Event>          eventsOut;
	vector<QueryRecord>    jc;
	vector<QueryRecord>    ec;
	/*end variables*/

	try {
	  
		/*context*/
		lb_server.setQueryServer(server_s, port);
		/*end context*/

		/*queryrec*/
		jc.push_back(QueryRecord("color", QueryRecord::EQUAL, "red"));
		ec.push_back(QueryRecord("color", QueryRecord::EQUAL, "green"));
		/*end queryrec*/

		/*query*/
		events_out = lb_server.queryEvents(jc,ec);
		/*end query*/


		/*printevents*/
		for(i = 0; i < eventsOut.size(); i++) {
			dumpEvent(&(eventsOut[i]));
		}
		/*end printevents*/

	} catch(std::exception e) {
		cerr << e.what() << endl;
	}

	return 0;
}
