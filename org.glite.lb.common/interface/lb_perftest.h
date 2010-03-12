#ifndef GLITE_LB_PERFTEST_H
#define GLITE_LB_PERFTEST_H 

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


#include "glite/jobid/cjobid.h"
#include "glite/jobid/strmd5.h"

#ifdef BUILDING_LB_COMMON
#include "events.h"
#else
#include "glite/lb/events.h"
#endif

#define PERFTEST_END_TAG_NAME "lb_perftest"
#define PERFTEST_END_TAG_VALUE "+++ konec testu +++"

int
glite_wll_perftest_init(const char *host,         /** hostname */
			const char *user,         /** user running this test */
			const char *testname,     /** name of the test */
			const char *filename,     /** file with events for job source */
			int n);                   /** number of jobs for job source */

char *
glite_wll_perftest_produceJobId();

int
glite_wll_perftest_produceEventString(char **event, char **jobid);

int
glite_wll_perftest_consumeEvent(edg_wll_Event *event);

int
glite_wll_perftest_consumeEventString(const char *event_string);

int
glite_wll_perftest_consumeEventIlMsg(const char *msg);

int 
glite_wll_perftest_createJobId(const char *bkserver,
			       int port,
			       const char *test_user,
			       const char *test_name,
			       int job_num,
			       edg_wlc_JobId *jobid);

#endif /* GLITE_LB_PERFTEST_H */
