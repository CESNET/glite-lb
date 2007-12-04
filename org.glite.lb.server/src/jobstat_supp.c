#ident "$Header$"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <regex.h>
#include <syslog.h>

#include "glite/jobid/cjobid.h"
#include "glite/lbu/trio.h"
#include "glite/lbu/db.h"
#include "glite/lb/context-int.h"

#include "store.h"
#include "index.h"
#include "jobstat.h"
#include "get_events.h"


/* TBD: share in whole logging or workload */
#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif

static char* enc_strlist(char *, char **) UNUSED_VAR;
static char **dec_strlist(char *, char **) UNUSED_VAR;

/*
 * string encoding routines for safe DB store
 */

static char *enc_string(char *old, char *item)
{
	char *out;
	if (item == NULL) {
		asprintf(&out,"%s-1 ", old);
	} else {
		asprintf(&out,"%s%ld %s",old, (long)strlen(item), item);
	}
	free(old);
	return out;
}

static char *dec_string(char *in, char **rest)
{
	int scret;
	long len = -1;
	char *out;
	
	scret = sscanf(in, "%ld", &len);
	if (scret < 1) {
		*rest = NULL;
		return NULL;
	}
	if (len == -1) {
		out = NULL;
		*rest = strchr(in, ' ') ? strchr(in, ' ') + 1 : NULL;
	} else {
		in = strchr(in, ' ') ? strchr(in, ' ') + 1 : NULL;
		out = (char *)malloc(len+1);
		if (out) {
			memcpy(out, in, len);
			*(out+len) = '\0';
		}
		*rest = in+len;
	}
	return out;
}

static char *enc_int(char *old, int item)
{
        char *out;
        asprintf(&out,"%s%d ", old, item);
	free(old);
        return out;
}

static char *enc_int_array(char *old, int *item, int itemsNo)
{
        char *out;
	int index;
	char *strpom;

	strpom=(char*)calloc(strlen(old)+1,sizeof(char));

	for (index=0; index <= itemsNo; index++) sprintf(strpom+strlen(strpom),"%d%s", item[index],index==itemsNo?"":";");

        asprintf(&out,"%s%s ", old, strpom);
	free(strpom);
        free(old);

        return out;
}

static int dec_int(char* in, char **rest)
{
	int scret;
	int out;

	scret = sscanf(in, "%d", &out);
	if (scret == 1) {
		*rest = strchr(in, ' ') ? strchr(in, ' ') + 1 : NULL;
	} else {
		out = 0;
		*rest = in;
	}
	return out;
}

static int dec_int_array(char* in, char **rest, int *out)	// Returns the number of items found in the array
{
	int charNo, itemsNo = 0, cindex, iindex = 0, lenindex;
	char *tempstr;

        /* Find out the number of items in the field first */

	for (charNo = 0;charNo<strlen(in);charNo++)	{
		if (in[charNo] == ' ') {	/* Only ' ' (space) is accepted as a separator. Should not be a broblem. */
			itemsNo++;
			break;
		}
		if (in[charNo] == ';') {
			itemsNo++;
		}
	}
	if (!itemsNo) {		/* No separator has been found. This is the last input string */
		itemsNo = 1;	/* - consider it an one-item array */
		*rest = NULL;
	}
	else *rest = in + charNo + 1;

	tempstr = (char*)calloc(charNo+1,sizeof(char));

	strcpy(tempstr,"");

	for (cindex = 0; cindex<charNo; cindex++) {
		if ((in[cindex] == ';') || (in[cindex] == ' ')) {
			out[iindex] = atoi(tempstr);
			strcpy(tempstr,"");
			iindex++;
		}
		else {
			lenindex=strlen(tempstr);
			tempstr[lenindex]=in[cindex];
			tempstr[lenindex+1]=0;
		}
	}
	if (in[cindex] != ' ') out[iindex] = atoi(tempstr);	/* string not terminated with a separator */

	free(tempstr);
	*rest = in + charNo + 1;

        return itemsNo;
}


static char* enc_jobid(char *old, edg_wlc_JobId item)
{
	char *str;
	char *out;

	str = edg_wlc_JobIdUnparse(item);
	out = enc_string(old, str);
	free(str);
	return out;
}
static edg_wlc_JobId dec_jobid(char *in, char **rest)
{
	char *str;
	edg_wlc_JobId jobid;
	
	str = dec_string(in, rest);
	if (str == NULL) return NULL;
	edg_wlc_JobIdParse(str, &jobid);
	free(str);
	return jobid;
}

static char* enc_strlist(char *old, char **item)
{
	char *ret;

	if (item == NULL) {
		asprintf(&ret,"%s-1 ", old);
		free(old);
		return ret;
	} else {
		asprintf(&ret,"%s1 ",old);
		free(old);
		if (ret == NULL) return ret;
	}
	do {
		ret = enc_string(ret, *item);
	} while (*(item++) != NULL);
	return ret;
}

