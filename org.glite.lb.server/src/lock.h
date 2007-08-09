#ifndef GLITE_LB_LOCK_H
#define GLITE_LB_LOCK_H

#ident "$Header$"

#include "glite/lb/context.h"

#define edg_wll_LockJob(ctx,job) edg_wll_LockUnlockJob((ctx),(job),-1)
#define edg_wll_UnlockJob(ctx,job) edg_wll_LockUnlockJob((ctx),(job),1)

int edg_wll_LockUnlockJob(const edg_wll_Context,const edg_wlc_JobId,int);
int edg_wll_JobSemaphore(const edg_wll_Context ctx, const edg_wlc_JobId job);

#endif /* GLITE_LB_LOCK_H */
