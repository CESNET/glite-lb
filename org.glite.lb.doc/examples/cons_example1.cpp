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

/*headers*/
#include "glite/jobid/JobId.h" 
#include "glite/lb/ServerConnection.h" 
#include "glite/lb/Job.h"
/*end headers*/

#include "iostream"

using namespace glite::lb;
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
	cerr <<  "usage: " << me << " [option]\n" 
		"\t-h, --help      Shows this screen.\n"
		"\t-s, --server    Server address.\n"
		"\t-p, --port      Query server port.\n"
		"\t-j, --jobid     ID of requested job.\n"
		"\t-u, --user      User DN.\n";
}


int main(int argc, char *argv[])
{
	char			   *server, *jobid_s, *user;
	int					opt, err = 0;
	long				i;
	int port = 0;

	server = jobid_s = user = NULL;
	while ( (opt = getopt_long(argc, argv, "hs:p:j:u:c:n:v:", opts, NULL)) != EOF)
		switch (opt) {
		case 'h': usage(argv[0]); return 0;
		case 's': server = strdup(optarg); break;
		case 'p': port = atoi(optarg); break;
		case 'j': jobid_s = strdup(optarg); break;
		case 'u': user = strdup(optarg); break;
		case '?': usage(argv[0]); return 1;
		}

	if ( !jobid_s ) { fprintf(stderr, "JobId not given\n"); return 1; }
	//if ( !server ) { fprintf(stderr, "LB proxy socket not given\n"); return 1; }

	/*variables*/
	ServerConnection          lb_server;
	glite::jobid::JobId       jobid;
	std::vector<QueryRecord>  job_cond;
	std::vector<JobStatus>    statesOut;
	/*end variables*/

	try {
		/*queryserver*/
		jobid = glite::jobid::JobId(jobid_s);

		lb_server.setQueryServer(jobid.host(), jobid.port());
		/*end queryserver*/

		/*querycond*/
		job_cond.push_back(QueryRecord(QueryRecord::OWNER, QueryRecord::EQUAL, std::string(user)));
		job_cond.push_back(QueryRecord(QueryRecord::STATUS, QueryRecord::EQUAL, JobStatus::RUNNING));
		job_cond.push_back(QueryRecord(QueryRecord::DESTINATION, QueryRecord::EQUAL, std::string("xyz")));
		/*end querycond*/

		/*query*/
		statesOut = lb_server.queryJobStates(job_cond, 0);  //* \label{l:queryjobs}
		/*end query*/
			
		/*printstates*/
		for (i = 0; i< statesOut.size(); i++ ) {
			cout << "jobId : " <<  statesOut[i].getValJobId(JobStatus::JOB_ID).toString() << endl;
			cout << "state : " <<  statesOut[i].name() << endl << endl;
		}
		/*end printstates*/

	} catch(std::exception e) {
		cerr << e.what() << endl;
	}
	return 0;
}
