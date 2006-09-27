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

#define rep(a,b) { free(a); a = (b == NULL) ? NULL : strdup(b); }

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

static int add_taglist(edg_wll_TagValue **lptr, const char *new_item, const char *new_item2)
{
	edg_wll_TagValue 	*itptr;
	int			i;

	if (*lptr == NULL) {
		itptr = (edg_wll_TagValue *) calloc(2,sizeof(edg_wll_TagValue));
		itptr[0].tag = strdup(new_item);
		itptr[0].value = strdup(new_item2);
		*lptr = itptr;
		return 1;
	} else {
		for (i = 0, itptr = *lptr; itptr[i].tag != NULL; i++)
			if ( !strcasecmp(itptr[i].tag, new_item) )
			{
				free(itptr[i].value);
				itptr[i].value = strdup(new_item2);
				return 1;
			}
		itptr = (edg_wll_TagValue *) realloc(*lptr, (i+2)*sizeof(edg_wll_TagValue));
		if (itptr != NULL) {
			itptr[i].tag = strdup(new_item);
			itptr[i].value = strdup(new_item2);
			itptr[i+1].tag = NULL;
			itptr[i+1].value = NULL;
			*lptr = itptr;
			return 1;
		} else {
			return 0;
		}
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
	rep(js->branch_tag_seqcode, NULL);
	rep(js->deep_resubmit_seqcode, e->any.seqcode);
}

static char* location_string(const char *source, const char *host, const char *instance)
{
	char *ret;
	asprintf(&ret, "%s/%s/%s", source, host, instance);
	return ret;
}

static int after_enter_wm(const char *es,const char *js)
{
	return component_seqcode(es,EDG_WLL_SOURCE_NETWORK_SERVER) >
		component_seqcode(js,EDG_WLL_SOURCE_NETWORK_SERVER);
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

static edg_wll_ErrorCode update_parent_status(edg_wll_JobStatCode old_state, enum edg_wll_StatDone_code old_done_code, intJobStat *cis)
{
	edg_wll_Context ctx;
	intJobStat	*pis;
	int		ret, update_hist = 0;


	edg_wll_InitContext(&ctx);

	if (edg_wll_LockJob(ctx,cis->pub.parent_job)) 
		goto err;

	if (edg_wll_LoadIntState(ctx, cis->pub.parent_job, - 1, &pis))
		goto err;

	/* Easy version, where the whole histogram is evolving... 
	 * not used because of performance reasons 
	 *
		pis->pub.children_hist[cis->pub.state]++;
		if (cis->pub.state == EDG_WLL_JOB_DONE) 
			pis->children_done_hist[cis->pub.done_code]++;
		pis->pub.children_hist[old_state]--;
		if (old_state == EDG_WLL_JOB_DONE)
			pis->children_done_hist[old_done_code]--;
		edg_wll_SetSubjobHistogram(ctx, cis->pub.parent_job, pis);
	*/

	/* Increment histogram for interesting states and 
	 * cook artificial events to enable parent job state shift 
	 */
	switch (cis->pub.state) {
		case EDG_WLL_JOB_RUNNING:
				pis->pub.children_hist[cis->pub.state]++;
				update_hist = 1;			

				/* not RUNNING yet? */
				if (pis->pub.state < EDG_WLL_JOB_RUNNING) {
					//XXX cook artificial event for parent job
					//    and call db_store
			}
			break;
		case EDG_WLL_JOB_DONE:
			// edg_wll_GetSubjobHistogram(ctx, cis->pub.parent_job, &pis);
			// not needed, load by edg_wll_LoadIntState()

			pis->pub.children_hist[cis->pub.state]++;
			update_hist = 1;			

			pis->children_done_hist[cis->pub.done_code]++;
			if (pis->pub.children_hist[cis->pub.state] == pis->pub.children_num) {
				// XXX cook artificial event for parent job
				//    and call db_store
			}
			break;
		// XXX: more cases to bo added...
		case EDG_WLL_JOB_CLEARED:
			break;
		default:
			break;
	}
	

	/* Decrement histogram for interesting states
	 */
	switch (old_state) {
		case EDG_WLL_JOB_RUNNING:
			pis->pub.children_hist[old_state]--;
			update_hist = 1;
			break;
		case EDG_WLL_JOB_DONE:
			pis->pub.children_hist[old_state]--;
			pis->children_done_hist[old_done_code]--;
			update_hist = 1;
			break;
		// XXX: more cases to bo added...
		default:
			break;
	}

	if (update_hist)
		edg_wll_SetSubjobHistogram(ctx, cis->pub.parent_job, pis);
	
err:
	edg_wll_UnlockJob(ctx,cis->pub.parent_job);
	destroy_intJobStat(pis);
	ret = edg_wll_Error(ctx,NULL,NULL);
	edg_wll_FreeContext(ctx);

	return ret;
}



// (?) || (0 && 1)  =>  true if (res == RET_OK)
#define USABLE(res,strict) ((res) == RET_OK || ( (res) == RET_SOON && !strict))

// (?) || (1 && 1)  =>  always true
#define USABLE_DATA(res,strict) ((res) == RET_OK || ( (res) != RET_FATAL && !strict))

#define USABLE_BRANCH(fine_res) ((fine_res) != RET_TOOOLD && (fine_res) != RET_BADBRANCH)
#define LRMS_STATE(state) ((state) == EDG_WLL_JOB_RUNNING || (state) == EDG_WLL_JOB_DONE)
#define PARSABLE_SEQCODE(code) (component_seqcode((code),0) >= 0)

int processEvent(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring)
{
	edg_wll_JobStatCode	old_state = js->pub.state;
	enum edg_wll_StatDone_code	old_done_code = js->pub.done_code;
	edg_wll_JobStatCode	new_state = EDG_WLL_JOB_UNKNOWN;
	int			res = RET_OK,
				fine_res = RET_OK;
				
	int	lm_favour_lrms = 0;

	if (old_state == EDG_WLL_JOB_ABORTED ||
		old_state == EDG_WLL_JOB_CANCELLED ||
		old_state == EDG_WLL_JOB_CLEARED) {
		res = RET_LATE;
	}

/* new event coming from NS => forget about any resubmission loops */
	if (e->type != EDG_WLL_EVENT_CANCEL && 
		js->last_seqcode &&
		after_enter_wm(e->any.seqcode,js->last_seqcode))
	{
		rep(js->branch_tag_seqcode,NULL); 
		rep(js->deep_resubmit_seqcode,NULL); 
		rep(js->last_branch_seqcode,NULL); 
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
						rep(js->pub.jdl, e->transfer.job); break;
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
						js->pub.resubmitted += 1;
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
						rep(js->pub.jdl, e->enQueued.job); break;
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
				syslog(LOG_ERR,"ReallyRunning on bad branch %s",
						e->any.source == EDG_WLL_SOURCE_LOG_MONITOR ? e->reallyRunning.wn_seq : e->any.seqcode);
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
				break;
			}
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_DONE;
				rep(js->pub.reason, e->done.reason);
				if (fine_res == RET_GOODBRANCH) {
					js->pub.payload_running = 0;
				}
				switch (e->done.status_code) {
					case EDG_WLL_DONE_CANCELLED:
						js->pub.state = EDG_WLL_JOB_CANCELLED;
					case EDG_WLL_DONE_OK:
						rep(js->pub.location, "none"); break;
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
				rep(js->pub.reason, e->abort.reason);
				rep(js->pub.location, "none");

				reset_branch(js, e);
			}
			break;

		case EDG_WLL_EVENT_CLEAR:
			if (USABLE(res, strict)) {
				js->pub.state = EDG_WLL_JOB_CLEARED;
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
				rep(js->pub.jdl, e->regJob.jdl);
				edg_wlc_JobIdFree(js->pub.parent_job);
				edg_wlc_JobIdDup(e->regJob.parent,
							&js->pub.parent_job);
				rep(js->pub.network_server, e->regJob.ns);
				js->pub.children_num = e->regJob.nsubjobs;
				if (e->regJob.jobtype == EDG_WLL_REGJOB_DAG
					|| e->regJob.jobtype == EDG_WLL_REGJOB_PARTITIONED) {
					js->pub.jobtype = EDG_WLL_STAT_DAG;
				}
				rep(js->pub.seed, e->regJob.seed);
			}
			break;
		case EDG_WLL_EVENT_USERTAG:
			if (USABLE_DATA(res, strict)) {
				if (e->userTag.name != NULL && e->userTag.value != NULL) {
					add_taglist(&js->pub.user_tags, 
						    e->userTag.name, e->userTag.value);
				} else {
					goto bad_event;
				}
			}
			break;
		case EDG_WLL_EVENT_LISTENER:
			/* ignore, listener port is not part of job status */
			break;
		case EDG_WLL_EVENT_CURDESCR:
		case EDG_WLL_EVENT_CHKPT:
		case EDG_WLL_EVENT_CHANGEACL:
			/* ignore, only for event log */
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
		}
		if (e->any.type == EDG_WLL_EVENT_CANCEL) {
			rep(js->last_cancel_seqcode, e->any.seqcode);
		} else {

/* the first set of LM events (Accept, Transfer/* -> LRMS)
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
			else rep(js->last_seqcode, e->any.seqcode);
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
			asprintf(&js->pub.network_server, "%s%s%s",
				e->any.host,
				inst != NULL ? ":" : " ",
				inst != NULL ? inst : "");
		}
	}
	
	/* check whether subjob state change does not change parent state */
	if ((js->pub.parent_job) && (old_state != js->pub.state)) { 
		if (update_parent_status(old_state, old_done_code, js))
			// XXX: is it good error code? (hard one)
			res = RET_INTERNAL;
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
	free(p->last_seqcode); p->last_seqcode = NULL;
	free(p->last_cancel_seqcode); p->last_cancel_seqcode = NULL;
			       p->resubmit_type = EDG_WLL_RESUBMISSION_UNDEFINED;
}

void destroy_intJobStat(intJobStat *p)
{
	edg_wll_FreeStatus(&p->pub);
	destroy_intJobStat_extension(p);
	memset(p, 0, sizeof(intJobStat));
}

