#ifndef _LBS_STORE_H
#define _LBS_STORE_H

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


#include "glite/lb/consumer.h"
#include "jobstat.h"
#include "lb_authz.h"

#ifdef __cplusplus
extern "C" {
#endif

/* store an event into the LB database */

int edg_wll_StoreEvent(
	edg_wll_Context,	/* INOUT */
	edg_wll_Event *,	/* IN */
	int *
);

void edg_wll_StoreAnonymous(
	edg_wll_Context,    /* INOUT */
	int		/* IN (boolean) */
);

/* update stored job state according to new event */

edg_wll_ErrorCode edg_wll_StepIntState(
	edg_wll_Context,	/* INOUT */
	edg_wlc_JobId,		/* IN */
	edg_wll_Event *,	/* IN */
	int,			/* IN */
	edg_wll_JobStat *
);

/* create embriotic job state for DAGs' subjob */

edg_wll_ErrorCode edg_wll_StepIntStateEmbriotic(
	edg_wll_Context ctx,	/* INOUT */
        edg_wll_Event *e	/* IN */
);

int db_store(edg_wll_Context,char *, char *);
int db_parent_store(edg_wll_Context, edg_wll_Event *, intJobStat *);
int handle_request(edg_wll_Context,char *);
int create_reply(const edg_wll_Context,char **);
int trans_db_store(edg_wll_Context,char *,edg_wll_Event *,intJobStat *);
edg_wll_ErrorCode intJobStat_embryonic(edg_wll_Context ctx, edg_wlc_JobId jobid, const edg_wll_RegJobEvent *e, intJobStat *stat);


int edg_wll_delete_event(edg_wll_Context,const char *, int);


#define USER_UNKNOWN	"unknown"

#ifdef __cplusplus
}
#endif

#endif
