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

/* maps PBS/Torque substates to LB job states */
static int _PBSsubstate2lbstate[] = {
	/* TRANSIN */  EDG_WLL_JOB_SUBMITTED,  
	/* TRANSICM */ EDG_WLL_JOB_SUBMITTED,   
	/* TRNOUT */   EDG_WLL_JOB_SUBMITTED,     
	/* TRNOUTCM */ EDG_WLL_JOB_SUBMITTED,   
	/* SUBSTATE04 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE05 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE06 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE07 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE08 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE09 */ EDG_WLL_JOB_UNKNOWN,
	/* QUEUED */   EDG_WLL_JOB_WAITING,     
	/* PRESTAGEIN */ EDG_WLL_JOB_READY, 
	/* SUBSTATE12 */ EDG_WLL_JOB_UNKNOWN,
	/* SYNCRES */  EDG_WLL_JOB_READY,    
	/* STAGEIN */  EDG_WLL_JOB_READY,    
	/* STAGEGO */  EDG_WLL_JOB_SCHEDULED,    
	/* STAGECMP */ EDG_WLL_JOB_READY,   
	/* SUBSTATE17 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE18 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE19 */ EDG_WLL_JOB_UNKNOWN,
	/* HELD */     EDG_WLL_JOB_WAITING,       
	/* SYNCHOLD */ EDG_WLL_JOB_WAITING,   
	/* DEPNHOLD */ EDG_WLL_JOB_WAITING,   
	/* SUBSTATE23 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE24 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE25 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE26 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE27 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE28 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE29 */ EDG_WLL_JOB_UNKNOWN,
	/* WAITING */  EDG_WLL_JOB_WAITING,    
	/* SUBSTATE31 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE32 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE33 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE34 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE35 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE36 */ EDG_WLL_JOB_UNKNOWN,
	/* STAGEFAIL */ EDG_WLL_JOB_UNKNOWN,  
	/* SUBSTATE38 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE39 */ EDG_WLL_JOB_UNKNOWN,
	/* PRERUN */    EDG_WLL_JOB_SCHEDULED,     
	/* STARTING */  EDG_WLL_JOB_SCHEDULED,
	/* RUNNING */   EDG_WLL_JOB_RUNNING,    
	/* SUSPEND */   EDG_WLL_JOB_WAITING,    
	/* SUBSTATE44 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE45 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE46 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE47 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE48 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE49 */ EDG_WLL_JOB_UNKNOWN,
	/* EXITING */   EDG_WLL_JOB_RUNNING,    
	/* STAGEOUT */  EDG_WLL_JOB_RUNNING,   
	/* STAGEDEL */  EDG_WLL_JOB_RUNNING,   
	/* EXITED */    EDG_WLL_JOB_RUNNING,     
	/* ABORT */     EDG_WLL_JOB_CANCELLED,      
	/* SUBSTATE55 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE56 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE57 */ EDG_WLL_JOB_UNKNOWN,
	/* OBIT */     EDG_WLL_JOB_RUNNING,       
	/* COMPLETE */ EDG_WLL_JOB_DONE,
	/* RERUN */    EDG_WLL_JOB_WAITING,      
	/* RERUN1 */   EDG_WLL_JOB_WAITING,     
	/* RERUN2 */   EDG_WLL_JOB_WAITING,     
	/* RERUN3 */   EDG_WLL_JOB_WAITING,     
	/* SUBSTATE64 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE65 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE66 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE67 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE68 */ EDG_WLL_JOB_UNKNOWN,
	/* SUBSTATE69 */ EDG_WLL_JOB_UNKNOWN,
	/* RETURNSTD */ EDG_WLL_JOB_RUNNING  
};


/* maps PBS/Torque job states to status characters for display */
static char *_PBSstate2char[] = {
	/* TRANSIT -> */ "T",
	/* QUEUED  -> */ "Q",
	/* HELD    -> */ "H",
	/* WAITING -> */ "W",
	/* RUNNING -> */ "R",
	/* EXITING -> */ "E",
	/* COMPLETE-> */ "C"
};

