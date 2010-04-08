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


#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "glite/lb/events.h"
#include "glite/lb/jobstat.h"
#include "glite/lb/context-int.h"

#include "glite/lbu/log.h"

#include "glite/jobid/strmd5.h"

#include "stats.h"
#include "authz_policy.h"
static int stats_inc_counter(edg_wll_Context,const edg_wll_JobStat *,edg_wll_Stats *);
static int stats_record_duration(edg_wll_Context,const edg_wll_JobStat *,const edg_wll_JobStat *,edg_wll_Stats *);

#define dprintf(x) { printf("[%d] ",getpid()); printf x; }

/* XXX: should be configurable at run time */
static struct _edg_wll_StatsArchive default_archives[] = { 
	{ 10, 60 },
	{ 60, 30 },
	{ 900, 12 },
	{ 0, 0 }
};

static edg_wll_QueryRec default_group[2] = { 
	{ EDG_WLL_QUERY_ATTR_DESTINATION, },
	{ EDG_WLL_QUERY_ATTR_UNDEF, }
};

static edg_wll_Stats default_stats[] = {
	{ STATS_COUNT, default_group, EDG_WLL_JOB_READY, 0, default_archives },
	{ STATS_COUNT, default_group, EDG_WLL_JOB_SCHEDULED, 0, default_archives },
	{ STATS_COUNT, default_group, EDG_WLL_JOB_RUNNING, 0, default_archives },
	{ STATS_COUNT, default_group, EDG_WLL_JOB_DONE, EDG_WLL_STAT_OK, default_archives },
	{ STATS_COUNT, default_group, EDG_WLL_JOB_DONE, EDG_WLL_STAT_FAILED, default_archives },
	{ STATS_DURATION, default_group, EDG_WLL_JOB_SCHEDULED, 0, default_archives },
	{ STATS_DURATION, default_group, EDG_WLL_JOB_RUNNING, 0, default_archives },
	{ STATS_UNDEF, }
	
};

extern int debug;

int edg_wll_InitStatistics(edg_wll_Context ctx)
{
	edg_wll_Stats *stats = default_stats; 	/* XXX: hardcoded */
	int	i,j;

	for (i=0; stats[i].type; i++) {
		char	fname[50],*zero;

		strcpy(fname,"/tmp/lb_stats.XXXXXX");
		stats[i].fd = mkstemp(fname);
		if (stats[i].fd < 0) return edg_wll_SetError(ctx,errno,fname);
		/* XXX: should be initialized from LB data */
		stats[i].grpno = 0;

		stats[i].grpsize = sizeof(struct edg_wll_stats_group)-sizeof(struct edg_wll_stats_archive);

		for (j=0; stats[i].archives[j].interval; j++) {
			stats[i].grpsize += sizeof(struct edg_wll_stats_archive)-sizeof(struct edg_wll_stats_cell);
			stats[i].grpsize += stats[i].archives[j].length * sizeof(struct edg_wll_stats_cell);
		}
		zero = calloc(1,stats[i].grpsize);
		write(stats[i].fd,zero,stats[i].grpsize);
		stats[i].map = mmap(NULL,stats[i].grpsize,PROT_READ|PROT_WRITE,MAP_SHARED,stats[i].fd,0);
		if (stats[i].map == MAP_FAILED) return edg_wll_SetError(ctx,errno,"mmap()");

		glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG, 
			"stats: using %s",fname);
		unlink(fname);
	}
	return 0;
}

int edg_wll_UpdateStatistics(
	edg_wll_Context ctx,
	const edg_wll_JobStat *from,
	const edg_wll_Event *e,
	const edg_wll_JobStat *to)
{
	int	i;

	/* XXX: should be propagated somhow via context, not hardcoded */
	edg_wll_Stats	*stats = default_stats;

	if (!ctx->count_statistics) return 0;

	for (i=0; stats[i].type; i++) switch (stats[i].type) {
		case STATS_COUNT:
			if (!to) continue;
			if (to->state == stats[i].major && (!from || from->state != to->state)) {
				switch (to->state) {
					case EDG_WLL_JOB_DONE:
						if (to->done_code != stats[i].minor) continue;
						break;
					default: break;
				}
				stats_inc_counter(ctx,to,stats+i);
			}
			break;
		case STATS_DURATION:
			if (!to || !from) continue;
			if (from->state == stats[i].major && from->state != to->state) 
				stats_record_duration(ctx,from,to,stats+i);
		default: break;
	}
	return 0;
}

