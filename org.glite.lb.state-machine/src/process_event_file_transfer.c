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

int processEvent_FileTransfer(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{
	edg_wll_JobStatCode     old_state = js->pub.state;
	int			res = RET_OK;


/*	not used in FT
	if ((js->last_seqcode != NULL) &&
			(edg_wll_compare_pbs_seq(js->last_seqcode, e->any.seqcode) > 0) ) {
		res = RET_LATE;	
	}
*/
	switch (e->any.type) {
		case EDG_WLL_EVENT_REGJOB:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_SUBMITTED;
			}
			if (USABLE_DATA(res)) {
				edg_wlc_JobIdFree(js->pub.parent_job);
				edg_wlc_JobIdDup(e->regJob.parent, &js->pub.parent_job);
			}
			break;
		case EDG_WLL_EVENT_FILETRANSFERREGISTER:
			if (USABLE(res)) {
				;
			}
			if (USABLE_DATA(res)) {
				rep(js->pub.ft_src, e->fileTransferRegister.src);
				rep(js->pub.ft_dest, e->fileTransferRegister.dest);
			}
			break;
		case EDG_WLL_EVENT_FILETRANSFER:
			if (USABLE(res)) {
				switch (e->fileTransfer.result) {
					case EDG_WLL_FILETRANSFER_START:
						js->pub.state = EDG_WLL_JOB_RUNNING;
						break;
					case EDG_WLL_FILETRANSFER_OK:
						js->pub.state = EDG_WLL_JOB_DONE;
						js->pub.done_code = EDG_WLL_STAT_OK;
						break;
					case EDG_WLL_FILETRANSFER_FAIL:
						js->pub.state = EDG_WLL_JOB_DONE;
						js->pub.done_code = EDG_WLL_STAT_FAILED;
						rep(js->pub.failure_reasons, e->fileTransfer.reason);
						break;
					default:
						break;
				}
			}
			if (USABLE_DATA(res)) {
				;
			}
			break;
		case EDG_WLL_EVENT_ABORT:
			if (USABLE(res)) {
				js->pub.state = EDG_WLL_JOB_ABORTED;
				js->pub.remove_from_proxy = 1;
				rep(js->pub.reason, e->abort.reason);
				rep(js->pub.location, "none");
			}
			break;
		case EDG_WLL_EVENT_SANDBOX:
			if (USABLE_DATA(res)) {
				if (e->sandbox.sandbox_type == EDG_WLL_SANDBOX_INPUT)
					js->pub.ft_sandbox_type = EDG_WLL_STAT_INPUT;

				if (e->sandbox.sandbox_type == EDG_WLL_SANDBOX_OUTPUT)
					js->pub.ft_sandbox_type = EDG_WLL_STAT_OUTPUT;

				if (e->sandbox.compute_job) {
					edg_wlc_JobIdFree(js->pub.ft_compute_job);
					edg_wlc_JobIdParse(e->sandbox.compute_job,&js->pub.ft_compute_job);
				}
#if 0
				else if (e->sandbox.transfer_job) { /* mutually exclusive with compute_job */
					edg_wlc_JobIdFree(js->pub.parent_job);
					edg_wlc_JobIdParse(e->sandbox.transfer_job, &js->pub.parent_job);
				}
#endif
			}
			break;
		case EDG_WLL_EVENT_RESUBMISSION:
                        if (USABLE(res)) {
                                if (e->resubmission.result == EDG_WLL_RESUBMISSION_WONTRESUB) {
                                        rep(js->pub.reason, e->resubmission.reason);
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
	if (! js->pub.location) js->pub.location = strdup("this is FILE TRANSFER");


	return RET_OK;
}

