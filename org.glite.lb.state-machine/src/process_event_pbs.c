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

// XXX: maybe not needed any more
// if not, remove also last_pbs_event_timestamp from intJobStat
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

int processEvent_PBS(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{
	edg_wll_JobStatCode     old_state = js->pub.state;
	int			res = RET_OK;


	if ((js->last_seqcode != NULL) &&
			(edg_wll_compare_pbs_seq(js->last_seqcode, e->any.seqcode) > 0) ) {
		res = RET_LATE;	
	}

	switch (e->any.type) {
		case EDG_WLL_EVENT_REGJOB:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_SUBMITTED;
				rep(js->pub.pbs_state, "Q");
			}
			if (USABLE_DATA(res)) {
			}
			break;
		case EDG_WLL_EVENT_PBSQUEUED:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_WAITING;
				rep(js->pub.pbs_state, "Q");
			}
			if (USABLE_DATA(res)) {
				if (!js->pub.pbs_queue)
					js->pub.pbs_queue = strdup(e->PBSQueued.queue);
				assert(!strcmp(js->pub.pbs_queue, e->PBSQueued.queue));
				/* rep_cond(js->pub.pbs_owner,e->PBSQueued.owner);
				rep_cond(js->pub.pbs_name,e->PBSQueued.name); */
			}
			break;
		case EDG_WLL_EVENT_PBSMATCH:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_READY;
				rep(js->pub.pbs_state, "Q");
			}
			if (USABLE_DATA(res)) {
				rep_cond(js->pub.pbs_dest_host,e->PBSMatch.dest_host);
			}
			break;
		case EDG_WLL_EVENT_PBSPENDING:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_WAITING;
				rep(js->pub.pbs_state, "Q");
				js->pbs_reruning = 0;		// reset possible reruning flag
			}
			if (USABLE_DATA(res)) {
				rep_cond(js->pub.pbs_reason,e->PBSPending.reason);
			}
			break;
		case EDG_WLL_EVENT_PBSRUN:
			if (USABLE(res)) {
				switch (get_pbs_event_source(e->any.seqcode)) {
					case EDG_WLL_PBS_EVENT_SOURCE_SERVER:
						js->pub.state = EDG_WLL_JOB_SCHEDULED;
						rep(js->pub.pbs_state, "Q");
						break;
					case EDG_WLL_PBS_EVENT_SOURCE_MOM:
						js->pub.state = EDG_WLL_JOB_RUNNING;
						rep(js->pub.pbs_state, "R");
						break;
					default:
						assert(0); // running event from strange source
						break;
				}
			}
			if (USABLE_DATA(res)) {
				rep_cond(js->pub.pbs_scheduler, e->PBSRun.scheduler);
				rep_cond(js->pub.pbs_dest_host, e->PBSRun.dest_host);
				js->pub.pbs_pid = e->PBSRun.pid;
			}
			break;
		case EDG_WLL_EVENT_PBSRERUN:
			if (USABLE(res)) {
				switch (get_pbs_event_source(e->any.seqcode)) {
					case EDG_WLL_PBS_EVENT_SOURCE_SERVER:
						js->pub.state = EDG_WLL_JOB_WAITING;
						rep(js->pub.pbs_state, "Q");
						break;
					case EDG_WLL_PBS_EVENT_SOURCE_MOM:
						js->pub.state = EDG_WLL_JOB_WAITING;
						rep(js->pub.pbs_state, "E");
						js->pbs_reruning = 1;
						break;
					default:
						assert(0); // running event from strande source
						break;
				}
			}
			if (USABLE_DATA(res)) {
			}
			break;
		case EDG_WLL_EVENT_PBSDONE:
			if (USABLE(res)) {
				switch (get_pbs_event_source(e->any.seqcode)) {
					case EDG_WLL_PBS_EVENT_SOURCE_SERVER:
						js->pub.state = EDG_WLL_JOB_DONE;
						js->pub.done_code = EDG_WLL_STAT_OK;
						rep(js->pub.pbs_state, "C");
						break;
					case EDG_WLL_PBS_EVENT_SOURCE_MOM:
						if (!js->pbs_reruning) {
							js->pub.state = EDG_WLL_JOB_DONE;
							js->pub.done_code = EDG_WLL_STAT_OK;
							rep(js->pub.pbs_state, "C");
						}
						break;
					default:
						assert(0); //done event from strange source
						break;
				}
			}
			if (USABLE_DATA(res)) {
				js->pub.pbs_exit_status =  e->PBSDone.exit_status;	
			}
			break;
		case EDG_WLL_EVENT_PBSRESOURCEUSAGE:
			if (USABLE(res)) {
				// signalize state done, done_code uknown
				js->pub.state = EDG_WLL_JOB_DONE;
				rep(js->pub.pbs_state, "C");
			}
			if (USABLE_DATA(res)) {
				char *new_resource_usage;
	
				trio_asprintf(&new_resource_usage,"%s%s\t%s = %f [%s]",
					(js->pub.pbs_resource_usage) ? js->pub.pbs_resource_usage : "",
					(js->pub.pbs_resource_usage) ? "\n": "",
					e->PBSResourceUsage.name,
					e->PBSResourceUsage.quantity,
					e->PBSResourceUsage.unit);

				if (js->pub.pbs_resource_usage) free(js->pub.pbs_resource_usage);
				js->pub.pbs_resource_usage = new_resource_usage;
			}
			break;
		case EDG_WLL_EVENT_PBSERROR:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_DONE;
				js->pub.done_code = EDG_WLL_STAT_FAILED;
				rep(js->pub.pbs_state, "C");
			}
			if (USABLE_DATA(res)) {
				char *new_error_desc;

				trio_asprintf(&new_error_desc,"%s%s\t%s",
					(js->pub.pbs_error_desc) ? js->pub.pbs_error_desc : "",
					(js->pub.pbs_error_desc) ? "\n" : "",
					e->PBSError.error_desc);
				
				if (js->pub.pbs_error_desc) free(js->pub.pbs_error_desc);
				js->pub.pbs_error_desc = new_error_desc;	
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

/* XXX : just debug output - remove */

	/*printf("processEvent_PBS(): %s (%s), state: %s --> %s\n ", 
		edg_wll_EventToString(e->any.type), 
		(res == RET_LATE) ? "RET_LATE" : "RET_OK", 
		edg_wll_StatToString(old_state), 
		edg_wll_StatToString(js->pub.state) );
	printf("\t%s\n",e->any.seqcode);
	printf("\t(last=%s)\n",js->last_seqcode);*/

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
	if (! js->pub.location) js->pub.location = strdup("this is PBS");


	return RET_OK;
}