static struct edg_wll_stats_archive *archive_skip(
		const struct edg_wll_stats_archive *a,
		int	len)
{
	return (struct edg_wll_stats_archive *) 
			(((char *) a) + sizeof(struct edg_wll_stats_archive)
			 	- sizeof(struct edg_wll_stats_cell)
				+ len * sizeof(struct edg_wll_stats_cell)
			);
}

static int stats_remap(edg_wll_Stats *stats)
{
	int newgrpno = stats->map->grpno;
	glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG,
		"stats_remap: size changed (%d != %d), remap",
		stats->grpno, newgrpno);
	munmap(stats->map,(stats->grpno ? stats->grpno : 1) * stats->grpsize);
	stats->map = mmap(NULL,newgrpno * stats->grpsize,
			PROT_READ|PROT_WRITE,MAP_SHARED,stats->fd,0);
	if (stats->map == MAP_FAILED) {
		if (debug) abort();
		return -1;
	}
	assert(stats->map->grpno == newgrpno);
	stats->grpno = newgrpno;
	return 0;
}


static int stats_inc_counter(edg_wll_Context ctx,const edg_wll_JobStat *jobstat,edg_wll_Stats *stats)
{
	int	i,j;
	char	*sig = NULL;
	struct edg_wll_stats_group	*g;
	struct edg_wll_stats_archive	*a;
	time_t	now = jobstat->stateEnterTime.tv_sec;

	/* XXX: we support destination grouping only */
	if (!jobstat->destination) return 0;
	edg_wll_ResetError(ctx);

	glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG,
		"inc_counter: destination %s, stats %d",
		jobstat->destination, (int) (stats - (edg_wll_Stats *) default_stats));

	if (flock(stats->fd,LOCK_EX)) return edg_wll_SetError(ctx,errno,"flock()");

	/* remap the file if someone changed its size */
	if (stats->map->grpno != stats->grpno && stats_remap(stats)) {
		edg_wll_SetError(ctx,errno,"shmem remap failed");
		goto cleanup;
	}

	sig = str2md5base64(jobstat->destination);

	for (i=0; i<stats->grpno; i++) {
		g = (struct edg_wll_stats_group *) (
				((char *) stats->map) + stats->grpsize * i
			);
		if (!strcmp(sig,g->sig)) break;
	}

	/* not found, initialize new */
	if (i == stats->grpno) {
		glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG,
			"group %s not found",sig);
		if (stats->grpno) {
			char	*zero = calloc(1,stats->grpsize);
			munmap(stats->map,stats->grpno * stats->grpsize);
			lseek(stats->fd,0,SEEK_END);
			write(stats->fd,zero,stats->grpsize);
			free(zero);
			stats->map = mmap(NULL,(stats->grpno+1) * stats->grpsize,
					PROT_READ|PROT_WRITE,MAP_SHARED,stats->fd,0);

			if (stats->map == MAP_FAILED) {
				edg_wll_SetError(ctx,errno,"mmap()");
				goto cleanup;
			}
		}
		stats->grpno++;
		stats->map->grpno++;
		glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG,
			"allocated");

		g = (struct edg_wll_stats_group *) (
				((char *) stats->map) + stats->grpsize * i);

		/* invalidate all cells in all archives */
		a = g->archive;
		for (i=0; stats->archives[i].interval; i++) {
			for (j=0; j<stats->archives[i].length; j++) a->cells[j].cnt = -1;
			a = archive_skip(a,stats->archives[i].length);
		}

		strcpy(g->sig,sig);
		g->last_update = now;
	}
	else 
		glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG,
			"group %s found at %d", sig, i);

	a = g->archive;
	for (i=0; stats->archives[i].interval; i++) {
		time_t	pt = g->last_update;

		pt -= pt % stats->archives[i].interval;

		/* nothing happened longer than is history of this archive */
		if (now-pt > stats->archives[i].interval * stats->archives[i].length) {
			for (j=0; j<stats->archives[i].length; j++) a->cells[j].cnt = 0;
			a->ptr = 0;
		}
		/* catch up eventually, cleaning not touched cells */
		else for (pt += stats->archives[i].interval; pt < now;
				pt += stats->archives[i].interval)
		{
			if (++(a->ptr) == stats->archives[i].length) a->ptr = 0;
			a->cells[a->ptr].cnt = 0;
		}

		/* validate an unused cell */
		if (a->cells[a->ptr].cnt < 0) a->cells[a->ptr].cnt = 0;

		/* now we can do IT */
		a->cells[a->ptr].cnt++;
		glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG,
			"update archive %d, cell %d to %d",
			i, a->ptr, a->cells[a->ptr].cnt);

		/* go to next archive */
		a = archive_skip(a,stats->archives[i].length);
	}

	g->last_update = now;


