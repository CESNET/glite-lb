#ident "$Header$"

#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include "glite/wmsutils/jobid/cjobid.h"
#include "glite/lb/context-int.h"
#include "lock.h"

extern int debug;

int edg_wll_JobSemaphore(const edg_wll_Context ctx, const edg_wlc_JobId job)
{
	char	*un = edg_wlc_JobIdGetUnique(job);
	int	n,i;
	static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";


	if (!un) {
		edg_wll_SetError(ctx,EINVAL,"jobid");
		return -1;
	}

	for (n=0; n<sizeof b64 && b64[n] != un[0]; n++);
	for (i=0; i<sizeof b64 && b64[i] != un[1]; i++);
	n += i<<6;

	free(un);
	return(n % ctx->semaphores);
}

int edg_wll_LockUnlockJob(const edg_wll_Context ctx,const edg_wlc_JobId job,int lock)
{
	struct sembuf	s;
	int 		n;

		
	if ((n=edg_wll_JobSemaphore(ctx, job)) == -1) return edg_wll_Error(ctx,NULL,NULL);

	if (debug) fprintf(stderr,"[%d] semop(%d,%d) \n",getpid(),n,lock);

	s.sem_num = n;
	s.sem_op = lock;
	s.sem_flg = SEM_UNDO;

	if (semop(ctx->semset,&s,1)) return edg_wll_SetError(ctx,errno,"edg_wll_LockUnlockJob()");
	return edg_wll_ResetError(ctx);
}