static char **dec_strlist(char *in, char **rest)
{
	char **out;
	int len = -1;
	char *tmp_in, *tmp_ret;
	int scret;

	scret = sscanf(in, "%d", &len);
	if (scret < 1) {
		*rest = NULL;
		return NULL;
	}
	if (len == -1) {
		*rest = strchr(in, ' ') ? strchr(in, ' ') + 1 : NULL;
		return NULL;
	}

	len = 0;
	tmp_in = in = strchr(in, ' ') + 1 ;
	do {
		tmp_ret = dec_string(tmp_in, &tmp_in);
		len++;
	}  while (tmp_ret != NULL);

	out = (char**) malloc(len*sizeof(char*));

	if (out) {
		len = 0;
		tmp_in = in;
		do {
			out[len] = dec_string(tmp_in, &tmp_in);
		} while  (out[len++] != NULL);
	}
	*rest = tmp_in;
	return out;
}

static char *enc_branch_states(char *old, branch_state *item)
{
	char *ret;

	if (item == NULL) {
		asprintf(&ret,"%s-1 ", old);
		free(old);
		return ret;
	} else {
		asprintf(&ret,"%s1 ",old);
		free(old);
		if (ret == NULL) return ret;
	}
	do {
		ret = enc_int(ret, (*item).branch);
		ret = enc_string(ret, (*item).destination);
		ret = enc_string(ret, (*item).ce_node);
		ret = enc_string(ret, (*item).jdl);
	} while ((*(item++)).branch != 0);
	return ret;
}

static branch_state *dec_branch_states(char *in, char **rest)
{
	branch_state *out;
	int len = -1, b = 0;
	char *tmp_in, *tmp_ret;
	int scret;

	scret = sscanf(in, "%d", &len);
	if (scret < 1) {
		*rest = NULL;
		return NULL;
	}
	if (len == -1) {
		*rest = strchr(in, ' ') ? strchr(in, ' ') + 1 : NULL;
		return NULL;
	}

	len = 0;
	tmp_in = in = strchr(in, ' ') + 1 ;
	do {
		b = dec_int(tmp_in, &tmp_in);
		tmp_ret = dec_string(tmp_in, &tmp_in); free(tmp_ret);
		if (!tmp_in) { *rest = tmp_in; return NULL; }
		tmp_ret = dec_string(tmp_in, &tmp_in); free(tmp_ret);
		if (!tmp_in) { *rest = tmp_in; return NULL; }
		tmp_ret = dec_string(tmp_in, &tmp_in); free(tmp_ret);
		if (!tmp_in) { *rest = tmp_in; return NULL; }
		len++;
	}  while (b != 0);

	out = (branch_state *) calloc(len+1, sizeof(branch_state));

	if (out) {
		len = 0;
		tmp_in = in;

		do {
			out[len].branch = dec_int(tmp_in, &tmp_in);
			out[len].destination = dec_string(tmp_in, &tmp_in);
			out[len].ce_node = dec_string(tmp_in, &tmp_in);
			out[len].jdl = dec_string(tmp_in, &tmp_in);
		} while  (out[len++].branch != 0);
		*rest = tmp_in;
	}
	else 
		*rest = 0;

	return out;

}

static char* enc_taglist(char *old, edg_wll_TagValue *item)
{
	char *ret;

	if (item == NULL) {
		asprintf(&ret,"%s-1 ", old);
		free(old);
		return ret;
	} else {
		asprintf(&ret,"%s1 ",old);
		free(old);
		if (ret == NULL) return ret;
	}
	do {
		ret = enc_string(ret, (*item).tag);
		ret = enc_string(ret, (*item).value);
	} while ((*(item++)).tag != NULL);
	return ret;
}

static edg_wll_TagValue *dec_taglist(char *in, char **rest)
{
	edg_wll_TagValue *out;
	int len = -1;
	char *tmp_in, *tmp_ret;
	int scret, end = 0;

	scret = sscanf(in, "%d", &len);
	if (scret < 1) {
		*rest = NULL;
		return NULL;
	}
	if (len == -1) {
		*rest = strchr(in, ' ') ? strchr(in, ' ') + 1 : NULL;
		return NULL;
	}

	len = 0;
	tmp_in = in = strchr(in, ' ') + 1 ;
	do {
		tmp_ret = dec_string(tmp_in, &tmp_in);
		if (tmp_ret) free(tmp_ret); 
		else end = 1;
		if (!tmp_in) { *rest = tmp_in; return NULL; }
		tmp_ret = dec_string(tmp_in, &tmp_in); 
		free(tmp_ret);
		if (!tmp_in) { *rest = tmp_in; return NULL; }
		len++;
	}  while (!end);

	out = (edg_wll_TagValue *) malloc(len*sizeof(edg_wll_TagValue));

	if (out) {
		len = 0;
		tmp_in = in;

		do {
			out[len].tag = dec_string(tmp_in, &tmp_in);
			out[len].value = dec_string(tmp_in, &tmp_in);
		} while  (out[len++].tag != NULL);
		*rest = tmp_in;
	}
	else 
		*rest = 0;

	return out;
}

