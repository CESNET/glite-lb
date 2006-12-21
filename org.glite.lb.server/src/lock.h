#define edg_wll_LockJob(ctx,job) edg_wll_LockUnlockJob((ctx),(job),-1)
#define edg_wll_UnlockJob(ctx,job) edg_wll_LockUnlockJob((ctx),(job),1)

int edg_wll_JobSemaphore(const edg_wll_Context ctx, const glite_lbu_JobId job);
int edg_wll_LockUnlockJob(const edg_wll_Context,const glite_lbu_JobId,int);