cleanup:
	free(sig);
	flock(stats->fd,LOCK_UN);
	return edg_wll_Error(ctx,NULL,NULL);
}

static int stats_record_duration(
		edg_wll_Context ctx,
		const edg_wll_JobStat *from,
		const edg_wll_JobStat *to,
		edg_wll_Stats *stats)
{
	return 0;
}

int edg_wll_StateRateServer(
	edg_wll_Context	ctx,
	const edg_wll_QueryRec	*group,
	edg_wll_JobStatCode	major,
	int			minor,
	time_t	*from, 
	time_t	*to,
	float	*rate,
	int	*res_from,
	int	*res_to
)
{
	edg_wll_Stats *stats = default_stats;   /* XXX: hardcoded */
	struct edg_wll_stats_group	*g;
	struct edg_wll_stats_archive	*a;
	int	i,j,matchi;
	char	*sig = NULL;
	time_t	afrom,ato;
	long	match;
	struct _edg_wll_GssPrincipal_data princ;

        princ.name = ctx->peerName;
        princ.fqans = ctx->fqans;

	edg_wll_ResetError(ctx);

	switch (ctx->count_statistics) {
		case 0: return edg_wll_SetError(ctx,ENOSYS,NULL);
		case 1: if (!ctx->noAuth && !check_authz_policy(&ctx->authz_policy, &princ, GET_STATISTICS)) return edg_wll_SetError(ctx,EPERM,NULL);
			break;
		case 2: break;
		default: abort();
	}

	if (group[0].attr != EDG_WLL_QUERY_ATTR_DESTINATION
		|| group[1].attr != EDG_WLL_QUERY_ATTR_UNDEF)
			return edg_wll_SetError(ctx,ENOSYS,
				"the only supported grouping is by destination");

	if (*from >= *to) return edg_wll_SetError(ctx,EINVAL,"from >= to");

	for (;stats->type; stats++) {
		if (stats->type != STATS_COUNT || stats->major != major) continue;
		switch (major) {
			case EDG_WLL_JOB_DONE:
				if (stats->minor != minor) continue;
				break;
			default: break;
		}
		break;
	}

	if (!stats->type) return edg_wll_SetError(ctx,ENOENT,"no matching state counter");

	/* remap the file if someone changed its size */
	if (stats->map->grpno != stats->grpno)
	{	
		if (flock(stats->fd,LOCK_EX)) return edg_wll_SetError(ctx,errno,"flock()");
		if (stats_remap(stats)) {
			edg_wll_SetError(ctx,errno,"shmem remap failed");
			goto cleanup;
		}
	}

	if (flock(stats->fd,LOCK_SH)) return edg_wll_SetError(ctx,errno,"flock()");

	/* XXX */
	sig = str2md5base64(group->value.c);


	for (i=0, g=stats->map; i<stats->grpno; i++) {
		if (!strcmp(sig,g->sig)) break;
		g = (struct edg_wll_stats_group *) (((char *) g) + stats->grpsize);
	}

	if (i == stats->grpno) {
		glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG, 
			"no match: %s\n",sig);
		edg_wll_SetError(ctx,ENOENT,"no matching group");
		goto cleanup;
	}

	match = 0;
	matchi = -1;
	/* XXX: assumes decreasing resolution of archives */ 
	for (j=0; stats->archives[j].interval; j++) {
		afrom = ato = g->last_update;

		ato += stats->archives[j].interval - ato % stats->archives[j].interval;
		afrom -= afrom % stats->archives[j].interval;
		afrom -= stats->archives[j].interval * (stats->archives[j].length-1);

		/* intersection of [from,to] and [afrom,ato] */
		if (afrom < *from) afrom = *from;
		if (ato > *to) ato = *to;

		/* find the best match */
		if (ato-afrom > match) {
			match = ato - afrom;
			matchi = j;
		}
	}

	glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG, 
		"best match: archive %d, interval %ld", matchi, match);

	if (matchi < 0) {
		if (*from > g->last_update) {
			/* special case -- we are sure that nothing arrived */
			*rate = 0;
			*res_from = *res_to = stats->archives[0].interval;
			goto cleanup;
		}
		edg_wll_SetError(ctx,ENOENT,"no data available");
		goto cleanup;
	}

	*res_from = *res_to = stats->archives[matchi].interval;

	a = g->archive;
	for (j=0; j<matchi; j++) a = archive_skip(a,stats->archives[j].length);

	i = stats->archives[matchi].interval;
	afrom = g->last_update - g->last_update % i
			- (stats->archives[matchi].length-1)*i;

	glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG,
		"archive from %ld = %s", afrom, ctime(&afrom));

	if (afrom > *from) *from = afrom;
	if (afrom + stats->archives[matchi].length * i < *to) *to = afrom + stats->archives[matchi].length * i;

	*rate = 0;
	match = 0;


	for (j=0; j<stats->archives[matchi].length; j++,afrom += i) {
		struct edg_wll_stats_cell 	*c = a->cells + ((a->ptr+j+1) % stats->archives[matchi].length);

		 glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG,
			"cell %d (abs %d): ",
			j, (a->ptr+j+1) % stats->archives[matchi].length);
		if (c->cnt < 0) {
			glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG, "invalid");
			continue; /* invalid cell */
		}

		glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG,
			"search %ld in %ld, %ld", *from, afrom, afrom+i);

		if (*from >= afrom && *from < afrom+i) {
			match += *from - afrom;
			*rate += c->cnt * (1.0 - ((float) *from-afrom)/i);
			glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG, "matched from: match %ld, rate %f", match, *rate);
		}
		else if (*from < afrom && *to >= afrom) {
			match += i;
			*rate += c->cnt;
			glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG, "matched in: match %ld, rate %f", match, *rate);
		}

		if (*to >= afrom && *to < afrom+i) {
			match -= i-(*to-afrom);
			*rate -= c->cnt * (((float) i)-(*to - afrom))/i;

			glite_common_log(LOG_CATEGORY_LB_SERVER, LOG_PRIORITY_DEBUG, "matched to: match %ld, rate %f", match, *rate);

		/* asi blbost 
			if (j == stats->archives[matchi].length - 1
				&& *to > g->last_update)
			{
				match -= *to - g->last_update;
				*rate -= c->cnt * (((float) *to) - g->last_update)/i;
				dprintf(("corrected wrt. last_update: match %ld, rate %f\n",match,*rate));
			}
		*/

			break;
		}
	}
	*rate /= match;

cleanup:
	free(sig);
	flock(stats->fd,LOCK_UN);
	return edg_wll_Error(ctx,NULL,NULL);
}

int edg_wll_StateDurationServer(
	edg_wll_Context	ctx,
	const edg_wll_QueryRec	*group,
	edg_wll_JobStatCode	major,
	int			minor,
	time_t	*from, 
	time_t	*to,
	float	*duration,
	int	*res_from,
	int	*res_to
)
{
	return edg_wll_SetError(ctx,ENOSYS,NULL);
}

