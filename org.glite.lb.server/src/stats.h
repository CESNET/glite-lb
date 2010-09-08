#ifndef GLITE_LB_STATS_H
#define GLITE_LB_STATS_H

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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include<search.h>

#include "glite/lb/context.h"
#include "glite/lb/events.h"
#include "glite/lb/jobstat.h"
#include "glite/lb/query_rec.h"

#define STATS_DEST_SIZE 256

int edg_wll_InitStatistics(edg_wll_Context);

int edg_wll_UpdateStatistics(
	edg_wll_Context,
	const edg_wll_JobStat *,
	const edg_wll_Event *,
	const edg_wll_JobStat *);


struct edg_wll_stats_cell {
	int	cnt;
	float	value;
	float 	value2;
};

struct edg_wll_stats_archive {
	int	ptr;
	struct edg_wll_stats_cell	cells[1];
};

struct edg_wll_stats_group {
	int	grpno;
	char	sig[33];
	char	destination[STATS_DEST_SIZE];
	time_t	last_update;
	struct edg_wll_stats_archive	archive[1];
};


typedef struct {
	enum { STATS_UNDEF = 0, STATS_COUNT, STATS_DURATION, STATS_DURATION_FROMTO }	type;
	edg_wll_QueryRec	*group;
	edg_wll_JobStatCode	base_state;
	edg_wll_JobStatCode     final_state;
	int			minor;
	struct _edg_wll_StatsArchive {
		int	interval,length;
	} *archives;

	int	fd;
	struct edg_wll_stats_group	*map;
	int	grpno,grpsize;
	unsigned htab_size;
	struct hsearch_data *htab;
} edg_wll_Stats;

int edg_wll_StateRateServer(
	edg_wll_Context	context,
	const edg_wll_QueryRec	*group,
	edg_wll_JobStatCode	major,
	int			minor,
	time_t	*from, 
	time_t	*to,
	float	**rates,
	char	***groups,
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
	float	**duration,
	char	***groups,
	int	*res_from,
	int	*res_to
);

int edg_wll_StateDurationFromToServer(
        edg_wll_Context ctx,
        const edg_wll_QueryRec  *group,
        edg_wll_JobStatCode     base_state,
        edg_wll_JobStatCode     final_state,
        int                     minor,
        time_t  *from,
        time_t  *to,
        float   **duration,
	float   **dispersion,
	char	***groups,
        int     *res_from,
        int     *res_to
);

#endif /* GLITE_LB_STATS_H */
