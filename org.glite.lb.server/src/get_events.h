#ifndef GLITE_LB_GET_EVENTS_H
#define GLITE_LB_GET_EVENTS_H
#ident "$Header$"

#include "glite/lb/context.h"
#include "glite/lb/events.h"
#include "glite/lb/query_rec.h"

/* Internal functions for getting event sets from the LB database */
#include "lbs_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0	/* rel 1 */
char *edg_wll_jobid_to_user( edg_wll_Context, char *);
void edg_wll_set_event_field_warn( edg_wll_Event *, char *, char *);
void edg_wll_set_event_field( edg_wll_Event *, char *, char *);
int edg_wll_get_events_restricted( edg_wll_Context, edg_wlc_JobId, char *, int, int, char *, edg_wll_Event **);
#define edg_wll_get_events(ctx,job,md5,emin,emax,ret) \
	edg_wll_get_events_restricted((ctx),(job),(md5),(emin),(emax),NULL,(ret))
int edg_wll_last_event( edg_wll_Context, char *);
int compare_events_by_tv(const void *, const void *);
#endif

int edg_wll_get_event_flesh(edg_wll_Context,int,edg_wll_Event *);

int edg_wll_QueryEventsServer(edg_wll_Context,int,const edg_wll_QueryRec **,const edg_wll_QueryRec **,edg_wll_Event **);

int edg_wll_QueryJobsServer(edg_wll_Context, const edg_wll_QueryRec **, int, edg_wlc_JobId **, edg_wll_JobStat **);

void edg_wll_SortEvents(edg_wll_Event *);

void edg_wll_SortPEvents(edg_wll_Event **);

#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_GET_EVENTS_H */