static char *enc_intlist(char *old, int *item)
{
	int len;
	char *ret;

	if (item == NULL) {
		asprintf(&ret,"%s-1 ", old);
		free(old);
		return ret;
	} else {
		asprintf(&ret,"%s1 ",old);
		free(old);
		if (ret == NULL) return ret;
	}
	len = *item; item++;
	ret = enc_int(ret, len);
	for (; len > 0 ; len--, item++) {
		ret = enc_int(ret, *item);
	}

	return ret;
}

static int *dec_intlist(char *in, char **rest)
{
	int len = -1;
	int *out, *ptr;
	char *tmp_in;
	int scret;

	scret = sscanf(in, "%d", &len);
	if (scret < 1) {
		*rest = NULL;
		return NULL;
	}
	tmp_in = strchr(in, ' ') ? strchr(in, ' ') + 1 : NULL;
	if (len == -1 || tmp_in == NULL) {
		*rest = tmp_in;
		return NULL;
	}

	len = dec_int(tmp_in, &tmp_in);
	out = (int *)malloc( (len+1) *sizeof(int));
	if (out) {
		*out = len; 
		ptr = out+1;
		while (len) {
			*ptr = dec_int(tmp_in, &tmp_in);
			len--; ptr++;
		}
	}
	*rest = tmp_in;
	return out;
}

static char* enc_timeval(char *old, struct timeval item)
{
	char *ret;
	
	ret = enc_int(old, (int)item.tv_sec);
	if (ret) {
		ret = enc_int(ret, (int)item.tv_usec);
	}
	return ret;
}

static struct timeval dec_timeval(char *in, char **rest)
{
	struct timeval t;
	char *tmp_in;
	
	t.tv_sec = dec_int(in, &tmp_in);
	if (tmp_in != NULL) t.tv_usec = dec_int(tmp_in, &tmp_in);
	*rest = tmp_in;
	return t;
}

