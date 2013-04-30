#ident "$Header: /cvs/glite/org.glite.lb.state-machine/src/process_event.c,v 1.38 2013/02/19 11:53:53 jfilipov Exp $"
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
#include <cclassad.h> 

#include "glite/lb/context-int.h"

#include "glite/lbu/trio.h"

#include "intjobstat.h"
#include "seqcode_aux.h"
#include "process_event.h"


/* TBD: share in whole logging or workload */
#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif

static int processEvent_glite(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring);
int processEvent_PBS(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring);
int processEvent_Condor(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring);
int processEvent_Cream(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring);
int processData_Cream(intJobStat *js, edg_wll_Event *e);
int processEvent_FileTransfer(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring);
int processEvent_FileTransferCollection(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring);
int processEvent_VirtualMachine(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring);

int filterEvent_PBS(intJobStat *js, edg_wll_Event *e, int ev_seq, edg_wll_Event *prev_ev, int prev_seq);

int add_stringlist(char ***lptr, const char *new_item);

int processEvent(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{
	if (js->pub.jobtype == -1 && e->type == EDG_WLL_EVENT_REGJOB)
		switch (e->regJob.jobtype) {
			case EDG_WLL_REGJOB_SIMPLE:
				js->pub.jobtype = EDG_WLL_STAT_SIMPLE;
				break;
			case EDG_WLL_REGJOB_DAG: 
			case EDG_WLL_REGJOB_PARTITIONABLE:
			case EDG_WLL_REGJOB_PARTITIONED:
				js->pub.jobtype = EDG_WLL_STAT_DAG;
				break;
			case EDG_WLL_REGJOB_COLLECTION:
				js->pub.jobtype = EDG_WLL_STAT_COLLECTION;
				break;
			case EDG_WLL_REGJOB_PBS:
				js->pub.jobtype = EDG_WLL_STAT_PBS;
				break;
			case EDG_WLL_REGJOB_CONDOR:
				js->pub.jobtype = EDG_WLL_STAT_CONDOR;
				break;
			case EDG_WLL_REGJOB_CREAM:
				js->pub.jobtype = EDG_WLL_STAT_CREAM;
				break;
			case EDG_WLL_REGJOB_FILE_TRANSFER:
				js->pub.jobtype = EDG_WLL_STAT_FILE_TRANSFER;
				break;
			case EDG_WLL_REGJOB_FILE_TRANSFER_COLLECTION:
				js->pub.jobtype = EDG_WLL_STAT_FILE_TRANSFER_COLLECTION;
				break;
			case EDG_WLL_REGJOB_VIRTUAL_MACHINE:
				js->pub.jobtype = EDG_WLL_STAT_VIRTUAL_MACHINE;
				break;
			default:
				trio_asprintf(errstring,"unknown job type %d in registration",e->regJob.jobtype);
				return RET_FAIL;
	}

	switch (js->pub.jobtype) {
		case EDG_WLL_STAT_SIMPLE:
		case EDG_WLL_STAT_DAG:
		case EDG_WLL_STAT_COLLECTION:
			return processEvent_glite(js,e,ev_seq,strict,errstring);
		case EDG_WLL_STAT_PBS: 
			return processEvent_PBS(js,e,ev_seq,strict,errstring);
		case EDG_WLL_STAT_CONDOR: 
			return processEvent_Condor(js,e,ev_seq,strict,errstring);
		case EDG_WLL_STAT_CREAM: 
			return processEvent_Cream(js,e,ev_seq,strict,errstring);
		case EDG_WLL_STAT_FILE_TRANSFER:
			return processEvent_FileTransfer(js,e,ev_seq,strict,errstring);
		case EDG_WLL_STAT_FILE_TRANSFER_COLLECTION:
			return processEvent_FileTransferCollection(js,e,ev_seq,strict,errstring);
		case EDG_WLL_STAT_VIRTUAL_MACHINE:
			return processEvent_VirtualMachine(js,e,ev_seq,strict,errstring);
		case -1: return RET_UNREG;
		default: 
			trio_asprintf(errstring,"undefined job type %d",js->pub.jobtype);
			return RET_FAIL;
	}
}

#define rep(a,b) { free(a); a = (b == NULL) ? NULL : strdup(b); }
#define rep_null(a) { free(a); a = NULL; }
#define rep_cond(a,b) { if (b) { free(a); a = strdup(b); } }

static void free_stringlist(char ***lptr)
{
	char **itptr;
	int i;

	if (*lptr) {
		for (i = 0, itptr = *lptr; itptr[i] != NULL; i++) 
			free(itptr[i]);
		free(itptr);
		*lptr = NULL;
	}
}



static void update_branch_state(char *b, char *d, char *c, char *j, branch_state **bs)
{
	int 	i = 0, branch;


	if (!b) 
		return;
	else 
		branch = component_seqcode(b, EDG_WLL_SOURCE_WORKLOAD_MANAGER);

	if (*bs != NULL) {
		while ((*bs)[i].branch) {
			if (branch == (*bs)[i].branch) {
				if (d) rep((*bs)[i].destination, d);	
				if (c) rep((*bs)[i].ce_node, c);	
				if (j) rep((*bs)[i].jdl, j);
				
				return;
			}	
			i++;
		}
	}

	*bs = (branch_state *) realloc(*bs, (i+2)*sizeof(branch_state));
	memset(&((*bs)[i]), 0, 2*sizeof(branch_state));

	(*bs)[i].branch = branch;
	rep((*bs)[i].destination, d);
	rep((*bs)[i].ce_node, c);
	rep((*bs)[i].jdl, j);
}


static void free_branch_state(branch_state **bs)
{
	int i = 0;
	
	if (*bs == NULL) return;

	while ((*bs)[i].branch) {
		free((*bs)[i].destination); 
		free((*bs)[i].ce_node);
		free((*bs)[i].jdl);
		i++;
	}
	free(*bs);
	*bs = NULL;
}

static int compare_branch_states(const void *a, const void *b)
{
	branch_state *c = (branch_state *) a;
	branch_state *d = (branch_state *) b;

	if (c->branch < d->branch) return -1;
	if (c->branch == d->branch) return 0;
	/* avoid warning: if (c->branch > d->branch) */ return 1;
}

static void load_branch_state(intJobStat *js)
{
	int	i, j, branch;


	if ( (!js->branch_tag_seqcode) || (!js->branch_states) )  return;

	branch = component_seqcode(js->branch_tag_seqcode, EDG_WLL_SOURCE_WORKLOAD_MANAGER);
	
	// count elements
	i = 0;
	while (js->branch_states[i].branch) i++;
	
	// sort them
	qsort(js->branch_states, (size_t) i, sizeof(branch_state),
		compare_branch_states);
	
	// find row corresponding to ReallyRunning WM seq.code (aka branch)	
	i = 0;
	while (js->branch_states[i].branch) {
		if (js->branch_states[i].branch == branch) break;
		i++;
	}
	
	// copy this and two before branches data to final state
	// (each field - dest,ce,jdl - comes from different event)
	// (and these events have most likely different WM seq.codes)
	// (even belonging into one logical branch)
	// (the newer the more important - so i-th element is copied as last)
	// (and may overwrite data from previous elements)
	for (j = i - 2; j <= i; j++) {
		if (j >= 0) {
			if (js->branch_states[j].destination)
				rep(js->pub.destination, js->branch_states[j].destination);
			if (js->branch_states[j].ce_node)
				rep(js->pub.ce_node, js->branch_states[j].ce_node);
			if (js->branch_states[j].jdl)
				rep(js->pub.matched_jdl, js->branch_states[j].jdl);
		}
	}
}

// clear branches (deep resub. or abort)
static void reset_branch(intJobStat *js, edg_wll_Event *e) 
{
	js->resubmit_type = EDG_WLL_RESUBMISSION_WILLRESUB;
	free_stringlist(&js->pub.possible_destinations);
	free_stringlist(&js->pub.possible_ce_nodes);
	free_branch_state(&js->branch_states);
	js->pub.payload_running = 0;
	rep_null(js->branch_tag_seqcode);
	rep(js->deep_resubmit_seqcode, e->any.seqcode);
}

static char* location_string(const char *source, const char *host, const char *instance)
{
	char *ret;
	trio_asprintf(&ret, "%s/%s/%s", source, host, instance);
	return ret;
}

/* is seq. number of 'es' before WMS higher then 'js' */
static int after_enter_wm(const char *es,const char *js)
{
	return ((component_seqcode(es,EDG_WLL_SOURCE_NETWORK_SERVER) >
		component_seqcode(js,EDG_WLL_SOURCE_NETWORK_SERVER))
		||
		(component_seqcode(es,EDG_WLL_SOURCE_USER_INTERFACE) >
		component_seqcode(js,EDG_WLL_SOURCE_USER_INTERFACE)));
}


static int badEvent(intJobStat *js UNUSED_VAR, edg_wll_Event *e, int ev_seq UNUSED_VAR)
{
	char *str;

	str = edg_wll_EventToString(e->any.type);
	fprintf(stderr, "edg_wll_JobStatus:  bad event: type %d (%s)\n",
		e->any.type, (str == NULL) ? "unknown" : str);
	free(str);
	return RET_FATAL;
}


// (?) || (0 && 1)  =>  true if (res == RET_OK)
#define USABLE(res,strict) ((res) == RET_OK || ( (res) == RET_SOON && !strict))

// (?) || (1 && 1)  =>  always true
#define USABLE_DATA(res,strict) ((res) == RET_OK || ( (res) != RET_FATAL && !strict))

#define USABLE_BRANCH(fine_res) ((fine_res) != RET_TOOOLD && (fine_res) != RET_BADBRANCH)
#define LRMS_STATE(state) ((state) == EDG_WLL_JOB_RUNNING || (state) == EDG_WLL_JOB_DONE)
#define PARSABLE_SEQCODE(code) ((code) && component_seqcode((code),0) >= 0)

static int processEvent_glite(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{
	edg_wll_JobStatCode	old_state = js->pub.state;
	edg_wll_JobStatCode	new_state = EDG_WLL_JOB_UNKNOWN;
	int			res = RET_OK,
				fine_res = RET_OK;
				
	int	lm_favour_lrms = 0;
	int	ignore_seq_code = 0;

	// Aborted may not be terminal state for collection in some cases
	// i.e. if some Done/failed subjob is resubmitted
	if ( (old_state == EDG_WLL_JOB_ABORTED && e->any.type != EDG_WLL_EVENT_COLLECTIONSTATE) ||
		old_state == EDG_WLL_JOB_CANCELLED ||
		old_state == EDG_WLL_JOB_CLEARED) {
		res = RET_LATE;
	}

/* new event coming from NS or UI => forget about any resubmission loops */
	if (e->type != EDG_WLL_EVENT_CANCEL && 
		js->last_seqcode &&
		after_enter_wm(e->any.seqcode,js->last_seqcode))
	{
		rep_null(js->branch_tag_seqcode); 
		rep_null(js->deep_resubmit_seqcode); 
		rep_null(js->last_branch_seqcode); 
	}

	if (js->deep_resubmit_seqcode && 
			before_deep_resubmission(e->any.seqcode, js->deep_resubmit_seqcode)) {
		res = RET_LATE;
		fine_res = RET_TOOOLD;
	}
	else if (js->branch_tag_seqcode) {		// ReallyRunning ev. arrived
		if (same_branch(e->any.seqcode, js->branch_tag_seqcode)) {
			if ((js->last_branch_seqcode != NULL) &&
				edg_wll_compare_seq(e->any.seqcode, js->last_branch_seqcode) < 0) {
				res = RET_LATE;
			}
			fine_res = RET_GOODBRANCH;
		}
		else {
			res = RET_LATE;
			fine_res = RET_BADBRANCH;
		}
	}
	else if ((js->last_seqcode != NULL) &&
			edg_wll_compare_seq(e->any.seqcode, js->last_seqcode) < 0) {
		res = RET_LATE;
	}

	if (e->any.source == EDG_WLL_SOURCE_CREAM_EXECUTOR || e->any.source == EDG_WLL_SOURCE_CREAM_INTERFACE){
                processData_Cream(js, e);
        }

	switch (e->any.type) {
		case EDG_WLL_EVENT_TRANSFER:
			if (e->transfer.result == EDG_WLL_TRANSFER_OK) {
				switch (e->transfer.source) {
					case EDG_WLL_SOURCE_USER_INTERFACE:
						new_state = EDG_WLL_JOB_WAITING; break;
					case EDG_WLL_SOURCE_JOB_SUBMISSION:
						/* if (LRMS_STATE(old_state)) res = RET_LATE; */
						new_state = EDG_WLL_JOB_READY; break;
					case EDG_WLL_SOURCE_LOG_MONITOR:
						if (LRMS_STATE(old_state)) {
							js->pub.stateEnterTimes[1 + EDG_WLL_JOB_SCHEDULED] =
								e->any.timestamp.tv_sec;
							res = RET_LATE;
						}
						new_state = EDG_WLL_JOB_SCHEDULED;
						lm_favour_lrms = 1;
						break;
					default:
						goto bad_event; break;
				}
			} else if (e->transfer.result == EDG_WLL_TRANSFER_FAIL) {
				/* transfer failed */ 
				switch (e->transfer.source) {
					case EDG_WLL_SOURCE_USER_INTERFACE:
						new_state = EDG_WLL_JOB_SUBMITTED; break;
					case EDG_WLL_SOURCE_JOB_SUBMISSION:
						if (LRMS_STATE(old_state)) res = RET_LATE;
						new_state = EDG_WLL_JOB_READY; break;
					case EDG_WLL_SOURCE_LOG_MONITOR:
						if (LRMS_STATE(old_state)) res = RET_LATE;
						new_state = EDG_WLL_JOB_READY; break;
					default:
						goto bad_event; break;
				}
			} else {
				/* e->transfer.result == EDG_WLL_TRANSFER_START */
				res = RET_IGNORE;
			}
			if (USABLE(res, strict)) {
				js->pub.state = new_state;
				rep(js->pub.reason, e->transfer.reason);

				free(js->pub.location);
				if (e->transfer.result == EDG_WLL_TRANSFER_OK) {
					js->pub.location = location_string(
						edg_wll_SourceToString(e->transfer.destination),
						e->transfer.dest_host,
						e->transfer.dest_instance);
				} else {
					js->pub.location = location_string(
						edg_wll_SourceToString(e->transfer.source),
						e->transfer.host,
						e->transfer.src_instance);
				}
			}
			if (USABLE_DATA(res, strict)) {
				switch (e->transfer.source) {
					case EDG_WLL_SOURCE_USER_INTERFACE:
						if (!js->pub.jdl) {
							rep(js->pub.jdl, e->transfer.job);
						}
						break;
					case EDG_WLL_SOURCE_JOB_SUBMISSION:
						rep(js->pub.condor_jdl, e->transfer.job); break;
					case EDG_WLL_SOURCE_LOG_MONITOR:
						rep(js->pub.rsl, e->transfer.job); break;
					default:
						goto bad_event; break;
						
				}
			}
			break;
		case EDG_WLL_EVENT_ACCEPTED:
			switch (e->accepted.source) {
				case EDG_WLL_SOURCE_NETWORK_SERVER:
					new_state = EDG_WLL_JOB_WAITING; break;
				case EDG_WLL_SOURCE_LOG_MONITOR:
					if (LRMS_STATE(old_state)) res = RET_LATE;
					new_state = EDG_WLL_JOB_READY; 
					lm_favour_lrms = 1;
					break;
				case EDG_WLL_SOURCE_LRMS:
					new_state = EDG_WLL_JOB_SCHEDULED; break;
				default:
					goto bad_event; break;
			}
			if (USABLE(res, strict)) {
				js->pub.state = new_state;
				free(js->pub.location);
				js->pub.location = location_string(
						edg_wll_SourceToString(e->accepted.source),
						e->accepted.host,
						e->accepted.src_instance);
			}
			if (USABLE_DATA(res, strict)) {
				switch (e->accepted.source) {
					case EDG_WLL_SOURCE_NETWORK_SERVER:
						rep(js->pub.ui_host, e->accepted.from_host);
						break; /* no WM id */
					case EDG_WLL_SOURCE_LOG_MONITOR:
						rep(js->pub.condorId, e->accepted.local_jobid); break;
					case EDG_WLL_SOURCE_LRMS:
						/* XXX localId */
						rep(js->pub.globusId, e->accepted.local_jobid); break;
					default:
						goto bad_event; break;
				}
			}
			break;
		case EDG_WLL_EVENT_REFUSED:
			switch (e->refused.source) {
				case EDG_WLL_SOURCE_NETWORK_SERVER:
					new_state = EDG_WLL_JOB_SUBMITTED; break;
				case EDG_WLL_SOURCE_LOG_MONITOR:
					new_state = EDG_WLL_JOB_READY; break;
				case EDG_WLL_SOURCE_LRMS:
					new_state = EDG_WLL_JOB_READY; break;
				default:
					goto bad_event; break;
			}
			if (USABLE(res, strict)) {
				js->pub.state = new_state;
				rep(js->pub.reason, e->refused.reason);

				free(js->pub.location);
				js->pub.location = location_string(
						edg_wll_SourceToString(e->refused.from),
						e->refused.from_host,
						e->refused.from_instance);
			}
			break;
		case EDG_WLL_EVENT_ENQUEUED:
			if (e->enQueued.result == EDG_WLL_ENQUEUED_OK) {
				switch (e->enQueued.source) {
					case EDG_WLL_SOURCE_NETWORK_SERVER:
						new_state = EDG_WLL_JOB_WAITING; break;
					case EDG_WLL_SOURCE_WORKLOAD_MANAGER:
						if (LRMS_STATE(old_state)) res = RET_LATE;
						update_branch_state(e->any.seqcode, NULL,
								NULL, e->enQueued.job, &js->branch_states);
						new_state = EDG_WLL_JOB_READY; break;
					case EDG_WLL_SOURCE_LOG_MONITOR:
						new_state = EDG_WLL_JOB_WAITING; break;
					default:
						goto bad_event; break;
				}
			} else if (e->enQueued.result == EDG_WLL_ENQUEUED_FAIL) {
				switch (e->enQueued.source) {
					case EDG_WLL_SOURCE_NETWORK_SERVER:
						new_state = EDG_WLL_JOB_WAITING; break;
					case EDG_WLL_SOURCE_WORKLOAD_MANAGER:
						new_state = EDG_WLL_JOB_WAITING; break;
					case EDG_WLL_SOURCE_LOG_MONITOR:
						new_state = old_state; break;
					default:
						goto bad_event; break;
				}
			} else { 
				/* e->enQueued.result == EDG_WLL_ENQUEUED_START */
				res = RET_IGNORE;
			}
			if (USABLE(res, strict)) {
				js->pub.state = new_state;
				rep(js->pub.reason, e->enQueued.reason);

				free(js->pub.location);
				if (e->enQueued.result == EDG_WLL_ENQUEUED_OK) {
					js->pub.location = location_string(
						e->enQueued.queue,
						e->enQueued.host,
						e->enQueued.src_instance);
					if (e->enQueued.source == EDG_WLL_SOURCE_LOG_MONITOR)
						js->pub.resubmitted = 1;
				} else {
					js->pub.location = location_string(
						edg_wll_SourceToString(e->enQueued.source),
						e->enQueued.host,
						e->enQueued.src_instance);
				}
			}
			if (USABLE_DATA(res, strict)) {
				switch (e->enQueued.source) {
					case EDG_WLL_SOURCE_NETWORK_SERVER:
						if (!js->pub.jdl || e->enQueued.result == EDG_WLL_ENQUEUED_OK) {
							rep(js->pub.jdl, e->enQueued.job);
						}
						break;
					case EDG_WLL_SOURCE_WORKLOAD_MANAGER:
						if (USABLE_BRANCH(res)) {
							rep(js->pub.matched_jdl, e->enQueued.job);
						}
						break;
					case EDG_WLL_SOURCE_LOG_MONITOR:
						/* no interim JDL here */
						break;
					default:
						goto bad_event; break;
				}
			}
			break;
		case EDG_WLL_EVENT_DEQUEUED:
			switch (e->deQueued.source) {
				case EDG_WLL_SOURCE_WORKLOAD_MANAGER:
					new_state = EDG_WLL_JOB_WAITING; break;
				case EDG_WLL_SOURCE_JOB_SUBMISSION:
					if (LRMS_STATE(old_state)) res = RET_LATE;
					new_state = EDG_WLL_JOB_READY; break;
				default:
					goto bad_event; break;
			}
			if (USABLE(res, strict)) {
				js->pub.state = new_state;
				free(js->pub.location);
				js->pub.location = location_string(
						edg_wll_SourceToString(e->deQueued.source),
						e->deQueued.host,
						e->deQueued.src_instance);
			}
			if (USABLE_DATA(res, strict)) {
				/* no WM/JSS local jobid */
			}
			break;
		case EDG_WLL_EVENT_HELPERCALL:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_WAITING;
				free(js->pub.location);
				js->pub.location = location_string(
						e->helperCall.helper_name,
						e->helperCall.host,
						e->helperCall.src_instance);
				/* roles and params used only for debugging */
			}
			break;
		case EDG_WLL_EVENT_HELPERRETURN:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_WAITING;
				free(js->pub.location);
				js->pub.location = location_string(
						edg_wll_SourceToString(EDG_WLL_SOURCE_WORKLOAD_MANAGER),
						e->helperReturn.host,
						e->helperReturn.src_instance);
				/* roles and retvals used only for debugging */
			}
			break;
		case EDG_WLL_EVENT_RUNNING:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_RUNNING;
				js->pub.jw_status = EDG_WLL_STAT_WRAPPER_RUNNING;
				free(js->pub.location);
				js->pub.location = location_string(
						edg_wll_SourceToString(EDG_WLL_SOURCE_LRMS),
						"worknode",
						e->running.node);
			}
			if (USABLE_DATA(res, strict)) {
				if (USABLE_BRANCH(fine_res)) {
					rep(js->pub.ce_node, e->running.node);
				}
				/* why? if (e->any.source == EDG_WLL_SOURCE_LOG_MONITOR) { */
					if (e->running.node) {
						update_branch_state(e->any.seqcode, NULL,
							e->running.node, NULL, &js->branch_states);
						add_stringlist(&js->pub.possible_ce_nodes,
							e->running.node);
					}
				/* } */
			}
			break;
		case EDG_WLL_EVENT_REALLYRUNNING:
			/* consistence check -- should not receive two contradicting ReallyRunning's within single
			   deep resub cycle */
			if (fine_res == RET_BADBRANCH) {
				char *jobid_s;

				jobid_s = glite_jobid_unparse(e->any.jobId);
				syslog(LOG_ERR,"ReallyRunning on bad branch %s (%s)",
						e->any.source == EDG_WLL_SOURCE_LOG_MONITOR ? e->reallyRunning.wn_seq : e->any.seqcode,
						jobid_s);
				free(jobid_s);
				break;
			}
			/* select the branch unless TOOOLD, i.e. before deep resubmission */
			if (!(res == RET_LATE && fine_res == RET_TOOOLD)) {
				if (e->any.source == EDG_WLL_SOURCE_LRMS) {
					rep(js->branch_tag_seqcode, e->any.seqcode);
					if (res == RET_OK) {
						rep(js->last_branch_seqcode, e->any.seqcode);
						js->pub.state = EDG_WLL_JOB_RUNNING;
					}
				}
				if (e->any.source == EDG_WLL_SOURCE_LOG_MONITOR) {
					if (!js->branch_tag_seqcode) {
						if (PARSABLE_SEQCODE(e->reallyRunning.wn_seq)) { 
							rep(js->branch_tag_seqcode, e->reallyRunning.wn_seq);
						} else
							goto bad_event;
					}
					if (!js->last_branch_seqcode) {
						if (PARSABLE_SEQCODE(e->reallyRunning.wn_seq)) {
							if (res == RET_OK) {
								rep(js->last_branch_seqcode, e->reallyRunning.wn_seq);
								js->pub.state = EDG_WLL_JOB_RUNNING;
							}
						} else
							goto bad_event;
					}
				}

				/* XXX: best effort -- if we are lucky, ReallyRunning is on the last shallow cycle,
				   	so we take in account events processed so far */
				if (res == RET_LATE && !js->last_branch_seqcode) {
					if (same_branch(js->last_seqcode,js->branch_tag_seqcode))
						rep(js->last_branch_seqcode,js->last_seqcode);
				}

				js->pub.jw_status = EDG_WLL_STAT_PAYLOAD_RUNNING;
				js->pub.payload_running = 1;
				load_branch_state(js);
			}
#if 0
			if (USABLE_DATA(res, strict)) {
				js->pub.state = EDG_WLL_JOB_RUNNING;
				free(js->pub.location);
				js->pub.location = location_string(
						edg_wll_SourceToString(EDG_WLL_SOURCE_LRMS),
						"worknode",
						e->running.node);
				js->pub.payload_running = 1;
				if (e->any.source == EDG_WLL_SOURCE_LRMS) {
					rep(js->branch_tag_seqcode, e->any.seqcode);
					rep(js->last_branch_seqcode, e->any.seqcode);
				}
				if (e->any.source == EDG_WLL_SOURCE_LOG_MONITOR) {
					if (!js->branch_tag_seqcode) {
						if (PARSABLE_SEQCODE(e->reallyRunning.wn_seq)) { 
							rep(js->branch_tag_seqcode, e->reallyRunning.wn_seq);
						} else
							goto bad_event;
					}
					if (!js->last_branch_seqcode) {
						if (PARSABLE_SEQCODE(e->reallyRunning.wn_seq)) {
							rep(js->last_branch_seqcode, e->reallyRunning.wn_seq);
						} else
							goto bad_event;
					}
				}
				load_branch_state(js);
			}
#endif
			break;
		case EDG_WLL_EVENT_SUSPEND:
			if (USABLE(res, strict)) {
				if (js->pub.state == EDG_WLL_JOB_RUNNING) {
					js->pub.suspended = 1;
					rep(js->pub.suspend_reason, e->suspend.reason);
				}
			}
			break;
		case EDG_WLL_EVENT_RESUME:
			if (USABLE(res, strict)) {
				if (js->pub.state == EDG_WLL_JOB_RUNNING) {
					js->pub.suspended = 0;
					rep(js->pub.suspend_reason, e->resume.reason);
				}
			}
			break;
		case EDG_WLL_EVENT_RESUBMISSION:
			if (USABLE(res, strict)) {
				if (e->resubmission.result == EDG_WLL_RESUBMISSION_WONTRESUB) {
					rep(js->pub.reason, e->resubmission.reason);
				}
			}
			if (USABLE_DATA(res, strict)) {
				if (e->resubmission.result == EDG_WLL_RESUBMISSION_WONTRESUB) {
					js->resubmit_type = EDG_WLL_RESUBMISSION_WONTRESUB;
				}
				else 
				if (e->resubmission.result == EDG_WLL_RESUBMISSION_WILLRESUB &&
						e->any.source == EDG_WLL_SOURCE_WORKLOAD_MANAGER) {
					reset_branch(js, e);
				}
				else
				if (e->resubmission.result == EDG_WLL_RESUBMISSION_SHALLOW) {
					js->resubmit_type = EDG_WLL_RESUBMISSION_SHALLOW;
					// deep resubmit stays forever deadline for events
					// rep(js->deep_resubmit_seqcode, NULL);
				}
			}
			break;
		case EDG_WLL_EVENT_DONE:
			if (e->any.source == EDG_WLL_SOURCE_LRMS) {
				/* Done from JobWrapper is not sufficient for transition
				 * to DONE state according its current definition */
				if (USABLE(res, strict)) {
					js->pub.jw_status = EDG_WLL_STAT_DONE;
					js->pub.payload_running = 0;
				}
				break;
			}
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_DONE;
				js->pub.jw_status = EDG_WLL_STAT_DONE;
				rep(js->pub.reason, e->done.reason);
				if (fine_res == RET_GOODBRANCH) {
					js->pub.payload_running = 0;
				}
				switch (e->done.status_code) {
					case EDG_WLL_DONE_CANCELLED:
						js->pub.state = EDG_WLL_JOB_CANCELLED;
					case EDG_WLL_DONE_OK:
						rep(js->pub.location, "none");
						break;
					case EDG_WLL_DONE_FAILED:
						if (js->pub.failure_reasons) {
							char *glued_reasons;

							asprintf(&glued_reasons,"%s\n%s [%s]", 
								js->pub.failure_reasons, e->done.reason, js->pub.destination);
							rep(js->pub.failure_reasons, glued_reasons)
						}
						else {
							asprintf(&(js->pub.failure_reasons),"%s [%s]",
								e->done.reason, js->pub.destination);
						}
						// fall through
					default:
						free(js->pub.location);
						js->pub.location = location_string(
							edg_wll_SourceToString(e->done.source),
							e->done.host,
							e->done.src_instance);
				}
			}
			if (USABLE_DATA(res, strict)) {
				switch (e->done.status_code) {
					case EDG_WLL_DONE_OK:
						js->pub.exit_code = e->done.exit_code;
						js->pub.done_code = EDG_WLL_STAT_OK; break;
					case EDG_WLL_DONE_CANCELLED:
						js->pub.exit_code = 0;
						js->pub.done_code = EDG_WLL_STAT_CANCELLED; break;
					case EDG_WLL_DONE_FAILED:
						if (!USABLE(res, strict)) {
							// record reason, destination could have changed already
							if (js->pub.failure_reasons) {
								char *glued_reasons;
	
								asprintf(&glued_reasons,"%s\n%s [delayed event, destination unreliable]", 
									js->pub.failure_reasons, e->done.reason);
								rep(js->pub.failure_reasons, glued_reasons)
							}
							else {
								asprintf(&(js->pub.failure_reasons),"%s [delayed event, destination unreliable]",
									e->done.reason);
							}
						}
						js->pub.exit_code = 0;
						js->pub.done_code = EDG_WLL_STAT_FAILED; break;
					default:
						goto bad_event; break;
				}
			}
			break;
		case EDG_WLL_EVENT_CANCEL:
			if (fine_res != RET_BADBRANCH) {
				if (js->last_cancel_seqcode != NULL &&
					edg_wll_compare_seq(e->any.seqcode, js->last_cancel_seqcode) < 0) {
					res = RET_LATE;
				}
			} 
			else {
				res = RET_LATE;
			}
			if (USABLE(res, strict)) {
				switch (e->cancel.status_code) {
					case EDG_WLL_CANCEL_REQ:
						js->pub.cancelling = 1; break;
					case EDG_WLL_CANCEL_DONE:
						js->pub.state = EDG_WLL_JOB_CANCELLED;
						js->pub.remove_from_proxy = 1;
						rep(js->pub.reason, e->cancel.reason);
						rep(js->last_seqcode, e->any.seqcode);
						rep(js->pub.location, "none");
						/* fall though */
					case EDG_WLL_CANCEL_ABORT:
						js->pub.cancelling = 0; break;
					default:
						/* do nothing */
						break;

				}
			}
			if (USABLE_DATA(res, strict)) {
				rep(js->pub.cancelReason, e->cancel.reason);
			}
			break;
		case EDG_WLL_EVENT_ABORT:
			// XXX: accept Abort from WM in every case
			//	setting res make USABLE macro true (awful !!)
			if (e->any.source == EDG_WLL_SOURCE_WORKLOAD_MANAGER) res = RET_OK;
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_ABORTED;
				js->pub.remove_from_proxy = 1;
				rep(js->pub.reason, e->abort.reason);
				rep(js->pub.location, "none");

				reset_branch(js, e);
			}
			break;

		case EDG_WLL_EVENT_CLEAR:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_CLEARED;
				js->pub.remove_from_proxy = 1;
				rep(js->pub.location, "none");
				switch (e->clear.reason) {
					case EDG_WLL_CLEAR_USER:
						rep(js->pub.reason, "user retrieved output sandbox");
						break;
					case EDG_WLL_CLEAR_TIMEOUT:
						rep(js->pub.reason, "timed out, resource purge forced");
						break;
					case EDG_WLL_CLEAR_NOOUTPUT:
						rep(js->pub.reason, "no output was generated");
						break;
					default:
						goto bad_event; break;

				}
			}
			if (USABLE_DATA(res, strict)) {
				switch (e->clear.reason) {
					case EDG_WLL_CLEAR_USER:
						js->pub.sandbox_retrieved = EDG_WLL_STAT_USER;
						break;
					case EDG_WLL_CLEAR_TIMEOUT:
						js->pub.sandbox_retrieved = EDG_WLL_STAT_TIMEOUT;
						break;
					case EDG_WLL_CLEAR_NOOUTPUT:
						js->pub.sandbox_retrieved = EDG_WLL_STAT_NOOUTPUT;
						break;
					default:
						goto bad_event; break;

				}

			}
			break;
		case EDG_WLL_EVENT_PURGE:
			/* ignore, meta-information only */
			break;
		case EDG_WLL_EVENT_MATCH:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_WAITING;
				js->pub.location = location_string(
						edg_wll_SourceToString(EDG_WLL_SOURCE_WORKLOAD_MANAGER),
						e->match.host,
						e->match.src_instance);
			}
			if (USABLE_DATA(res, strict)) {
				if (USABLE_BRANCH(fine_res)) {
					rep(js->pub.destination, e->match.dest_id);
				}
				if (e->match.dest_id) {
					update_branch_state(e->any.seqcode, e->match.dest_id,
						NULL, NULL, &js->branch_states);
					add_stringlist(&js->pub.possible_destinations, 
						e->match.dest_id);
				}
			}
			break;
		case EDG_WLL_EVENT_PENDING:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_WAITING;
				rep(js->pub.reason, e->pending.reason);
				js->pub.location = location_string(
						edg_wll_SourceToString(EDG_WLL_SOURCE_WORKLOAD_MANAGER),
						e->match.host,
						e->match.src_instance);
			}
			break;
		case EDG_WLL_EVENT_REGJOB:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_SUBMITTED;
			}
			if (USABLE_DATA(res, strict)) {
				rep_cond(js->pub.jdl, e->regJob.jdl);
				edg_wlc_JobIdFree(js->pub.parent_job);
				edg_wlc_JobIdDup(e->regJob.parent,
							&js->pub.parent_job);
				rep(js->pub.network_server, e->regJob.ns);
				js->pub.children_num = e->regJob.nsubjobs;
				switch (e->regJob.jobtype) {
					case EDG_WLL_REGJOB_DAG:
					case EDG_WLL_REGJOB_PARTITIONED:
						js->pub.jobtype = EDG_WLL_STAT_DAG;
						js->pub.children_hist[EDG_WLL_JOB_UNKNOWN+1] = js->pub.children_num;
						break;
					case EDG_WLL_REGJOB_COLLECTION:
						js->pub.jobtype = EDG_WLL_STAT_COLLECTION;
						js->pub.children_hist[EDG_WLL_JOB_UNKNOWN+1] = js->pub.children_num;
						break;
					case EDG_WLL_REGJOB_FILE_TRANSFER_COLLECTION:
						js->pub.jobtype = EDG_WLL_STAT_FILE_TRANSFER_COLLECTION;
						js->pub.children_hist[EDG_WLL_JOB_UNKNOWN+1] = js->pub.children_num;
						break;
					default:
						break;
				}
				rep(js->pub.seed, e->regJob.seed);
			}
			break;
		case EDG_WLL_EVENT_USERTAG:
			if (USABLE_DATA(res, strict)) {
				if (e->userTag.name != NULL && e->userTag.value != NULL) {
					add_taglist(e->userTag.name, e->userTag.value, e->any.seqcode, js);
				} else {
					goto bad_event;
				}
			}
			break;
		case EDG_WLL_EVENT_LISTENER:
			/* ignore, listener port is not part of job status */
		case EDG_WLL_EVENT_CURDESCR:
		case EDG_WLL_EVENT_CHKPT:
			/* these three event are probably dead relics */
		case EDG_WLL_EVENT_RESOURCEUSAGE:
			/* ignore, not reflected in job status */
			break;
		case EDG_WLL_EVENT_CHANGEACL:
			/* ignore, only for event log */
			/* seq. code of this event should not influence future state computeation */
			ignore_seq_code = 1;
			break;
		case EDG_WLL_EVENT_COLLECTIONSTATE:
			new_state = edg_wll_StringToStat(e->collectionState.state);
 			if (USABLE(res, strict)) {
                                js->pub.state = new_state;
				if (new_state == EDG_WLL_JOB_DONE)
					js->pub.done_code = e->collectionState.done_code;
			}
			break;
		case EDG_WLL_EVENT_SANDBOX:
			if (USABLE_DATA(res, strict)) {
				if ((e->sandbox.sandbox_type == EDG_WLL_SANDBOX_INPUT) && e->sandbox.transfer_job) {
					edg_wlc_JobIdFree(js->pub.isb_transfer);
					edg_wlc_JobIdParse(e->sandbox.transfer_job,&js->pub.isb_transfer);
				}

				if ((e->sandbox.sandbox_type == EDG_WLL_SANDBOX_OUTPUT) && e->sandbox.transfer_job) {
					edg_wlc_JobIdFree(js->pub.osb_transfer);
					edg_wlc_JobIdParse(e->sandbox.transfer_job,&js->pub.osb_transfer);
				}
			}
			break;
		case EDG_WLL_EVENT_GRANTPAYLOADOWNERSHIP:
			if (js->payload_owner_pending) {
				if (edg_wll_gss_equal_subj(e->grantPayloadOwnership.payload_owner, js->payload_owner_pending))
					js->pub.payload_owner = js->payload_owner_pending;
				else
					free(js->payload_owner_pending);
				js->payload_owner_pending = NULL;
			} else
				rep (js->payload_owner_unconfirmed, e->grantPayloadOwnership.payload_owner);
			ignore_seq_code = 1;
			break;
		case EDG_WLL_EVENT_TAKEPAYLOADOWNERSHIP:
			if (js->payload_owner_unconfirmed &&
			    edg_wll_gss_equal_subj(e->any.user, js->payload_owner_unconfirmed)) {
				js->pub.payload_owner = js->payload_owner_unconfirmed;
				js->payload_owner_unconfirmed = NULL;
			} else
				rep (js->payload_owner_pending, e->any.user);
			ignore_seq_code = 1;
			break;

