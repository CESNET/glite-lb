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

#include <iostream>
#include <utility>
#include <vector>

#include <sys/time.h>
#include <unistd.h>

#include "glite/wmsutils/jobid/JobId.h"
#include "glite/lb/Job.h"
#include "glite/wmsutils/exception/Exception.h"
#include "glite/lb/LoggingExceptions.h"

using namespace glite::lb;
using namespace std;


/* print results */
static void printStatus(JobStatus &stat);


int main(int argc,char *argv[])
{

	try {

	JobStatus jobStatus;

/* connect to server */
	ServerConnection server;

/* get status of the job */
	if (argc < 2 ||  strcmp(argv[1],"--help") == 0) {
		cout << "Usage : " << argv[0] << " job_id [job_id [...]]" << endl;
		cout << "        " << argv[0] << " -all server[:port] [limit]" << endl;
		return 1;
	}
	else 	if (argc >= 2 && strcmp(argv[1],"-all") == 0) {
		std::vector<JobStatus> jobStates;

		std::vector<QueryRecord> q(1,QueryRecord(QueryRecord::OWNER,QueryRecord::EQUAL,""));


		if (argc > 2) {
			char	*sc = strdup(argv[2]),*p;


			strtok(sc,":");
			p = strtok(NULL,":");

			server.setParam(EDG_WLL_PARAM_QUERY_SERVER,sc);
			if (p) server.setParam(EDG_WLL_PARAM_QUERY_SERVER_PORT,atoi(p));
		}
		

		if (argc > 3) server.setParam(EDG_WLL_PARAM_QUERY_JOBS_LIMIT,atoi(argv[3]));
		
		try {
			server.queryJobStates(q, 0, jobStates);
		} catch (LoggingException &e) {
			if (e.getCode() == E2BIG && 
				server.getParamInt(EDG_WLL_PARAM_QUERY_RESULTS) == EDG_WLL_QUERYRES_LIMITED)
			{
				cerr << e.dbgMessage() << endl;
				cout << "WARNING: Output truncated (soft limit = " <<
					server.getParamInt(EDG_WLL_PARAM_QUERY_JOBS_LIMIT) << ")" << endl; 
			}
			else throw;
		}

		cout << "Number of jobs: " << jobStates.size() << endl;
		cout << endl;

		std::vector<JobStatus>::iterator i = jobStates.begin();
		while (i != jobStates.end()) printStatus(*i++);
	}
	else {	
		Job job;
 		for (int i=1; i < argc; i++) {
			try {
				glite::wmsutils::jobid::JobId jobId(argv[i]);
		
				job = jobId;
				jobStatus = job.status(Job::STAT_CLASSADS);
				printStatus(jobStatus);
			} catch (glite::wmsutils::exception::Exception &e) {
				cerr << e.dbgMessage() << endl;
			}

		}
	}

	}
	catch (glite::wmsutils::exception::Exception &e) {
		cerr << e.dbgMessage() << endl;
	}
}


static void printStatus(JobStatus &stat) {
	std::vector<pair<JobStatus::Attr, JobStatus::AttrType> > attrList = stat.getAttrs();		
	cout << "status: " << stat.name() << endl;
	for (unsigned i=0; i < attrList.size(); i++ ) {
		cout << stat.getAttrName(attrList[i].first) << " = " ;
	
		switch (attrList[i].second) {

		case JobStatus::INT_T: 
			cout << stat.getValInt(attrList[i].first) << endl; 
			break;

		case JobStatus::STRING_T: 
			cout << stat.getValString(attrList[i].first) << endl; 
			break;

		case JobStatus::TIMEVAL_T: 
		{ 
			timeval t = stat.getValTime(attrList[i].first);
			cout << t.tv_sec << "." << t.tv_usec << " s " << endl; 
		}
		break;

		case JobStatus::JOBID_T:
			if(((glite::wmsutils::jobid::JobId)stat.getValJobId(attrList[i].first)).isSet())
				cout << stat.getValJobId(attrList[i].first).toString();
			cout << endl;
			break;

		case JobStatus::INTLIST_T: 
		{
			std::vector<int> v = stat.getValIntList(attrList[i].first);
			for(unsigned int i=0; i < v.size(); i++) 
				cout << v[i] << " ";
			cout << endl;
		}
		break;

		case JobStatus::STRLIST_T:
		{
			std::vector<std::string> v = stat.getValStringList(attrList[i].first);
			for(unsigned int i=0; i < v.size(); i++)
				cout << v[i] << " ";
			cout<< endl;
		}
		break;

		case JobStatus::TAGLIST_T:
		{
			std::vector<std::pair<std::string, std::string> > v = stat.getValTagList(attrList[i].first);
			for(unsigned int i=0; i < v.size(); i++)
				cout << v[i].first << "=" << v[i].second << " ";
			cout<< endl;
		}
		break;

		case JobStatus::STSLIST_T:
		{
			std::vector<JobStatus> v = stat.getValJobStatusList(attrList[i].first);
			for(unsigned int i=0; i < v.size(); i++)
				cout << v[i].name() << " ";
			cout << endl;
		}
		break;

		case JobStatus::BOOL_T:
			cout << stat.getValBool(attrList[i].first) << endl;
			break;

		default : /* something is wrong */ 
			cout << " ** attr type " 
			     << attrList[i].second 
			     << " not handled" << endl;
			break;
		}
	}
	cout << endl;
} 
