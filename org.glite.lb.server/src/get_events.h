#ifndef GLITE_LB_GET_EVENTS_H
#define GLITE_LB_GET_EVENTS_H
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


#include "glite/lb/context.h"
#include "glite/lb/events.h"
#include "glite/lb/query_rec.h"

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