/************************************ CREAM events ****************************/
		case EDG_WLL_EVENT_CREAMSTART:
			break;

		case EDG_WLL_EVENT_CREAMPURGE:
			break;

		case EDG_WLL_EVENT_CREAMACCEPTED:
                        break;

		case EDG_WLL_EVENT_CREAMSTORE:
			if (USABLE(res, strict)) {
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
			break;
		case EDG_WLL_EVENT_CREAMCALL:
			if (e->any.source == EDG_WLL_SOURCE_CREAM_EXECUTOR &&
                                e->CREAMCall.callee == EDG_WLL_SOURCE_LRMS &&
                                e->CREAMCall.command == EDG_WLL_CREAMCALL_CMDSTART &&
                                e->CREAMCall.result == EDG_WLL_CREAMCALL_OK)
			{
				if (USABLE(res, strict)) {
                                        // BLAH -> LRMS
					js->pub.state = EDG_WLL_JOB_SCHEDULED;
					js->pub.cream_state = EDG_WLL_STAT_CREAM_IDLE;
					rep_cond(js->pub.cream_reason, e->CREAMCall.reason);
					rep_cond(js->pub.reason, e->CREAMCall.reason);
				}
			}
			if (e->CREAMCall.command == EDG_WLL_CREAMCALL_CMDCANCEL &&
                                e->CREAMCall.result == EDG_WLL_CREAMCALL_OK)
                        {
                                if (USABLE(res, strict)){
                                        js->pub.cream_cancelling = 1;
                                        js->pub.cancelling = 1;
					 rep_cond(js->pub.cream_reason, e->CREAMCall.reason);
	                                rep_cond(js->pub.reason, e->CREAMCall.reason);
                                }
                        }
			break;
		case EDG_WLL_EVENT_CREAMCANCEL:
			if (USABLE(res, strict)) {
                                if (e->CREAMCancel.status_code == EDG_WLL_CANCEL_DONE) {
                                        js->pub.state = EDG_WLL_JOB_CANCELLED;
                                        js->pub.cream_state = EDG_WLL_STAT_CREAM_ABORTED;
                                }
				rep(js->pub.cream_reason, e->CREAMCancel.reason);
                                rep(js->pub.reason, e->CREAMCancel.reason);
                        }
                        break;
		case EDG_WLL_EVENT_CREAMABORT:
                        if (USABLE(res, strict)) {
                                js->pub.state = EDG_WLL_JOB_ABORTED;
                                js->pub.cream_state = EDG_WLL_STAT_CREAM_ABORTED;
				rep(js->pub.cream_reason, e->CREAMAbort.reason);
                                rep(js->pub.reason, e->CREAMAbort.reason);
                        }
                        break;
		case EDG_WLL_EVENT_CREAMSTATUS:
                        if (USABLE(res, strict) && e->CREAMStatus.result == EDG_WLL_CREAMSTATUS_DONE)
                        {
                                switch (js->pub.cream_state = edg_wll_StringToCreamStat(e->CREAMStatus.new_state))
                                {
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
                                        case EDG_WLL_STAT_CREAM_ABORTED: js->pub.state = EDG_WLL_JOB_ABORTED; break;
					case EDG_WLL_STAT_CREAM_CANCELLED: js->pub.state = EDG_WLL_JOB_CANCELLED; break;
					case EDG_WLL_STAT_CREAM_PURGED: js->pub.state = EDG_WLL_JOB_CLEARED; break;
                                }
                        }
                        break;

		default:
			goto bad_event;
			break;
	}

	if (USABLE(res,strict)) {
		js->pub.lastUpdateTime = e->any.timestamp;
		if (old_state != js->pub.state) {
			js->pub.stateEnterTime = js->pub.lastUpdateTime;
			js->pub.stateEnterTimes[1 + js->pub.state]
				= (int)js->pub.lastUpdateTime.tv_sec;

			/* in case of resubmit switch JW status back to unknown */
			if (js->pub.state  == EDG_WLL_JOB_WAITING || 
			    js->pub.state == EDG_WLL_JOB_READY ||
			    js->pub.state ==  EDG_WLL_JOB_SCHEDULED) {
				js->pub.jw_status = EDG_WLL_STAT_UNKNOWN;
			}
		}
		if (e->any.type == EDG_WLL_EVENT_CANCEL) {
			rep(js->last_cancel_seqcode, e->any.seqcode);
		} else {

/* the first set of LM events (Accept, Transfer/XX -> LRMS)
   should not should shift the state (to Ready, Scheduled) but NOT to
   update js->last_seqcode completely, in order not to block following
   LRMS events which are likely to arrive later but should still affect
   job state (as there may be no more LM events due to the Condor bug).
   However, don't ignore the incoming seqcode completely, to catch up
   with possibly delayed WM/JSS events */

			if (lm_favour_lrms) {
				free(js->last_seqcode);
				js->last_seqcode = set_component_seqcode(e->any.seqcode,EDG_WLL_SOURCE_LOG_MONITOR,0);
			}
			else if (!ignore_seq_code) { 
				rep(js->last_seqcode, e->any.seqcode); 
			}
		}

		if (js->pub.state != EDG_WLL_JOB_RUNNING) {
			js->pub.suspended = 0;
			rep_null(js->pub.suspend_reason);
		}

		if (fine_res == RET_GOODBRANCH) {
			rep(js->last_branch_seqcode, e->any.seqcode);
		}
	}

	if (USABLE_DATA(res,strict)) {
		if (e->any.source == EDG_WLL_SOURCE_NETWORK_SERVER &&
			js->pub.network_server == NULL) {
			char *inst;
			inst = e->any.src_instance;
			trio_asprintf(&js->pub.network_server, "%s%s%s",
				e->any.host,
				inst != NULL ? ":" : " ",
				inst != NULL ? inst : "");
		}
	}

	return res;