static char *enc_JobStat(char *old, edg_wll_JobStat* stat)
{
	char *ret;

	ret = enc_int(old, stat->state);
	if (ret) ret = enc_jobid(ret, stat->jobId);
	if (ret) ret = enc_string(ret, stat->owner);
	if (ret) ret = enc_int(ret, stat->jobtype);
	if (ret) ret = enc_jobid(ret, stat->parent_job);
	if (ret) ret = enc_string(ret, stat->seed);
	if (ret) ret = enc_int(ret, stat->children_num);
		/* children histogram also stored in the DB, see bellow. Other children data not stored. */
	if (ret) ret = enc_string(ret, stat->condorId);
	if (ret) ret = enc_string(ret, stat->globusId);
	if (ret) ret = enc_string(ret, stat->localId);
	if (ret) ret = enc_string(ret, stat->jdl);
	if (ret) ret = enc_string(ret, stat->matched_jdl);
	if (ret) ret = enc_string(ret, stat->destination);
	if (ret) ret = enc_string(ret, stat->condor_jdl);
	if (ret) ret = enc_string(ret, stat->rsl);
	if (ret) ret = enc_string(ret, stat->reason);
	if (ret) ret = enc_string(ret, stat->location);
	if (ret) ret = enc_string(ret, stat->ce_node);
	if (ret) ret = enc_string(ret, stat->network_server);
	if (ret) ret = enc_int(ret, stat->subjob_failed);
	if (ret) ret = enc_int(ret, stat->done_code);
	if (ret) ret = enc_int(ret, stat->exit_code);
	if (ret) ret = enc_int(ret, stat->resubmitted);
	if (ret) ret = enc_int(ret, stat->cancelling);
	if (ret) ret = enc_string(ret, stat->cancelReason);
	if (ret) ret = enc_int(ret, stat->cpuTime);
	if (ret) ret = enc_taglist(ret, stat->user_tags);
	if (ret) ret = enc_timeval(ret, stat->stateEnterTime);
	if (ret) ret = enc_intlist(ret, stat->stateEnterTimes);
	if (ret) ret = enc_timeval(ret, stat->lastUpdateTime);
	if (ret) ret = enc_int(ret, stat->expectUpdate);
	if (ret) ret = enc_string(ret, stat->expectFrom);
	if (ret) ret = enc_int(ret, stat->payload_running);
	if (ret) ret = enc_strlist(ret, stat->possible_destinations);
	if (ret) ret = enc_strlist(ret, stat->possible_ce_nodes);
	if (ret) ret = enc_int(ret, stat->suspended);
	if (ret) ret = enc_string(ret, stat->suspend_reason);
	if (ret) ret = enc_int_array(ret, stat->children_hist, EDG_WLL_NUMBER_OF_STATCODES);
	if (ret) ret = enc_string(ret, stat->failure_reasons);
	if (ret) ret = enc_int(ret, stat->remove_from_proxy);
	if (ret) ret = enc_string(ret, stat->pbs_state);
	if (ret) ret = enc_string(ret, stat->pbs_queue);
	if (ret) ret = enc_string(ret, stat->pbs_owner);
	if (ret) ret = enc_string(ret, stat->pbs_name);
	if (ret) ret = enc_string(ret, stat->pbs_reason);
	if (ret) ret = enc_string(ret, stat->pbs_scheduler);
	if (ret) ret = enc_string(ret, stat->pbs_dest_host);
	if (ret) ret = enc_int(ret, stat->pbs_pid);
	if (ret) ret = enc_int(ret, stat->pbs_exit_status);
	if (ret) ret = enc_string(ret, stat->pbs_error_desc);
	if (ret) ret = enc_string(ret, stat->condor_status);
	if (ret) ret = enc_string(ret, stat->condor_universe);
	if (ret) ret = enc_string(ret, stat->condor_owner);
	if (ret) ret = enc_string(ret, stat->condor_preempting);
	if (ret) ret = enc_int(ret, stat->condor_shadow_pid);
	if (ret) ret = enc_int(ret, stat->condor_shadow_exit_status);
	if (ret) ret = enc_int(ret, stat->condor_starter_pid);
	if (ret) ret = enc_int(ret, stat->condor_starter_exit_status);
	if (ret) ret = enc_int(ret, stat->condor_job_pid);
	if (ret) ret = enc_int(ret, stat->condor_job_exit_status);
	if (ret) ret = enc_string(ret, stat->condor_dest_host);
	if (ret) ret = enc_string(ret, stat->condor_reason);
	if (ret) ret = enc_string(ret, stat->condor_error_desc);

	return ret;
}
static edg_wll_JobStat* dec_JobStat(char *in, char **rest)
{
	char *tmp_in;
	edg_wll_JobStat *stat;

	stat = (edg_wll_JobStat *) calloc(1,sizeof(edg_wll_JobStat));
	if (!stat) return stat;
	tmp_in = in;

        stat->state = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->jobId = dec_jobid(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->owner = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->jobtype = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->parent_job = dec_jobid(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->seed = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->children_num = dec_int(tmp_in, &tmp_in);
                /* children histogram also stored in the DB, see bellow. Other children data not stored. */
        if (tmp_in != NULL) stat->condorId = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->globusId = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->localId = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->jdl = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->matched_jdl = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->destination = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->condor_jdl = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->rsl = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->reason = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->location = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->ce_node = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->network_server = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->subjob_failed = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->done_code = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->exit_code = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->resubmitted = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->cancelling = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->cancelReason = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->cpuTime = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->user_tags = dec_taglist(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->stateEnterTime = dec_timeval(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->stateEnterTimes = dec_intlist(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->lastUpdateTime = dec_timeval(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->expectUpdate = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->expectFrom = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->payload_running = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->possible_destinations = dec_strlist(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->possible_ce_nodes = dec_strlist(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->suspended = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->suspend_reason = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) {
			    stat->children_hist = (int*)calloc(EDG_WLL_NUMBER_OF_STATCODES+1, sizeof(int));
			    dec_int_array(tmp_in, &tmp_in, stat->children_hist);
	}
        if (tmp_in != NULL) stat->failure_reasons = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->remove_from_proxy = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_state = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_queue = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_owner = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_name = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_reason = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_scheduler = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_dest_host = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_pid = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_exit_status = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_error_desc = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->condor_status = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->condor_universe = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->condor_owner = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->condor_preempting = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->condor_shadow_pid = dec_int(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->condor_shadow_exit_status = dec_int(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->condor_starter_pid = dec_int(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->condor_starter_exit_status = dec_int(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->condor_job_pid = dec_int(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->condor_job_exit_status = dec_int(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->condor_dest_host = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->condor_reason = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->condor_error_desc = dec_string(tmp_in, &tmp_in);

	*rest = tmp_in;

	return stat;
}

char *enc_intJobStat(char *old, intJobStat* stat)
{
	char *ret;

	ret = enc_JobStat(old, &stat->pub);
	if (ret) ret = enc_int(ret, stat->resubmit_type);
	if (ret) ret = enc_string(ret, stat->last_seqcode);
	if (ret) ret = enc_string(ret, stat->last_cancel_seqcode);
	if (ret) ret = enc_string(ret, stat->branch_tag_seqcode);
	if (ret) ret = enc_string(ret, stat->last_branch_seqcode);
	if (ret) ret = enc_string(ret, stat->deep_resubmit_seqcode);
	if (ret) ret = enc_branch_states(ret, stat->branch_states);
	if (ret) ret = enc_timeval(ret, stat->last_pbs_event_timestamp);
	if (ret) ret = enc_int(ret, stat->pbs_reruning);
	return ret;
}

intJobStat* dec_intJobStat(char *in, char **rest)
{
	edg_wll_JobStat *pubstat;
	intJobStat *stat = 0;
	char *tmp_in;

	pubstat = dec_JobStat(in, &tmp_in);

	if (tmp_in != NULL) {
		stat = (intJobStat *)calloc(1,sizeof(intJobStat));
	} 
	if (stat != NULL) {
		stat->pub = *pubstat;
		free(pubstat);
		stat->resubmit_type = dec_int(tmp_in, &tmp_in);
		if (tmp_in != NULL) {
			stat->last_seqcode = dec_string(tmp_in, &tmp_in);
		}
		if (tmp_in != NULL) {
			stat->last_cancel_seqcode = dec_string(tmp_in, &tmp_in);
		}
		if (tmp_in != NULL) {
			stat->branch_tag_seqcode = dec_string(tmp_in, &tmp_in);
		}
		if (tmp_in != NULL) {
			stat->last_branch_seqcode = dec_string(tmp_in, &tmp_in);
		}
		if (tmp_in != NULL) {
			stat->deep_resubmit_seqcode = dec_string(tmp_in, &tmp_in);
		}
		if (tmp_in != NULL) {
			stat->branch_states = dec_branch_states(tmp_in, &tmp_in);
		}
		if (tmp_in != NULL) {
			stat->last_pbs_event_timestamp = dec_timeval(tmp_in, &tmp_in);
		}
		if (tmp_in != NULL) {
			stat->pbs_reruning = dec_int(tmp_in, &tmp_in);
		}
	} else if (tmp_in != NULL) {
		edg_wll_FreeStatus(pubstat);
		free(pubstat);
	}

	*rest = tmp_in;
	return stat;
}

/*
 * Compute part of SQL command used for indexed state table columns
 */

edg_wll_ErrorCode edg_wll_IColumnsSQLPart(edg_wll_Context ctx,
					void *job_index_cols_v,
					intJobStat *stat,
					int is_insert,
					char **names_out,
					char **values_out)
{
	int i;
	char *names, *values;
	char *data;
	char *tmp;
	char *tmpval;
	edg_wll_IColumnRec *job_index_cols = (edg_wll_IColumnRec *)job_index_cols_v;

	edg_wll_ResetError(ctx);

	if (is_insert) names = strdup(""); else names = NULL;
	values = strdup("");

	if (job_index_cols != NULL)
	for (i=0; job_index_cols[i].colname; i++) {
		data = NULL;
		switch (job_index_cols[i].qrec.attr) {
			case EDG_WLL_QUERY_ATTR_OWNER:
				if (stat->pub.owner) {
					tmpval = edg_wll_gss_normalize_subj(stat->pub.owner, 0);
					trio_asprintf(&data, "'%|Ss'", tmpval);
					free(tmpval);
				} else data = strdup("''");
				break;
			case EDG_WLL_QUERY_ATTR_LOCATION:
				if (stat->pub.location)
					trio_asprintf(&data, "'%|Ss'", stat->pub.location);
				else data = strdup("''");
				break;
			case EDG_WLL_QUERY_ATTR_DESTINATION:
				if (stat->pub.destination)
					trio_asprintf(&data, "'%|Ss'", stat->pub.destination);
				else data = strdup("''");
				break;
			case EDG_WLL_QUERY_ATTR_DONECODE:
				asprintf(&data, "%d", stat->pub.done_code);
				break;
			case EDG_WLL_QUERY_ATTR_USERTAG:
				if (stat->pub.user_tags) {
					int k;
					for (k=0; stat->pub.user_tags[k].tag &&
							strcmp(stat->pub.user_tags[k].tag,job_index_cols[i].qrec.attr_id.tag);
						k++);
					if (stat->pub.user_tags[k].tag != NULL) {
						trio_asprintf(&data, "'%|Ss'", stat->pub.user_tags[k].value);
					} else data = strdup("''");
				} else data = strdup("''");
				break;
			case EDG_WLL_QUERY_ATTR_TIME:
				if (stat->pub.stateEnterTimes)
					glite_lbu_TimeToDB(stat->pub.stateEnterTimes[job_index_cols[i].qrec.attr_id.state+1], &data);
				else data = strdup("0");
				break;
			case EDG_WLL_QUERY_ATTR_RESUBMITTED:
				asprintf(&data, "%d", stat->pub.resubmitted);
				break;
			case EDG_WLL_QUERY_ATTR_STATEENTERTIME:
				glite_lbu_TimeToDB(stat->pub.stateEnterTime.tv_sec, &data);
				break;
			case EDG_WLL_QUERY_ATTR_LASTUPDATETIME:
				glite_lbu_TimeToDB(stat->pub.lastUpdateTime.tv_sec, &data);
				break;
			case EDG_WLL_QUERY_ATTR_JDL_ATTR: // XXX: It's not clear how this is supposed to work
				if (stat->pub.jdl)
					trio_asprintf(&data, "'%|Ss'", stat->pub.destination);
				else data = strdup("''");
				break;
/*			case EDG_WLL_QUERY_ATTR_STATEENTERTIME: /// XXX: Which way of handling this is correct?
				if (stat->pub.stateEnterTime)
					glite_lbu_TimeToDB(stat->pub.stateEnterTime, &data);
				else data = strdup("0");
				break;
			case EDG_WLL_QUERY_ATTR_LASTUPDATETIME:
				if (stat->pub.lastUpdateTime)
					glite_lbu_TimeToDB(stat->pub.lastUpdateTime, &data);
				else data = strdup("0");
				break;*/

				/* XXX add more attributes when defined */
			default:
				/* do not use */
				break;
		}

		if (!data) continue;

		if (is_insert) {
			asprintf(&tmp, "%s,`%s`", names, job_index_cols[i].colname);
			free(names); names = tmp;
			asprintf(&tmp, "%s,%s", values, data);
			free(values); values = tmp;
		} else {
			/* update */
			asprintf(&tmp, "%s,`%s`=%s", values, job_index_cols[i].colname, data);
			free(values); values = tmp;
		}
		free(data);
	}

	if (is_insert) *names_out = names;
	*values_out = values;

	return edg_wll_Error(ctx, NULL, NULL);
}


int component_seqcode(const char *a, edg_wll_Source index)
{
	unsigned int    c[EDG_WLL_SOURCE__LAST];
	int		res;
	char		sc[EDG_WLL_SEQ_SIZE];

	if (!strstr(a, "LBS")) snprintf(sc,EDG_WLL_SEQ_SIZE,"%s:LBS=000000",a);
	else snprintf(sc,EDG_WLL_SEQ_SIZE,"%s",a);

	res =  sscanf(sc, "UI=%d:NS=%d:WM=%d:BH=%d:JSS=%d:LM=%d:LRMS=%d:APP=%d:LBS=%d",
			&c[EDG_WLL_SOURCE_USER_INTERFACE],
			&c[EDG_WLL_SOURCE_NETWORK_SERVER],
			&c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&c[EDG_WLL_SOURCE_BIG_HELPER],
			&c[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&c[EDG_WLL_SOURCE_LOG_MONITOR],
			&c[EDG_WLL_SOURCE_LRMS],
			&c[EDG_WLL_SOURCE_APPLICATION],
			&c[EDG_WLL_SOURCE_LB_SERVER]);
	if (res != EDG_WLL_SOURCE__LAST-1) {
		syslog(LOG_ERR, "unparsable sequence code %s\n", sc);
		fprintf(stderr, "unparsable sequence code %s\n", sc);
		return -1;
	}

	return(c[index]);	
}

char * set_component_seqcode(char *a,edg_wll_Source index,int val)
{
	unsigned int    c[EDG_WLL_SOURCE__LAST];
	int		res;
	char 		*ret;
	char		sc[EDG_WLL_SEQ_SIZE];

	if (!strstr(a, "LBS")) snprintf(sc,EDG_WLL_SEQ_SIZE,"%s:LBS=000000",a);
	else snprintf(sc,EDG_WLL_SEQ_SIZE,"%s",a);

	res =  sscanf(sc, "UI=%d:NS=%d:WM=%d:BH=%d:JSS=%d:LM=%d:LRMS=%d:APP=%d:LBS=%d",
			&c[EDG_WLL_SOURCE_USER_INTERFACE],
			&c[EDG_WLL_SOURCE_NETWORK_SERVER],
			&c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&c[EDG_WLL_SOURCE_BIG_HELPER],
			&c[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&c[EDG_WLL_SOURCE_LOG_MONITOR],
			&c[EDG_WLL_SOURCE_LRMS],
			&c[EDG_WLL_SOURCE_APPLICATION],
			&c[EDG_WLL_SOURCE_LB_SERVER]);
	if (res != EDG_WLL_SOURCE__LAST-1) {
		syslog(LOG_ERR, "unparsable sequence code %s\n", sc);
		fprintf(stderr, "unparsable sequence code %s\n", sc);
		return NULL;
	}

	c[index] = val;
	trio_asprintf(&ret,"UI=%06d:NS=%010d:WM=%06d:BH=%010d:JSS=%06d"
                                ":LM=%06d:LRMS=%06d:APP=%06d:LBS=%06d",
                        c[EDG_WLL_SOURCE_USER_INTERFACE],
                        c[EDG_WLL_SOURCE_NETWORK_SERVER],
                        c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
                        c[EDG_WLL_SOURCE_BIG_HELPER],
                        c[EDG_WLL_SOURCE_JOB_SUBMISSION],
                        c[EDG_WLL_SOURCE_LOG_MONITOR],
                        c[EDG_WLL_SOURCE_LRMS],
                        c[EDG_WLL_SOURCE_APPLICATION],
                        c[EDG_WLL_SOURCE_LB_SERVER]);
	return ret;
}

int before_deep_resubmission(const char *a, const char *b)
{
	if (component_seqcode(a, EDG_WLL_SOURCE_WORKLOAD_MANAGER) < 
	    component_seqcode(b, EDG_WLL_SOURCE_WORKLOAD_MANAGER) )
		return(1);
	else
		return(0);

}

int same_branch(const char *a, const char *b) 
{
	if (component_seqcode(a, EDG_WLL_SOURCE_WORKLOAD_MANAGER) == 
	    component_seqcode(b, EDG_WLL_SOURCE_WORKLOAD_MANAGER) )
		return(1);
	else
		return(0);
}

int edg_wll_compare_pbs_seq(const char *a,const char *b)
{
	char	timestamp_a[14], pos_a[10], src_a;
	char	timestamp_b[14], pos_b[10], src_b;
	int	ev_code_a, ev_code_b;
	int	res;

	res = sscanf(a,"TIMESTAMP=%14s:POS=%10s:EV.CODE=%3d:SRC=%c", timestamp_a, pos_a, &ev_code_a, &src_a);	

	if (res != 4) {
		syslog(LOG_ERR, "unparsable sequence code %s\n", a);
		fprintf(stderr, "unparsable sequence code %s\n", a);
		return -1;
	}

	res = sscanf(b,"TIMESTAMP=%14s:POS=%10s:EV.CODE=%3d:SRC=%c", timestamp_b, pos_b, &ev_code_b, &src_b);	

	if (res != 4) {
		syslog(LOG_ERR, "unparsable sequence code %s\n", b);
		fprintf(stderr, "unparsable sequence code %s\n", b);
		return -1;
	}

	/* wild card for PBSJobReg - this event should always come as firt one 		*/
	/* bacause it hold job.type, which is necessary for further event processing	*/
	if (ev_code_a == EDG_WLL_EVENT_REGJOB) return -1;
	if (ev_code_b == EDG_WLL_EVENT_REGJOB) return 1;

	/* sort event w.t.r. to timestamps */
	if ((res = strcmp(timestamp_a,timestamp_b)) != 0) {
		return res;
	}
	else {
		/* if timestamps equal, sort if w.t.r. to file position */
		/* if you both events come from the same log file	*/
		if (src_a == src_b) {
			/* zero mean in fact duplicate events in log	*/
			return strcmp(pos_a,pos_b);
		}
		/* if the events come from diffrent log files		*/
		/* it is possible to prioritize some src log file	*/
		else	{
			/* prioritize events from pbs_mom */
			if (src_a == 'm') return 1;
			if (src_b == 'm') return -1;

			/* then prioritize events from pbs_server */
			if (src_a == 's') return 1;
			if (src_b == 's') return -1;

			/* other priorities comes here... */
		}	
	}

	return 0;
}

edg_wll_PBSEventSource get_pbs_event_source(const char *pbs_seq_num) {
	switch (pbs_seq_num[EDG_WLL_SEQ_PBS_SIZE - 2]) {
		case 'c': return(EDG_WLL_PBS_EVENT_SOURCE_SCHEDULER);
		case 's': return(EDG_WLL_PBS_EVENT_SOURCE_SERVER);
		case 'm': return(EDG_WLL_PBS_EVENT_SOURCE_MOM);
		case 'a': return(EDG_WLL_PBS_EVENT_SOURCE_ACCOUNTING);
		default: return(EDG_WLL_PBS_EVENT_SOURCE_UNDEF);
	}
}

edg_wll_CondorEventSource get_condor_event_source(const char *condor_seq_num) {
	switch (condor_seq_num[EDG_WLL_SEQ_CONDOR_SIZE - 2]) {
		case 'L': return(EDG_WLL_CONDOR_EVENT_SOURCE_COLLECTOR);
		case 'M': return(EDG_WLL_CONDOR_EVENT_SOURCE_MASTER);
		case 'm': return(EDG_WLL_CONDOR_EVENT_SOURCE_MATCH);
		case 'N': return(EDG_WLL_CONDOR_EVENT_SOURCE_NEGOTIATOR);
		case 'C': return(EDG_WLL_CONDOR_EVENT_SOURCE_SCHED);
		case 'H': return(EDG_WLL_CONDOR_EVENT_SOURCE_SHADOW);
		case 's': return(EDG_WLL_CONDOR_EVENT_SOURCE_STARTER);
		case 'S': return(EDG_WLL_CONDOR_EVENT_SOURCE_START);
		case 'j': return(EDG_WLL_CONDOR_EVENT_SOURCE_JOBQUEUE);
		default: return(EDG_WLL_CONDOR_EVENT_SOURCE_UNDEF);
	}
}

int edg_wll_compare_seq(const char *a, const char *b)
{
	unsigned int    c[EDG_WLL_SOURCE__LAST];
	unsigned int    d[EDG_WLL_SOURCE__LAST];
	int		res, i;
	char		sca[EDG_WLL_SEQ_SIZE], scb[EDG_WLL_SEQ_SIZE];


	if ( (strstr(a,"TIMESTAMP=") == a) && (strstr(b,"TIMESTAMP=") == b) ) 
		return edg_wll_compare_pbs_seq(a,b);

	if (!strstr(a, "LBS")) snprintf(sca,EDG_WLL_SEQ_SIZE,"%s:LBS=000000",a);
	else snprintf(sca,EDG_WLL_SEQ_SIZE,"%s",a);
	if (!strstr(b, "LBS")) snprintf(scb,EDG_WLL_SEQ_SIZE,"%s:LBS=000000",b);
	else snprintf(scb,EDG_WLL_SEQ_SIZE,"%s",b);

	assert(EDG_WLL_SOURCE__LAST == 10);

	res =  sscanf(sca, "UI=%d:NS=%d:WM=%d:BH=%d:JSS=%d:LM=%d:LRMS=%d:APP=%d:LBS=%d",
			&c[EDG_WLL_SOURCE_USER_INTERFACE],
			&c[EDG_WLL_SOURCE_NETWORK_SERVER],
			&c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&c[EDG_WLL_SOURCE_BIG_HELPER],
			&c[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&c[EDG_WLL_SOURCE_LOG_MONITOR],
			&c[EDG_WLL_SOURCE_LRMS],
			&c[EDG_WLL_SOURCE_APPLICATION],
			&c[EDG_WLL_SOURCE_LB_SERVER]);
	if (res != EDG_WLL_SOURCE__LAST-1) {
		syslog(LOG_ERR, "unparsable sequence code %s\n", sca);
		fprintf(stderr, "unparsable sequence code %s\n", sca);
		return -1;
	}

	res =  sscanf(scb, "UI=%d:NS=%d:WM=%d:BH=%d:JSS=%d:LM=%d:LRMS=%d:APP=%d:LBS=%d",
			&d[EDG_WLL_SOURCE_USER_INTERFACE],
			&d[EDG_WLL_SOURCE_NETWORK_SERVER],
			&d[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&d[EDG_WLL_SOURCE_BIG_HELPER],
			&d[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&d[EDG_WLL_SOURCE_LOG_MONITOR],
			&d[EDG_WLL_SOURCE_LRMS],
			&d[EDG_WLL_SOURCE_APPLICATION],
			&d[EDG_WLL_SOURCE_LB_SERVER]);
	if (res != EDG_WLL_SOURCE__LAST-1) {
		syslog(LOG_ERR, "unparsable sequence code %s\n", scb);
		fprintf(stderr, "unparsable sequence code %s\n", scb);
		return 1;
	}

	for (i = EDG_WLL_SOURCE_USER_INTERFACE ; i < EDG_WLL_SOURCE__LAST; i++) {
		if (c[i] < d[i]) return -1;
		if (c[i] > d[i]) return  1;
	}

	return 0;
}

static int compare_events_by_seq(const void *a, const void *b)
{
        const edg_wll_Event *e = (edg_wll_Event *) a;
        const edg_wll_Event *f = (edg_wll_Event *) b;
	int ret;


	ret = edg_wll_compare_seq(e->any.seqcode, f->any.seqcode);
	if (ret) return ret;
	
	if (e->any.timestamp.tv_sec < f->any.timestamp.tv_sec) return -1;
	if (e->any.timestamp.tv_sec > f->any.timestamp.tv_sec) return 1;
	if (e->any.timestamp.tv_usec < f->any.timestamp.tv_usec) return -1;
	if (e->any.timestamp.tv_usec > f->any.timestamp.tv_usec) return 1;
	return 0;
}

static int compare_pevents_by_seq(const void *a, const void *b)
{
        const edg_wll_Event **e = (const edg_wll_Event **) a;
        const edg_wll_Event **f = (const edg_wll_Event **) b;
	return compare_events_by_seq(*e,*f);
}

void edg_wll_SortEvents(edg_wll_Event *e)
{
	int	n;

	if (!e) return;
	for (n=0; e[n].type; n++);
	qsort(e,n,sizeof(*e),compare_events_by_seq);
}

void edg_wll_SortPEvents(edg_wll_Event **e)
{
	edg_wll_Event **p;
	int	n;

	if (!e) return;
	p = e;
	for (n=0; *p; n++) {
		p++;
	}
	qsort(e,n,sizeof(*e),compare_pevents_by_seq);
}


void init_intJobStat(intJobStat *p)
{
	memset(p, 0, sizeof(intJobStat));
	p->pub.jobtype = -1 /* why? EDG_WLL_STAT_SIMPLE */;
	p->pub.children_hist = (int*) calloc(1+EDG_WLL_NUMBER_OF_STATCODES, sizeof(int));
	p->pub.children_hist[0] = EDG_WLL_NUMBER_OF_STATCODES;
	p->pub.stateEnterTimes = (int*) calloc(1+EDG_WLL_NUMBER_OF_STATCODES, sizeof(int));
	p->pub.stateEnterTimes[0] = EDG_WLL_NUMBER_OF_STATCODES;
	/* TBD: generate */
}