int processEvent_PBS(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{
	edg_wll_JobStatCode     old_state = js->pub.state;
	int			res = RET_OK;


	if ((js->last_seqcode != NULL) &&
			(edg_wll_compare_pbs_seq(js->last_seqcode, e->any.seqcode) > 0) ) {
		res = RET_LATE;	
	}

	switch (e->any.type) {
	case EDG_WLL_EVENT_PBSINTERNALSTATECHANGE:
		if(USABLE(res)) {
			/* TODO: should we use this? Maybe to cross check...
			 * js->pub.state = _PBSsubstate2lbstate[e->PBSInternalStateChange.newsubstate]; 
			 */
			rep(js->pub.pbs_state, _PBSstate2char[e->PBSInternalStateChange.newstate]);
			js->pub.pbs_substate = e->PBSInternalStateChange.newsubstate;
		}
		break;

	case EDG_WLL_EVENT_REGJOB:
		if (USABLE(res)) {
			js->pub.state = EDG_WLL_JOB_SUBMITTED;
		}
		if (USABLE_DATA(res)) {
			/* this is going to be the first server taking care of the job */
			rep(js->pub.network_server, e->regJob.ns);
		}
		break;

	case EDG_WLL_EVENT_PBSTRANSFER:
		if(USABLE(res)) {
			switch(e->PBSTransfer.result) {
			case EDG_WLL_PBSTRANSFER_START:
				break;

			case EDG_WLL_PBSTRANSFER_OK:
				break;

			case EDG_WLL_PBSTRANSFER_REFUSED:
			case EDG_WLL_PBSTRANSFER_FAIL:
				break;

			default:
				break;

			}
		}
		if(USABLE_DATA(res)) {
			/* job going to another server */
			switch(e->PBSTransfer.result) {
			case EDG_WLL_PBSTRANSFER_OK:
				/* update job location */
				switch(e->PBSTransfer.destination) {
				case EDG_WLL_SOURCE_PBS_SERVER:
					break;
				default:
					/* where is it going? */
					break;

				}
				break;
			default:
				break;
			}
		}
		break;

	case EDG_WLL_EVENT_PBSACCEPTED:
		if(USABLE(res)) {
			switch(e->any.source) {
			case EDG_WLL_SOURCE_PBS_SERVER:
				/* accepted by server means job is submitted */
				js->pub.state = EDG_WLL_JOB_SUBMITTED;
				break;

			case EDG_WLL_SOURCE_PBS_SMOM:
			case EDG_WLL_SOURCE_PBS_MOM:
				/* accepted by MOM: job is going to run */
				js->pub.state = EDG_WLL_JOB_SCHEDULED;
				break;

			default:
				/* this would be weird */
				break;
			}
		}
		break;

	case EDG_WLL_EVENT_PBSREFUSED:
		break;

	case EDG_WLL_EVENT_PBSQUEUED:
		if (USABLE(res)) {
			js->pub.state = EDG_WLL_JOB_WAITING;
		}
		if (USABLE_DATA(res)) {
			if(e->any.source == EDG_WLL_SOURCE_PBS_SERVER) {
				/* queue */
				if (!js->pub.pbs_queue)
					js->pub.pbs_queue = strdup(e->PBSQueued.queue);
				assert(!strcmp(js->pub.pbs_queue, e->PBSQueued.queue));
				
				/* job owner */
				if(!js->pub.pbs_owner) 
					rep_cond(js->pub.pbs_owner, e->PBSQueued.owner);
				/* job_name */
				if(!js->pub.pbs_name)
					rep_cond(js->pub.pbs_name, e->PBSQueued.name);
			}
		}
		break;

	case EDG_WLL_EVENT_PBSMATCH:
		/* XXX - not used yet */
		if (USABLE(res)) {
			js->pub.state = EDG_WLL_JOB_READY;
		}
		if (USABLE_DATA(res)) {
			rep_cond(js->pub.pbs_dest_host,e->PBSMatch.dest_host);
		}
		break;

	case EDG_WLL_EVENT_PBSPENDING:
		/* XXX - not used yet */
		if (USABLE(res)) {
			js->pub.state = EDG_WLL_JOB_WAITING;
			js->pbs_reruning = 0;		// reset possible reruning flag
		}
		if (USABLE_DATA(res)) {
			rep_cond(js->pub.pbs_reason,e->PBSPending.reason);
		}
		break;

	case EDG_WLL_EVENT_PBSWAITING:
		break;

	case EDG_WLL_EVENT_PBSRUN:
		if (USABLE(res)) {
			switch (e->any.source) {
			case EDG_WLL_SOURCE_PBS_SERVER:
				js->pub.state = EDG_WLL_JOB_SCHEDULED;
				break;
			case EDG_WLL_SOURCE_PBS_MOM:
				js->pub.state = EDG_WLL_JOB_RUNNING;
				break;
			default:
				assert(0); // running event from strange source
				break;
			}
		}
		if (USABLE_DATA(res)) {
			/* session id */
			rep_cond(js->pub.pbs_scheduler, e->PBSRun.scheduler);
			rep_cond(js->pub.pbs_dest_host, e->PBSRun.dest_host);
			js->pub.pbs_pid = e->PBSRun.pid;
		}
		break;

	case EDG_WLL_EVENT_PBSRERUN:
		if (USABLE(res)) {
			switch (e->any.source) {
			case EDG_WLL_SOURCE_PBS_SERVER:
				js->pub.state = EDG_WLL_JOB_WAITING;
				break;
			case EDG_WLL_SOURCE_PBS_MOM:
				js->pub.state = EDG_WLL_JOB_WAITING;
				js->pbs_reruning = 1;
				break;
			default:
				assert(0); // running event from strande source
				break;
			}
		}
		if (USABLE_DATA(res)) {
			/* session id */
		}
		break;

	case EDG_WLL_EVENT_PBSABORT:
		break;

	case EDG_WLL_EVENT_PBSDONE:
		if (USABLE(res)) {
			switch (e->any.source) {
			case EDG_WLL_SOURCE_PBS_SERVER:
				js->pub.state = EDG_WLL_JOB_DONE;
				js->pub.done_code = EDG_WLL_STAT_OK;
				break;
			case EDG_WLL_SOURCE_PBS_MOM:
				if (!js->pbs_reruning) {
					js->pub.state = EDG_WLL_JOB_DONE;
					js->pub.done_code = EDG_WLL_STAT_OK;
				}
				break;
			default:
				assert(0); //done event from strange source
				break;
			}
		}
		if (USABLE_DATA(res)) {
			/* exit status */
			js->pub.pbs_exit_status =  e->PBSDone.exit_status;	
		}
		break;

	case EDG_WLL_EVENT_PBSRESOURCEUSAGE:
		if (USABLE(res)) {
			// signalize state done, done_code uknown
			js->pub.state = EDG_WLL_JOB_DONE;
		}
		if (USABLE_DATA(res)) {
			char *new_resource_usage;
			
			/*trio_asprintf(&new_resource_usage,"%s%s\t%s = %f [%s]",
				      (js->pub.pbs_resource_usage) ? js->pub.pbs_resource_usage : "",
				      (js->pub.pbs_resource_usage) ? "\n": "",
				      e->PBSResourceUsage.name,
				      e->PBSResourceUsage.quantity,
				      e->PBSResourceUsage.unit);
			*/
			if (js->pub.pbs_resource_usage) free(js->pub.pbs_resource_usage);
			js->pub.pbs_resource_usage = new_resource_usage;
		}
		break;

	case EDG_WLL_EVENT_PBSERROR:
		/* XXX - not used yet */
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