bad_event:
	badEvent(js,e,ev_seq);
	return RET_SUSPECT;
}

int add_stringlist(char ***lptr, const char *new_item)
{
	char **itptr;
	int		i;

	if (*lptr == NULL) {
		itptr = (char **) malloc(2*sizeof(char *));
		itptr[0] = strdup(new_item);
		itptr[1] = NULL;
		*lptr = itptr;
		return 1;
	} else {
		for (i = 0, itptr = *lptr; itptr[i] != NULL; i++);
		itptr = (char **) realloc(*lptr, (i+2)*sizeof(char *));
		if (itptr != NULL) {
			itptr[i] = strdup(new_item);
			itptr[i+1] = NULL;
			*lptr = itptr;
			return 1;
		} else {
			return 0;
		}
	}
}

void destroy_intJobStat_extension(intJobStat *p)
{
	if (p->last_seqcode) free(p->last_seqcode);
	if (p->last_cancel_seqcode) free(p->last_cancel_seqcode);
	if (p->branch_tag_seqcode) free(p->branch_tag_seqcode);
	if (p->last_branch_seqcode) free(p->last_branch_seqcode);
	if (p->deep_resubmit_seqcode) free(p->deep_resubmit_seqcode);
	free_branch_state(&p->branch_states);
	if (p->tag_seq_codes) {
		int	i;
		
		for (i=0; p->tag_seq_codes[i]; i++) free(p->tag_seq_codes[i]);
		free(p->tag_seq_codes);
	}
	if (p->payload_owner_pending) free(p->payload_owner_pending);
	if (p->payload_owner_unconfirmed) free(p->payload_owner_unconfirmed);

	memset(p,0,sizeof(*p));
}

