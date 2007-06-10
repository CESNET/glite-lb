#ifndef __GLITE_LB_STATS_H__
#define __GLITE_LB_STATS_H__

#ident "$Header$"

#include "glite/lb/context.h"
#include "glite/lb/events.h"
#include "glite/lb/jobstat.h"
#include "glite/lb/query_rec.h"

int edg_wll_InitStatistics(edg_wll_Context);

int edg_wll_UpdateStatistics(
	edg_wll_Context,
	const edg_wll_JobStat *,
	const edg_wll_Event *,
	const edg_wll_JobStat *);


struct edg_wll_stats_cell {
	int	cnt;
	float	value;
};

struct edg_wll_stats_archive {
	int	ptr;
	struct edg_wll_stats_cell	cells[1];
};

struct edg_wll_stats_group {
	int	grpno;
	char	sig[33];
	time_t	last_update;
	struct edg_wll_stats_archive	archive[1];
};


typedef struct {
	enum { STATS_UNDEF = 0, STATS_COUNT, STATS_DURATION }	type;
	edg_wll_QueryRec	*group;
	edg_wll_JobStatCode	major;
	int			minor;
	struct _edg_wll_StatsArchive {
		int	interval,length;
	} *archives;

	int	fd;
	struct edg_wll_stats_group	*map;
	int	grpno,grpsize;
} edg_wll_Stats;

int edg_wll_StateRateServer(
	edg_wll_Context	context,
	const edg_wll_QueryRec	*group,
	edg_wll_JobStatCode	major,
	int			minor,
	time_t	*from, 
	time_t	*to,
	float	*rate,
	int	*res_from,
	int	*res_to
);


int edg_wll_StateDurationServer(
	edg_wll_Context	context,
	const edg_wll_QueryRec	*group,
	edg_wll_JobStatCode	major,
	int			minor,
	time_t	*from, 
	time_t	*to,
	float	*duration,
	int	*res_from,
	int	*res_to
);

#endif /* __GLITE_LB_STATS_H__ */
