#ifndef GLITE_LB_STORE_H
#define GLITE_LB_STORE_H

#ident "$Header$"

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
int store_job_server_proxy(edg_wll_Context ctx, edg_wll_Event *event, char *jobIdHost, unsigned int jobIdPort);

int edg_wll_delete_event(edg_wll_Context,const char *, int);


#define USER_UNKNOWN	"unknown"

#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_STORE_H */
