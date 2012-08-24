#ident "$Header:"
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

/*static int compare_timestamps(struct timeval a, struct timeval b)
{
	if ( (a.tv_sec > b.tv_sec) || 
		((a.tv_sec == b.tv_sec) && (a.tv_usec > b.tv_usec)) ) return 1;
	if ( (a.tv_sec < b.tv_sec) ||
                ((a.tv_sec == b.tv_sec) && (a.tv_usec < b.tv_usec)) ) return -1;
	return 0;
}*/


// XXX move this defines into some common place to be reusable
#define USABLE(res) ((res) == RET_OK)
#define USABLE_DATA(res) (1)
#define rep(a,b) { free(a); a = (b == NULL) ? NULL : strdup(b); }
#define rep_cond(a,b) { if (b) { free(a); a = strdup(b); } }


int processEvent_VirtualMachine(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{
	edg_wll_JobStatCode     old_state = js->pub.state;
	int			res = RET_OK;


	switch (e->any.type) {
		case EDG_WLL_EVENT_REGJOB:
			if (USABLE(res)) {
				js->pub.vm_state = EDG_WLL_STAT_VM_PENDING;
				js->pub.state = EDG_WLL_JOB_SUBMITTED;
			}
			break;
		case EDG_WLL_EVENT_VMCREATE:
			if (USABLE_DATA(res)) {
				rep_cond(js->pub.vm_require, e->vMCreate.require);
				rep_cond(js->pub.vm_image, e->vMCreate.image);
				rep_cond(js->pub.vm_id, e->vMCreate.id);
				rep_cond(js->pub.vm_name, e->vMCreate.name);
				rep_cond(js->pub.owner, e->vMCreate.owner);
				rep_cond(js->pub.vm_hostname, e->vMCreate.hostname);
				rep_cond(js->pub.destination, e->vMCreate.hostname);
			}
			break;
		case EDG_WLL_EVENT_VMHOST:
			if (USABLE_DATA(res)) {
				rep_cond(js->pub.vm_phy_hostname, e->vMHost.hostname);
				//XXX transfer to prolog/boot state?
			}
			break;
		case EDG_WLL_EVENT_VMRUNNING:
			if (USABLE(res)) {
				switch( e->vMRunning.vm_source){
				case EDG_WLL_VMRUNNING_CM:
				case EDG_WLL_VMRUNNING_VMM:
					js->pub.vm_state = EDG_WLL_STAT_VM_RUNNING;
					break;
				case EDG_WLL_VMRUNNING_MACHINE:
					js->pub.vm_state = EDG_WLL_STAT_VM_REALLY_RUNNING;
                                        break;
				default:
					break; 
				}
				js->pub.state = EDG_WLL_JOB_RUNNING;
			}
			break;
		case EDG_WLL_EVENT_VMSHUTDOWN:
                        if (USABLE(res)) {
				switch (e->vMShutdown.vm_source){
				case EDG_WLL_VMSHUTDOWN_CM:
					js->pub.vm_state = EDG_WLL_STAT_VM_SHUTDOWN;
					break;
				case EDG_WLL_VMSHUTDOWN_VMM:
					js->pub.vm_system_halting = 1;
					break;
				case EDG_WLL_VMSHUTDOWN_MACHINE:
					js->pub.vm_system_halting = 1;
					if (js->pub.vm_state == EDG_WLL_STAT_VM_REALLY_RUNNING)
						js->pub.vm_state = EDG_WLL_STAT_VM_RUNNING;
					break;
				}
                        }
			if (USABLE_DATA(res))
				if (e->vMDone.usage)
					rep_cond(js->pub.vm_usage, e->vMDone.usage);
                        break;
		case EDG_WLL_EVENT_VMSTOP:
                        if (USABLE(res)) {
                                js->pub.vm_state = EDG_WLL_STAT_VM_STOPPED;
                        }
                        break;
		case EDG_WLL_EVENT_VMRESUME:
                        if (USABLE(res)) {
                                js->pub.vm_state = EDG_WLL_STAT_VM_PENDING;
				js->pub.state = EDG_WLL_JOB_WAITING;
				js->pub.vm_system_halting = 0;
				//XXX clear hostname here?
                        }
                        break;
		case EDG_WLL_EVENT_VMDONE:
                        if (USABLE(res)) {
				switch (e->vMDone.status_code){
					case EDG_WLL_VMDONE_OK:
					case EDG_WLL_VMDONE_DELETE:
						js->pub.vm_state = EDG_WLL_STAT_VM_DONE;
						js->pub.state = EDG_WLL_JOB_DONE;
						break;
					case EDG_WLL_VMDONE_FAILURE:
						js->pub.vm_state = EDG_WLL_STAT_VM_FAILURE;
						js->pub.state = EDG_WLL_JOB_DONE
;
						break;
					case EDG_WLL_VMDONE_STATUS_CODE_UNDEFINED:
						break;
				}
				if (USABLE_DATA(res))
					rep_cond(js->pub.vm_usage, e->vMDone.usage);
                        }
                        break;
		default:
			break;
	}

	if (USABLE(res)) {
		//rep(js->last_seqcode, e->any.seqcode);

		js->pub.lastUpdateTime = e->any.timestamp;
		if (old_state != js->pub.state) {
			js->pub.stateEnterTime = js->pub.lastUpdateTime;
			js->pub.stateEnterTimes[1 + js->pub.state]
				= (int)js->pub.lastUpdateTime.tv_sec;
		}
	}
	if (! js->pub.location) js->pub.location = strdup("this is VIRTUAL MACHINE");

	return RET_OK;
}

