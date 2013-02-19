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


#define _GNU_SOURCE
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

/* XXX lookup table */
static char *cream_states[EDG_WLL_NUMBER_OF_CREAM_STATES];

// XXX move this defines into some common place to be reusable
#define USABLE(res) ((res) == RET_OK)
#define USABLE_DATA(res) (1)
#define rep(a,b) { free(a); a = (b == NULL) ? NULL : strdup(b); }
#define rep_cond(a,b) { if (b) { free(a); a = strdup(b); } }

int processData_Cream(intJobStat *js, edg_wll_Event *e)
{
	int			res = RET_OK;

	switch (e->any.type) {
		case EDG_WLL_EVENT_REGJOB:
			if (USABLE_DATA(res)) {
				rep_cond(js->pub.cream_owner, js->pub.owner);
				rep_cond(js->pub.jdl, e->regJob.jdl);
				rep_cond(js->pub.cream_jdl, e->regJob.jdl);
				rep_cond(js->pub.cream_endpoint, e->regJob.ns);
				rep_cond(js->pub.destination, e->regJob.ns);
				rep_cond(js->pub.network_server, e->regJob.ns);
			}
			break;
		case EDG_WLL_EVENT_CREAMSTART:
			// nothing to be done
			break;
		case EDG_WLL_EVENT_CREAMPURGE:
			// no state transition
			break;
		case EDG_WLL_EVENT_CREAMACCEPTED:
			if (USABLE(res)){
				rep(js->pub.cream_id, e->CREAMAccepted.local_jobid);
				rep(js->pub.globusId, e->CREAMAccepted.local_jobid);
			}
			break;
		case EDG_WLL_EVENT_CREAMSTATUS:
			if (USABLE(res) && e->CREAMStatus.result == EDG_WLL_CREAMSTATUS_DONE)
			{
				if (e->CREAMStatus.exit_code && strcmp(e->CREAMStatus.exit_code, "N/A"))
				{
					js->pub.cream_exit_code = atoi(e->CREAMStatus.exit_code);
					js->pub.exit_code = atoi(e->CREAMStatus.exit_code);
				}
				if (e->CREAMStatus.worker_node){ /*XXX should never be false */
					if (js->pub.cream_node) 
						free(js->pub.cream_node);
					js->pub.cream_node = strdup(e->CREAMStatus.worker_node);
					if (js->pub.ce_node)
						free(js->pub.ce_node);
					js->pub.ce_node = strdup(e->CREAMStatus.worker_node);
				}
				if (e->CREAMStatus.LRMS_jobid){ /*XXX should never be false */
					if (js->pub.cream_lrms_id) 
						free(js->pub.cream_lrms_id);
					js->pub.cream_lrms_id = strdup(e->CREAMStatus.LRMS_jobid);
					if (js->pub.localId)
						free(js->pub.localId);
					js->pub.localId = strdup(e->CREAMStatus.LRMS_jobid);
				}
				if (e->CREAMStatus.failure_reason){
					if (js->pub.cream_failure_reason) 
						free(js->pub.cream_failure_reason);
					js->pub.cream_failure_reason = strdup(e->CREAMStatus.failure_reason);
					if (js->pub.failure_reasons){
						char *glued_reasons;
						asprintf(&glued_reasons,"%s\n", e->CREAMStatus.failure_reason);
						rep(js->pub.failure_reasons, glued_reasons);
					}
					else 
						asprintf(&(js->pub.failure_reasons),"%s", e->CREAMStatus.failure_reason);
				}
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

	if (! js->pub.location) js->pub.location = strdup("this is CREAM");

	return RET_OK;
}

int processEvent_Cream(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{
	edg_wll_JobStatCode     old_state = js->pub.state;
	int			res = RET_OK;

	if (!cream_states[0]) {
		int	i;
		for (i=0; i<EDG_WLL_NUMBER_OF_CREAM_STATES; i++)
			cream_states[i] = edg_wll_CreamStatToString(i);
	}

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
				js->pub.cream_state = EDG_WLL_STAT_CREAM_REGISTERED;
			}
			break;
		case EDG_WLL_EVENT_CREAMSTART:
			// nothing to be done
			break;
		case EDG_WLL_EVENT_CREAMPURGE:
			// no state transition
			break;
		case EDG_WLL_EVENT_CREAMACCEPTED:
			break;
		case EDG_WLL_EVENT_CREAMSTORE:
			if (USABLE(res)) {
				switch (e->CREAMStore.command) {
					case EDG_WLL_CREAMSTORE_CMDSTART:
						if (e->CREAMStore.result == EDG_WLL_CREAMSTORE_OK) {
							js->pub.state = EDG_WLL_JOB_WAITING;
							js->pub.cream_state = EDG_WLL_STAT_CREAM_PENDING;
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
				rep_cond(js->pub.cream_reason, e->CREAMStore.reason);
				rep_cond(js->pub.reason, e->CREAMStore.reason);
			}
			if (USABLE_DATA(res)) {
			}
			break;
		case EDG_WLL_EVENT_CREAMCALL:
			if (e->any.source == EDG_WLL_SOURCE_CREAM_EXECUTOR &&
				e->CREAMCall.callee == EDG_WLL_SOURCE_LRMS &&
				e->CREAMCall.command == EDG_WLL_CREAMCALL_CMDSTART &&
				e->CREAMCall.result == EDG_WLL_CREAMCALL_OK)
			{
				if (USABLE(res)) {
					// BLAH -> LRMS
						js->pub.state = EDG_WLL_JOB_SCHEDULED;
						js->pub.cream_state = EDG_WLL_STAT_CREAM_IDLE;
						rep_cond(js->pub.cream_reason, e->CREAMCall.reason);
						rep_cond(js->pub.reason, e->CREAMCall.reason);
					}
	
				if (USABLE_DATA(res)) {
					rep(js->pub.cream_reason, e->CREAMCall.reason);
					rep(js->pub.reason, e->CREAMCall.reason);
				}
			}
			if (e->CREAMCall.command == EDG_WLL_CREAMCALL_CMDCANCEL &&
                                e->CREAMCall.result == EDG_WLL_CREAMCALL_OK)
			{
				if (USABLE(res)){
					js->pub.cream_cancelling = 1;
					js->pub.cancelling = 1;
				}
				rep_cond(js->pub.cream_reason, e->CREAMCall.reason);
				rep_cond(js->pub.reason, e->CREAMCall.reason);
			}
			if (e->CREAMCall.command == EDG_WLL_CREAMCALL_CMDPURGE &&
				e->CREAMCall.result == EDG_WLL_CREAMCALL_OK)
			{
				if (USABLE(res)){
					js->pub.state = EDG_WLL_JOB_CLEARED;
					js->pub.cream_state = EDG_WLL_STAT_CREAM_PURGED;
				}
				rep_cond(js->pub.cream_reason, e->CREAMCall.reason);
                                rep_cond(js->pub.reason, e->CREAMCall.reason);
			}
			break;
		case EDG_WLL_EVENT_CREAMCANCEL:
			if (USABLE(res)) {
				if (e->CREAMCancel.status_code == EDG_WLL_CANCEL_DONE) {
					js->pub.state = EDG_WLL_JOB_CANCELLED;
					js->pub.cream_state = EDG_WLL_STAT_CREAM_ABORTED;
				}
			}
			if (USABLE_DATA(res)) {
				rep(js->pub.cream_reason, e->CREAMCancel.reason);
				rep(js->pub.reason, e->CREAMCancel.reason);
			}
			break;
		case EDG_WLL_EVENT_CREAMABORT:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_ABORTED;
				js->pub.cream_state = EDG_WLL_STAT_CREAM_ABORTED;
			}
			if (USABLE_DATA(res)) {
				rep(js->pub.cream_reason, e->CREAMAbort.reason);
				rep(js->pub.reason, e->CREAMAbort.reason);
			}
			break;

		case EDG_WLL_EVENT_CREAMSTATUS:
			if (USABLE(res) && e->CREAMStatus.result == EDG_WLL_CREAMSTATUS_DONE)
			{
				switch (js->pub.cream_state = edg_wll_StringToCreamStat(e->CREAMStatus.new_state))
				{
					/* XXX: should not arrive */
					case EDG_WLL_STAT_CREAM_REGISTERED:
					case EDG_WLL_NUMBER_OF_CREAM_STATES:
						break;

					case EDG_WLL_STAT_CREAM_PENDING: js->pub.state = EDG_WLL_JOB_WAITING; break;
					case EDG_WLL_STAT_CREAM_IDLE: js->pub.state = EDG_WLL_JOB_SCHEDULED; break;
					case EDG_WLL_STAT_CREAM_RUNNING:
						js->pub.state = EDG_WLL_JOB_RUNNING;
						js->pub.jw_status = EDG_WLL_STAT_WRAPPER_RUNNING;
						break;
					case EDG_WLL_STAT_CREAM_REALLY_RUNNING:
						js->pub.state = EDG_WLL_JOB_RUNNING;
						js->pub.jw_status = EDG_WLL_STAT_PAYLOAD_RUNNING;
						break;
					case EDG_WLL_STAT_CREAM_HELD: /* TODO */ break;
					case EDG_WLL_STAT_CREAM_DONE_OK:
						js->pub.state = EDG_WLL_JOB_DONE;
						js->pub.done_code = EDG_WLL_STAT_OK;
						js->pub.cream_done_code = EDG_WLL_STAT_OK;
						break;
					case EDG_WLL_STAT_CREAM_DONE_FAILED:
						js->pub.state = EDG_WLL_JOB_DONE;
						js->pub.done_code = EDG_WLL_STAT_FAILED;
						js->pub.cream_done_code = EDG_WLL_STAT_FAILED;
						break;
					case EDG_WLL_STAT_CREAM_CANCELLED: js->pub.state = EDG_WLL_JOB_CANCELLED; break;
					case EDG_WLL_STAT_CREAM_ABORTED: js->pub.state = EDG_WLL_JOB_ABORTED; break;
					case EDG_WLL_STAT_CREAM_PURGED: js->pub.state = EDG_WLL_JOB_CLEARED; break;
				}
			}
			break;

		case EDG_WLL_EVENT_USERTAG:
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

	processData_Cream(js, e);

	return RET_OK;
}

