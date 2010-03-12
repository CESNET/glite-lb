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


/**
 * @file Job.cpp
 * @version $Revision$
 */


#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "Job.h"
#include "consumer.h"
#include "glite/lb/LoggingExceptions.h"
#include "glite/lb/context-int.h"

EWL_BEGIN_NAMESPACE;

#define CLASS_PREFIX "glite::lb::Job::"

const int Job::STAT_CLASSADS = EDG_WLL_STAT_CLASSADS;
const int Job::STAT_CHILDREN = EDG_WLL_STAT_CHILDREN;
const int Job::STAT_CHILDSTAT = EDG_WLL_STAT_CHILDSTAT;

Job::Job(void)
  : jobId() 
{
}


Job::Job(const glite::jobid::JobId &in) 
  : jobId(in)
{
}


Job::~Job(void) 
{
}


Job & Job::operator= (const glite::jobid::JobId &in) 
{
  try {
    jobId = in;
    return *this;
  } catch (Exception &) {
    STACK_ADD;
    throw;
  }
}


JobStatus 
Job::status(int flags) const
{
  JobStatus	  jobStatus;

  try {
    edg_wll_JobStat	*cstat = jobStatus.c_ptr();
    int ret = edg_wll_JobStatus(server.getContext(),
				jobId.c_jobid(), 
				flags,
				cstat);
    check_result(ret,
		 server.getContext(),
		 "edg_wll_JobStatus");

/* XXX the enums match due to automatic generation */
    jobStatus.status = (JobStatus::Code) cstat->state;
    
    return(jobStatus);

  } catch (Exception &e) {
    STACK_ADD;
    throw;
  }
}


void 
Job::log(std::vector<Event> &eventList) const
{
  edg_wll_Event *events = NULL,*ev;
  int             result, qresults_param;
  char            *errstr = NULL;
  edg_wll_Context context;

  try {
    context = server.getContext();
    result = edg_wll_JobLog(context, jobId.c_jobid(), &events);
    if (result == E2BIG) {
	    edg_wll_Error(context, NULL, &errstr);
	    check_result(edg_wll_GetParam(context,
				    EDG_WLL_PARAM_QUERY_RESULTS, &qresults_param),
			    context,
			    "edg_wll_GetParam(EDG_WLL_PARAM_QUERY_RESULTS)");
	    if (qresults_param != EDG_WLL_QUERYRES_LIMITED) {
		    edg_wll_SetError(context, result, errstr);
		    check_result(result, context,"edg_wll_JobLog");
		}
    } else {
	    check_result(result, context,"edg_wll_JobLog");
    }

    for (int i=0; events[i].type != EDG_WLL_EVENT_UNDEF; i++) {
	ev = (edg_wll_Event *) malloc(sizeof *ev);
	memcpy(ev,events+i,sizeof *ev);
      eventList.push_back(Event(ev)); 
    }

    free(events);

    if (result) {
	    edg_wll_SetError(context, result, errstr);
	    check_result(result, context,"edg_wll_JobLog");
    }
  } catch (Exception &e) {
    if(errstr) free(errstr);

    STACK_ADD;
    throw;
  }

}


const std::vector<Event> 
Job::log(void) const
{
  std::vector<Event>	eventList;
  
  log(eventList);
  return(eventList);
}


const std::pair<std::string,u_int16_t> 
Job::queryListener(std::string const & name) const
{
  std::string	host;
  char *c_host = NULL;
  uint16_t   port;

  try {
    int ret = edg_wll_QueryListener(server.getContext(),
				    jobId.c_jobid(),
				    name.c_str(),
				    &c_host,
				    &port);
    check_result(ret,
		 server.getContext(),
		 "edg_wll_QueryListener");
    
    host = c_host;
    free(c_host);
    return(std::pair<std::string,u_int16_t>(host,port));    

  } catch (Exception &e) {
    if(c_host) free(c_host);
    STACK_ADD;
    throw;
  }
}


void Job::setParam(edg_wll_ContextParam par, int val)
{
	server.setParam(par,val);
}

void Job::setParam(edg_wll_ContextParam par, const std::string val)
{
	server.setParam(par,val);
}
 
void Job::setParam(edg_wll_ContextParam par, const struct timeval & val)
{
	server.setParam(par,val);
}
 

int Job::getParamInt(edg_wll_ContextParam par) const
{
	return server.getParamInt(par);
}

std::string Job::getParamString(edg_wll_ContextParam par) const
{
	return server.getParamString(par);
}

struct timeval Job::getParamTime(edg_wll_ContextParam par) const
{
	return server.getParamTime(par);
}

  

EWL_END_NAMESPACE;
