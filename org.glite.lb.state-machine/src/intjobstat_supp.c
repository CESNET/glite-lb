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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <regex.h>

#include "glite/jobid/cjobid.h"
#include "glite/lbu/trio.h"
#include "glite/lbu/db.h"
#include "glite/lb/context-int.h"
#include "intjobstat.h"
#include "seqcode_aux.h"

#include "glite/lb/jobstat.h"


/* TBD: share in whole logging or workload */

/* XXX - how come this is not generated from .T when the JobStat structure is? */

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

	/* count number of fields only */
	len = 0;
	tmp_in = in = strchr(in, ' ') + 1 ;
	do {
		tmp_ret = dec_string(tmp_in, &tmp_in);
		free(tmp_ret); 
		len++;
	}  while (tmp_ret != NULL);

	out = (char**) malloc(len*sizeof(char*));

	/* get them */
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
	else t.tv_usec = 0;
	*rest = tmp_in;
	return t;
}

char *enc_JobStat(char *old, edg_wll_JobStat* stat)
{
	char *ret;

	ret = enc_int(old, stat->state);
	if (ret) ret = enc_jobid(ret, stat->jobId);
	if (ret) ret = enc_string(ret, stat->owner);
	if (ret) ret = enc_int(ret, stat->jobtype);
	if (ret) ret = enc_jobid(ret, stat->parent_job);
	if (ret) ret = enc_string(ret, stat->seed);
	if (ret) ret = enc_int(ret, stat->children_num);
	if (ret) ret = enc_int_array(ret, stat->children_hist, EDG_WLL_NUMBER_OF_STATCODES);
		/* children histogram stored in the DB, other children data not stored. */
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
	if (ret) ret = enc_timeval(ret, stat->lastUpdateTime);
	if (ret) ret = enc_intlist(ret, stat->stateEnterTimes);
	if (ret) ret = enc_int(ret, stat->expectUpdate);
	if (ret) ret = enc_string(ret, stat->expectFrom);
	if (ret) ret = enc_string(ret, stat->acl);
	if (ret) ret = enc_int(ret, stat->payload_running);
	if (ret) ret = enc_strlist(ret, stat->possible_destinations);
	if (ret) ret = enc_strlist(ret, stat->possible_ce_nodes);
	if (ret) ret = enc_int(ret, stat->suspended);
	if (ret) ret = enc_string(ret, stat->suspend_reason);
	if (ret) ret = enc_string(ret, stat->failure_reasons);
	if (ret) ret = enc_int(ret, stat->remove_from_proxy);
	if (ret) ret = enc_string(ret, stat->ui_host);
	if (ret) ret = enc_strlist(ret, stat->user_fqans);
	if (ret) ret = enc_int(ret, stat->sandbox_retrieved);
	if (ret) ret = enc_int(ret, stat->jw_status);
	if (ret) ret = enc_jobid(ret, stat->isb_transfer);
	if (ret) ret = enc_jobid(ret, stat->osb_transfer);
	if (ret) ret = enc_string(ret, stat->payload_owner);
	if (ret) ret = enc_string(ret, stat->pbs_state);
	if (ret) ret = enc_string(ret, stat->pbs_queue);
	if (ret) ret = enc_string(ret, stat->pbs_owner);
	if (ret) ret = enc_string(ret, stat->pbs_name);
	if (ret) ret = enc_string(ret, stat->pbs_reason);
	if (ret) ret = enc_string(ret, stat->pbs_scheduler);
	if (ret) ret = enc_string(ret, stat->pbs_dest_host);
	if (ret) ret = enc_int(ret, stat->pbs_pid);
	if (ret) ret = enc_taglist(ret, stat->pbs_resource_requested);
	if (ret) ret = enc_taglist(ret, stat->pbs_resource_usage);
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
	if (ret) ret = enc_int(ret, stat->cream_state);
	if (ret) ret = enc_string(ret, stat->cream_id);
	if (ret) ret = enc_string(ret, stat->cream_owner);
	if (ret) ret = enc_string(ret, stat->cream_endpoint);
	if (ret) ret = enc_string(ret, stat->cream_jdl);
	if (ret) ret = enc_string(ret, stat->cream_reason);
	if (ret) ret = enc_string(ret, stat->cream_failure_reason);
	if (ret) ret = enc_string(ret, stat->cream_lrms_id);
	if (ret) ret = enc_string(ret, stat->cream_node);
	if (ret) ret = enc_int(ret, stat->cream_done_code);
	if (ret) ret = enc_int(ret, stat->cream_exit_code);
	if (ret) ret = enc_int(ret, stat->cream_cancelling);
	if (ret) ret = enc_int(ret, stat->cream_cpu_time);
	if (ret) ret = enc_int(ret, stat->cream_jw_status);
	if (ret) ret = enc_jobid(ret, stat->ft_compute_job);
	if (ret) ret = enc_int(ret, stat->ft_sandbox_type);
	if (ret) ret = enc_string(ret, stat->ft_src);
	if (ret) ret = enc_string(ret, stat->ft_dest);
	if (ret) ret = enc_int(ret, stat->vm_state);
	if (ret) ret = enc_string(ret, stat->vm_image);
	if (ret) ret = enc_string(ret, stat->vm_require);
	if (ret) ret = enc_string(ret, stat->vm_usage);
	if (ret) ret = enc_string(ret, stat->vm_hostname);
	if (ret) ret = enc_string(ret, stat->vm_machine);

	return ret;
}
edg_wll_JobStat* dec_JobStat(char *in, char **rest)
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
        if (tmp_in != NULL) {
			    stat->children_hist = (int*)calloc(EDG_WLL_NUMBER_OF_STATCODES+1, sizeof(int));
			    dec_int_array(tmp_in, &tmp_in, stat->children_hist);
	}
                /* children histogram stored in the DB, other children data not stored. */
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
        if (tmp_in != NULL) stat->lastUpdateTime = dec_timeval(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->stateEnterTimes = dec_intlist(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->expectUpdate = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->expectFrom = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->acl = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->payload_running = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->possible_destinations = dec_strlist(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->possible_ce_nodes = dec_strlist(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->suspended = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->suspend_reason = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->failure_reasons = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->remove_from_proxy = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->ui_host = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->user_fqans = dec_strlist(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->sandbox_retrieved = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->jw_status = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->isb_transfer = dec_jobid(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->osb_transfer = dec_jobid(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->payload_owner = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_state = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_queue = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_owner = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_name = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_reason = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_scheduler = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_dest_host = dec_string(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_pid = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_resource_requested = dec_taglist(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->pbs_resource_usage = dec_taglist(tmp_in, &tmp_in);
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
	if (tmp_in != NULL) stat->cream_state = dec_int(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->cream_id = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->cream_owner = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->cream_endpoint = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->cream_jdl = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->cream_reason = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->cream_failure_reason = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->cream_lrms_id = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->cream_node = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->cream_done_code = dec_int(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->cream_exit_code = dec_int(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->cream_cancelling = dec_int(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->cream_cpu_time = dec_int(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->cream_jw_status = dec_int(tmp_in, &tmp_in);
        if (tmp_in != NULL) stat->ft_compute_job = dec_jobid(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->ft_sandbox_type = dec_int(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->ft_src = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->ft_dest = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->vm_state = dec_int(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->vm_image = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->vm_require = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->vm_usage = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->vm_hostname = dec_string(tmp_in, &tmp_in);
	if (tmp_in != NULL) stat->vm_machine = dec_string(tmp_in, &tmp_in);

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
	if (ret) ret = enc_strlist(ret, stat->tag_seq_codes);
	if (ret) ret = enc_string(ret, stat->payload_owner_pending);
	if (ret) ret = enc_string(ret, stat->payload_owner_unconfirmed);
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
		if (tmp_in != NULL) {
			stat->tag_seq_codes = dec_strlist(tmp_in, &tmp_in);
		}
		if (tmp_in != NULL) {
			stat->payload_owner_pending = dec_string(tmp_in, &tmp_in);
		}
		if (tmp_in != NULL) {
			stat->payload_owner_unconfirmed = dec_string(tmp_in, &tmp_in);
		}
	} else if (tmp_in != NULL) {
		edg_wll_FreeStatus(pubstat);
		free(pubstat);
	}

	*rest = tmp_in;
	return stat;
}

edg_wll_JobStat* intJobStat_to_JobStat(intJobStat* intStat){
	return &(intStat->pub);
}

char* intJobStat_getLastSeqcode(intJobStat* intStat){
	return intStat->last_seqcode;
}

void intJobStat_nullLastSeqcode(intJobStat* intStat){
        intStat->last_seqcode = NULL;
}

