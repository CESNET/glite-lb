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

#include "glite/wmsutils/jobid/cjobid.h"
#include "glite/lb/producer.h"
#include "glite/lb/context-int.h"
#include "glite/lb/trio.h"

#include "store.h"
#include "index.h"
#include "jobstat.h"
#include "lbs_db.h"
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
	char *tmp_in, *tmp_ret, *tmp_ret2;
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
		tmp_ret2 = dec_string(tmp_in, &tmp_in);
		if (!tmp_in) { *rest = tmp_in; return NULL; }
		tmp_ret = dec_string(tmp_in, &tmp_in);
		if (!tmp_in) { *rest = tmp_in; return NULL; }
		len++;
	}  while (tmp_ret2 != NULL);

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
		/* children data are not stored in DB */
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
                /* children data are not stored in DB */
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
	
	*rest = tmp_in;
	return stat;
}

char *enc_intJobStat(char *old, intJobStat* stat)
{
	char *ret;

	ret = enc_JobStat(old, &stat->pub);
	if (ret) ret = enc_int(ret, stat->wontresub);
	if (ret) ret = enc_string(ret, stat->last_seqcode);
	if (ret) ret = enc_string(ret, stat->last_cancel_seqcode);
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
		stat->wontresub = dec_int(tmp_in, &tmp_in);
		if (tmp_in != NULL) {
			stat->last_seqcode = dec_string(tmp_in, &tmp_in);
		}
		if (tmp_in != NULL) {
			stat->last_cancel_seqcode = dec_string(tmp_in, &tmp_in);
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
	edg_wll_IColumnRec *job_index_cols = (edg_wll_IColumnRec *)job_index_cols_v;

	edg_wll_ResetError(ctx);

	if (is_insert) names = strdup(""); else names = NULL;
	values = strdup("");

	if (job_index_cols != NULL)
	for (i=0; job_index_cols[i].colname; i++) {
		data = NULL;
		switch (job_index_cols[i].qrec.attr) {
			case EDG_WLL_QUERY_ATTR_OWNER:
				if (stat->pub.owner)
					trio_asprintf(&data, "'%|Ss'", stat->pub.owner);
				else data = strdup("''");
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
					data = strdup(edg_wll_TimeToDB(stat->pub.stateEnterTimes[
							job_index_cols[i].qrec.attr_id.state+1]));
				else data = strdup("0");
				break;
			case EDG_WLL_QUERY_ATTR_RESUBMITTED:
				asprintf(&data, "%d", stat->pub.resubmitted);
				break;

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


/*
 * Set values of index columns in state table (after index reconfiguration)
 */

edg_wll_ErrorCode edg_wll_RefreshIColumns(edg_wll_Context ctx, void *job_index_cols) {

	edg_wll_Stmt sh, sh2;
	int njobs, ret = -1;
	intJobStat *stat;
	edg_wlc_JobId jobid;
	char *res[5];
	char *rest;
	char *icvalues, *stmt;
	int i;

	edg_wll_ResetError(ctx);
	if (!job_index_cols) return 0;

	if ((njobs = edg_wll_ExecStmt(ctx, "select s.jobid,s.int_status,s.seq,s.version,j.dg_jobid"
				       " from states s, jobs j where s.jobid=j.jobid",&sh)) < 0) {
		edg_wll_FreeStmt(&sh);
		return edg_wll_Error(ctx, NULL, NULL);
	}
	while ((ret=edg_wll_FetchRow(sh,res)) >0) {
		if (strcmp(res[3], INTSTAT_VERSION)) {
			stat = NULL;
			if (!edg_wlc_JobIdParse(res[4], &jobid)) {
				if ((stat = malloc(sizeof(intJobStat))) != NULL) {
					if (edg_wll_intJobStatus(ctx, jobid, 0, stat, 1)) {
						free(stat);
						stat = NULL;
					}
				}
				edg_wlc_JobIdFree(jobid);
			}
		} else {
			stat = dec_intJobStat(res[1], &rest);
			if (rest == NULL) stat = NULL;
		}
		if (stat == NULL) {
			edg_wll_FreeStmt(&sh);
			return edg_wll_SetError(ctx, EDG_WLL_ERROR_SERVER_RESPONSE,
				"cannot decode int_status from states DB table");
		}

		edg_wll_IColumnsSQLPart(ctx, job_index_cols, stat, 0, NULL, &icvalues);
		trio_asprintf(&stmt, "update states set seq=%s%s where jobid='%|Ss'", res[2], icvalues, res[0]);
		ret = edg_wll_ExecStmt(ctx, stmt, &sh2);
		edg_wll_FreeStmt(&sh2);

		for (i = 0; i < 5; i++) free(res[i]);
		destroy_intJobStat(stat); free(stat);
		free(stmt); free(icvalues);

		if (ret < 0) return edg_wll_Error(ctx, NULL, NULL);
		
	}
	edg_wll_FreeStmt(&sh);
	return edg_wll_Error(ctx, NULL, NULL);
}


int edg_wll_compare_seq(const char *a, const char *b)
{
	unsigned int    c[EDG_WLL_SOURCE__LAST];
	unsigned int    d[EDG_WLL_SOURCE__LAST];
	int		res, i;

	assert(EDG_WLL_SOURCE__LAST == 9);

	res =  sscanf(a, "UI=%d:NS=%d:WM=%d:BH=%d:JSS=%d:LM=%d:LRMS=%d:APP=%d",
			&c[EDG_WLL_SOURCE_USER_INTERFACE],
			&c[EDG_WLL_SOURCE_NETWORK_SERVER],
			&c[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&c[EDG_WLL_SOURCE_BIG_HELPER],
			&c[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&c[EDG_WLL_SOURCE_LOG_MONITOR],
			&c[EDG_WLL_SOURCE_LRMS],
			&c[EDG_WLL_SOURCE_APPLICATION]);
	if (res != EDG_WLL_SOURCE__LAST-1) {
		syslog(LOG_ERR, "unparsable sequence code %s\n", a);
		fprintf(stderr, "unparsable sequence code %s\n", a);
		return -1;
	}

	res =  sscanf(b, "UI=%d:NS=%d:WM=%d:BH=%d:JSS=%d:LM=%d:LRMS=%d:APP=%d",
			&d[EDG_WLL_SOURCE_USER_INTERFACE],
			&d[EDG_WLL_SOURCE_NETWORK_SERVER],
			&d[EDG_WLL_SOURCE_WORKLOAD_MANAGER],
			&d[EDG_WLL_SOURCE_BIG_HELPER],
			&d[EDG_WLL_SOURCE_JOB_SUBMISSION],
			&d[EDG_WLL_SOURCE_LOG_MONITOR],
			&d[EDG_WLL_SOURCE_LRMS],
			&d[EDG_WLL_SOURCE_APPLICATION]);
	if (res != EDG_WLL_SOURCE__LAST-1) {
		syslog(LOG_ERR, "unparsable sequence code %s\n", b);
		fprintf(stderr, "unparsable sequence code %s\n", b);
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
        const edg_wll_Event *e = (edg_wll_Event *)a;
        const edg_wll_Event *f = (edg_wll_Event *)b;

	return edg_wll_compare_seq(e->any.seqcode, f->any.seqcode);
}

void edg_wll_SortEvents(edg_wll_Event *e)
{
	int	n;

	if (!e) return;
	for (n=0; e[n].type; n++);
	qsort(e,n,sizeof *e,compare_events_by_seq);
}
