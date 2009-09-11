#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <syslog.h>

#include "glite/lb/context-int.h"

#include "glite/lbu/trio.h"

#include "intjobstat.h"
#include "seqcode_aux.h"

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
#define rep_cond(a,b) { if (b) { free(a); a = strdup(b); } }

int processEvent_Cream(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{
	edg_wll_JobStatCode     old_state = js->pub.state;
	int			res = RET_OK;


/*	not used in CREAM
	if ((js->last_seqcode != NULL) &&
			(edg_wll_compare_pbs_seq(js->last_seqcode, e->any.seqcode) > 0) ) {
		res = RET_LATE;	
	}
*/
	switch (e->any.type) {
		case EDG_WLL_EVENT_REGJOB:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_SUBMITTED;
				js->pub.cream_state = EDG_WLL_STAT_REGISTERED;
			}
			if (USABLE_DATA(res)) {
				rep_cond(js->pub.cream_owner, js->pub.owner);
				rep_cond(js->pub.cream_jdl, e->regJob.jdl);
				rep_cond(js->pub.cream_endpoint, e->regJob.ns);
			}
			break;
		case EDG_WLL_EVENT_CREAMSTART:
			// nothing to be done
			break;
		case EDG_WLL_EVENT_CREAMPURGE:
			// no state transition
		case EDG_WLL_EVENT_CREAMSTORE:
			if (USABLE(res)) {
				switch (e->CREAMStore.command) {
					case EDG_WLL_CREAMSTORE_CMDSTART:
						if (e->CREAMStore.result == EDG_WLL_CREAMSTORE_OK) {
							js->pub.state = EDG_WLL_JOB_WAITING;
							js->pub.cream_state = EDG_WLL_STAT_PENDING;
						}
						break;
					case EDG_WLL_CREAMSTORE_CMDSUSPEND:
						if (e->CREAMStore.result == EDG_WLL_CREAMSTORE_OK) {
							js->pub.suspended = 1;
						}
						break;
					case EDG_WLL_CREAMSTORE_CMDRESUME:
						if (e->CREAMStore.result == EDG_WLL_CREAMSTORE_OK) {
							js->pub.suspended = 0;
						}
						break;
					default:
						break;
				}
			}
			if (USABLE_DATA(res)) {
			}
			break;
		case EDG_WLL_EVENT_CREAMCALL:
			if (USABLE(res)) {
				if (e->CREAMCall.callee == EDG_WLL_SOURCE_LRMS && e->any.source == EDG_WLL_SOURCE_LRMS)
					js->pub.state = EDG_WLL_JOB_SCHEDULED;
					js->pub.cream_state = EDG_WLL_STAT_IDLE;

			}
			if (USABLE_DATA(res)) {
				rep(js->pub.cream_reason, e->CREAMCall.reason);
			}
			break;

		case EDG_WLL_EVENT_CREAMRUNNING:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_RUNNING;
				js->pub.cream_state = EDG_WLL_STAT_RUNNING;
				if (e->any.source == EDG_WLL_SOURCE_LRMS) {
					js->pub.cream_jw_status = EDG_WLL_STAT_WRAPPER_RUNNING;
				}
			}
			if (USABLE_DATA(res)) {
				rep(js->pub.cream_node, e->CREAMRunning.node);
			}
			break;
		case EDG_WLL_EVENT_CREAMREALLYRUNNING:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_RUNNING;
				js->pub.cream_state = EDG_WLL_STAT_REALLYRUNNING;
				if (e->any.source == EDG_WLL_SOURCE_LRMS) {
					js->pub.cream_jw_status = EDG_WLL_STAT_PAYLOAD_RUNNING;
				}
			}
			if (USABLE_DATA(res)) {
			}
			break;
		case EDG_WLL_EVENT_CREAMDONE:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_DONE;
				js->pub.cream_state = (e->CREAMDone.status_code == EDG_WLL_DONE_OK) ? EDG_WLL_STAT_DONEOK : EDG_WLL_STAT_DONEFAILED;
				if (e->any.source == EDG_WLL_SOURCE_LRMS) {
					js->pub.cream_jw_status = EDG_WLL_STAT_DONE;
				}
			}
			if (USABLE_DATA(res)) {
				js->pub.cream_done_code = e->CREAMDone.status_code;
				js->pub.cream_exit_code = e->CREAMDone.exit_code;
				rep(js->pub.cream_reason, e->CREAMDone.reason);
			}
			break;
		case EDG_WLL_EVENT_CREAMCANCEL:
			if (USABLE(res)) {
				if (e->CREAMCancel.status_code == EDG_WLL_CANCEL_DONE) {
					js->pub.state = EDG_WLL_JOB_CANCELLED;
					js->pub.cream_state = EDG_WLL_STAT_ABORTED;
				}
			}
			if (USABLE_DATA(res)) {
				rep(js->pub.cream_reason, e->CREAMCancel.reason);
			}
			break;
		case EDG_WLL_EVENT_CREAMABORT:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_ABORTED;
				js->pub.cream_state = EDG_WLL_STAT_ABORTED;
			}
			if (USABLE_DATA(res)) {
				rep(js->pub.cream_reason, e->CREAMAbort.reason);
			}
			break;

		case EDG_WLL_EVENT_USERTAG:
			if (USABLE_DATA(res)) {
				if (e->userTag.name != NULL && e->userTag.value != NULL) {
					add_taglist(e->userTag.name, e->userTag.value, e->any.seqcode, js);
				}
			}
			break;

		default:
			break;
	}

	if (USABLE(res)) {
		rep(js->last_seqcode, e->any.seqcode);

		js->pub.lastUpdateTime = e->any.timestamp;
		if (old_state != js->pub.state) {
			js->pub.stateEnterTime = js->pub.lastUpdateTime;
			js->pub.stateEnterTimes[1 + js->pub.state]
				= (int)js->pub.lastUpdateTime.tv_sec;
		}
	}
	if (! js->pub.location) js->pub.location = strdup("this is CREAM");


	return RET_OK;
}