void destroy_intJobStat(intJobStat *p)
{
	edg_wll_FreeStatus(&p->pub);
	destroy_intJobStat_extension(p);
	memset(p, 0, sizeof(intJobStat));
}

void init_intJobStat(intJobStat *p)
{
	memset(p, 0, sizeof(intJobStat));
	edg_wll_InitStatus(&(p->pub));
	p->pub.jobtype = -1 /* why? EDG_WLL_STAT_SIMPLE */;
	p->pub.children_hist = (int*) calloc(1+EDG_WLL_NUMBER_OF_STATCODES, sizeof(int));
	p->pub.children_hist[0] = EDG_WLL_NUMBER_OF_STATCODES;
	p->pub.stateEnterTimes = (int*) calloc(1+EDG_WLL_NUMBER_OF_STATCODES, sizeof(int));
	p->pub.stateEnterTimes[0] = EDG_WLL_NUMBER_OF_STATCODES;
	/* TBD: generate */
}


int filterEvent(intJobStat *js, edg_wll_Event *e, int ev_seq, edg_wll_Event *prev_ev, int prev_seq) 
{
	if(NULL == js) {
		/* job type not specified yet */
		return 0;
	}
	switch (js->pub.jobtype) {
		case EDG_WLL_STAT_PBS: 
			return filterEvent_PBS(js, e, ev_seq, prev_ev, prev_seq);
	}
	return 0;
}
