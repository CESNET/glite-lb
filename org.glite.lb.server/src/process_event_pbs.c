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

static int compare_timestamps(struct timeval a, struct timeval b)
{
	if ( (a.tv_sec > b.tv_sec) || 
		((a.tv_sec == b.tv_sec) && (a.tv_usec > b.tv_usec)) ) return 1;
	if ( (a.tv_sec < b.tv_sec) ||
                ((a.tv_sec == b.tv_sec) && (a.tv_usec < b.tv_usec)) ) return -1;
	return 0;
}

// XXX move this defines into some common place to be reusable
#define USABLE(res) ((res) == RET_OK)
#define USABLE_DATA(res) (1)
#define rep(a,b) { free(a); a = (b == NULL) ? NULL : strdup(b); }

int processEvent_PBS(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{
	edg_wll_JobStatCode     old_state = js->pub.state;
	int			res = RET_OK;


	fputs("processEvent_PBS()",stderr);

	if (compare_timestamps(js->last_pbs_event_timestamp, e->any.timestamp) > 0)
		res = RET_LATE;	

	switch (e->any.type) {
		case EDG_WLL_EVENT_REGJOB:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_SUBMITTED;
				rep(js->pub.pbs_state, "Q");
			}
			if (USABLE_DATA(res)) {
			}
			break;
		case EDG_WLL_EVENT_PBSREG:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_SUBMITTED;
				rep(js->pub.pbs_state, "Q");
			}
			if (USABLE_DATA(res)) {
				js->pub.pbs_queue = strdup(e->pBSReg.queue);
			}
			break;
		case EDG_WLL_EVENT_PBSQUEUED:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_WAITING;
				rep(js->pub.pbs_state, "Q");
			}
			if (USABLE_DATA(res)) {
				if (!js->pub.pbs_queue)
					strdup(e->pBSQueued.queue);
				assert(!strcmp(js->pub.pbs_queue, e->pBSQueued.queue));
				rep(js->pub.pbs_owner,e->pBSQueued.owner);
				rep(js->pub.pbs_name,e->pBSQueued.name);
			}
			break;
		case EDG_WLL_EVENT_PBSPLAN:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_READY;
				rep(js->pub.pbs_state, "Q");
			}
			if (USABLE_DATA(res)) {
			}
			break;
		case EDG_WLL_EVENT_PBSRUN:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_RUNNING;
				rep(js->pub.pbs_state, "R");
			}
			if (USABLE_DATA(res)) {
				rep(js->pub.pbs_scheduler, e->pBSRun.scheduler);
				rep(js->pub.pbs_dest_host, e->pBSRun.dest_host);
				js->pub.pbs_pid = e->pBSRun.pid;
			}
			break;
		case EDG_WLL_EVENT_PBSDONE:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_DONE;
				rep(js->pub.pbs_state, "C");
			}
			if (USABLE_DATA(res)) {
				js->pub.pbs_exit_status =  e->pBSDone.exit_status;	
			}
			break;
		default:
			break;
	}

	if (USABLE(res)) {
		js->pub.lastUpdateTime = e->any.timestamp;
		if (old_state != js->pub.state) {
			js->pub.stateEnterTime = js->pub.lastUpdateTime;
			js->pub.stateEnterTimes[1 + js->pub.state]
				= (int)js->pub.lastUpdateTime.tv_sec;
		}
	}
	if (! js->pub.location) js->pub.location = strdup("this is PBS");
	return RET_OK;
}

