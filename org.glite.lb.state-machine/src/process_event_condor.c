#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <syslog.h>

#include "glite/lb/producer.h"
#include "glite/lb/context-int.h"

#include "intjobstat.h"
#include "seqcode_aux.h"

/* TBD: share in whole logging or workload */
#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif

// XXX: maybe not needed any more
// if not, remove also last_condor_event_timestamp from intJobStat
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
#define rep_cond(a,b) { if (b) { free(a); a = strdup(b); } }

int processEvent_Condor(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{
	edg_wll_JobStatCode     old_state = js->pub.state;
	int			res = RET_OK;


	if ((js->last_seqcode != NULL) &&
			(edg_wll_compare_condor_seq(js->last_seqcode, e->any.seqcode) > 0) ) {
		res = RET_LATE;	
	}

	switch (e->any.type) {
		case EDG_WLL_EVENT_REGJOB:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_SUBMITTED;
				rep(js->pub.condor_status, "Idle");
			}
			if (USABLE_DATA(res)) {
				rep(js->pub.jdl, e->regJob.jdl);
			}
			break;
		case EDG_WLL_EVENT_CONDORMATCH:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_READY;
				rep(js->pub.condor_status, "Idle");
			}
			if (USABLE_DATA(res)) {
				rep_cond(js->pub.condor_dest_host,e->CondorMatch.dest_host);
			}
			break;
		case EDG_WLL_EVENT_CONDORREJECT:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_ABORTED;
				rep(js->pub.condor_status, "Unexpanded");
			}
			if (USABLE_DATA(res)) {
				switch(e->CondorReject.status_code) {
					case EDG_WLL_CONDORREJECT_NOMATCH:
						rep(js->pub.condor_reason,"No match found.");
						break;
					case EDG_WLL_CONDORREJECT_OTHER:
					default:
						break;
				}
			}
			break;
		case EDG_WLL_EVENT_CONDORSHADOWSTARTED:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_READY;
				rep(js->pub.condor_status, "Idle");
			}
			if (USABLE_DATA(res)) {
				switch (get_condor_event_source(e->any.seqcode)) {
					case EDG_WLL_CONDOR_EVENT_SOURCE_SCHED:
						js->pub.condor_shadow_pid = e->CondorShadowStarted.shadow_pid;
						break;
					default:
						break;
				}
			}
			break;
		case EDG_WLL_EVENT_CONDORSHADOWEXITED:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_DONE;
				rep(js->pub.condor_status, "Completed");
			}
			if (USABLE_DATA(res)) {
				switch (get_condor_event_source(e->any.seqcode)) {
					case EDG_WLL_CONDOR_EVENT_SOURCE_SHADOW:
						js->pub.condor_shadow_exit_status = e->CondorShadowExited.shadow_exit_status;
						break;
					default:
						break;
				}
			}
			break;
		case EDG_WLL_EVENT_CONDORSTARTERSTARTED:
			if (USABLE(res)) {
				switch (get_condor_event_source(e->any.seqcode)) {
					case EDG_WLL_CONDOR_EVENT_SOURCE_START:
						js->pub.state = EDG_WLL_JOB_SCHEDULED;
						rep(js->pub.condor_status, "Idle");
						break;
					case EDG_WLL_CONDOR_EVENT_SOURCE_STARTER:
						js->pub.state = EDG_WLL_JOB_RUNNING;
						rep(js->pub.condor_status, "Running");
						break;
					default:
						break;
				}
			}
			if (USABLE_DATA(res)) {
				switch (get_condor_event_source(e->any.seqcode)) {
					case EDG_WLL_CONDOR_EVENT_SOURCE_STARTER:
						rep(js->pub.condor_universe, e->CondorStarterStarted.universe);
						js->pub.condor_starter_pid = e->CondorStarterStarted.starter_pid;
						break;
					default:
						break;
				}
			}
			break;
		case EDG_WLL_EVENT_CONDORSTARTEREXITED:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_DONE;
				rep(js->pub.condor_status, "Completed");
			}
			if (USABLE_DATA(res)) {
				switch (get_condor_event_source(e->any.seqcode)) {
					case EDG_WLL_CONDOR_EVENT_SOURCE_START:
						js->pub.condor_starter_pid = e->CondorStarterExited.starter_pid;
						js->pub.condor_starter_exit_status = e->CondorStarterExited.starter_exit_status;
						break;
					case EDG_WLL_CONDOR_EVENT_SOURCE_STARTER:
						js->pub.condor_starter_pid = e->CondorStarterExited.starter_pid;
						js->pub.condor_job_pid = e->CondorStarterExited.job_pid;
						js->pub.condor_job_exit_status = e->CondorStarterExited.job_exit_status;
						break;
					default:
						break;
				}
			}
			break;
		case EDG_WLL_EVENT_CONDORRESOURCEUSAGE:
			if (USABLE(res)) {
			}
			if (USABLE_DATA(res)) {
			}
			break;
		case EDG_WLL_EVENT_CONDORERROR:
			if (USABLE(res)) {
			}
			if (USABLE_DATA(res)) {
			}
			break;

		default:
			break;
	}

/* XXX : just debug output - remove */

	printf("processEvent_Condor(): %s (%s), state: %s --> %s\n ", 
		edg_wll_EventToString(e->any.type), 
		(res == RET_LATE) ? "RET_LATE" : "RET_OK", 
		edg_wll_StatToString(old_state), 
		edg_wll_StatToString(js->pub.state) );
	printf("\t%s\n",e->any.seqcode);
	printf("\t(last=%s)\n",js->last_seqcode);

/*----------------------------------*/

	if (USABLE(res)) {
		rep(js->last_seqcode, e->any.seqcode);

		js->pub.lastUpdateTime = e->any.timestamp;
		if (old_state != js->pub.state) {
			js->pub.stateEnterTime = js->pub.lastUpdateTime;
			js->pub.stateEnterTimes[1 + js->pub.state]
				= (int)js->pub.lastUpdateTime.tv_sec;
		}
	}
	if (! js->pub.location) js->pub.location = strdup("this is CONDOR");


	return RET_OK;
}

