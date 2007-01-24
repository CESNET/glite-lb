#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <syslog.h>

#include "glite/lb/producer.h"
#include "glite/lb/context-int.h"

#include "jobstat.h"
#include "lock.h"

/* TBD: share in whole logging or workload */
#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif

int processEvent_PBS(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{
	fputs("processEvent_PBS()",stderr);

	if (! js->pub.location) js->pub.location = strdup("this is PBS");
	return RET_OK;
}

