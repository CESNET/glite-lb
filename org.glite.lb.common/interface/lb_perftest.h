#ifndef LB_PERFTEST_H
#define LB_PERFTEST_H

#ident "$Header$"

#include "glite/wmsutils/jobid/cjobid.h"
#include "glite/wmsutils/jobid/strmd5.h"
#include "glite/lb/events.h"

#define PERFTEST_END_TAG_NAME "lb_perftest"
#define PERFTEST_END_TAG_VALUE "+++ konec testu +++"

int
glite_wll_perftest_init();

int
glite_wll_perftest_consumeEvent(edg_wll_Event *event);

int
glite_wll_perftest_consumeEventString(const char *event_string);

int 
glite_wll_perftest_createJobId(const char *bkserver,
			       int port,
			       const char *test_user,
			       int test_num,
			       int job_num,
			       edg_wlc_JobId *jobid);

#endif
