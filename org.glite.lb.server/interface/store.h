#ident "$Header$"

#ifndef GLITE_LB_STORE_H
#define GLITE_LB_STORE_H

#include "jobstat.h"
#include "lb_authz.h"

#ifdef __cplusplus
extern "C" {
#endif

/* store an event into the LB database */

int edg_wll_StoreEvent(
	edg_wll_Context,	/* INOUT */
	edg_wll_Event *,	/* IN */
	const char *,		/* IN */
	int *
);

void edg_wll_StoreAnonymous(
	edg_wll_Context,    /* INOUT */
	int		/* IN (boolean) */
);

int db_store(edg_wll_Context, char *);
int db_parent_store(edg_wll_Context, edg_wll_Event *, intJobStat *);
int handle_request(edg_wll_Context,char *);
int handle_il_message(edg_wll_Context,char *);
int create_reply(const edg_wll_Context,char **);
int is_job_local(edg_wll_Context, glite_jobid_const_t jobId);
int store_job_server_proxy(edg_wll_Context ctx, edg_wll_Event *event, int *register_to_JP);
int register_subjobs_embryonic(edg_wll_Context,const edg_wll_RegJobEvent *);
edg_wll_ErrorCode intJobStat_embryonic(edg_wll_Context ctx, glite_jobid_const_t jobid, const edg_wll_RegJobEvent *e, intJobStat *stat);


int edg_wll_delete_event(edg_wll_Context,const char *, int);


#define USER_UNKNOWN	"unknown"

/* flags for JP registrations */
#define REG_JOB_TO_JP		1
#define REG_SUBJOBS_TO_JP	2


#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_STORE_H */
