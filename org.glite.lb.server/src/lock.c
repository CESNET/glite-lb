#ident "$Header$"

#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include "glite/jobid/cjobid.h"
#include "glite/lb/context-int.h"
#include "lock.h"

extern int debug;

int edg_wll_JobSemaphore(const edg_wll_Context ctx, glite_jobid_const_t job)
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

int edg_wll_LockUnlockJob(const edg_wll_Context ctx,glite_jobid_const_t job,int lock)
{
	struct sembuf	s;
	int 		n;

		
	if ((n=edg_wll_JobSemaphore(ctx, job)) == -1) return edg_wll_Error(ctx,NULL,NULL);

	if (debug) fprintf(stderr,"[%d] try semop(%d,%d) \n",getpid(),n,lock);

	s.sem_num = n;
	s.sem_op = lock;
	s.sem_flg = SEM_UNDO;

	if (semop(ctx->semset,&s,1)) { 
		if (debug) fprintf(stderr,"[%d] failed semop(%d,%d) \n",getpid(),n,lock);
		return edg_wll_SetError(ctx,errno,"edg_wll_LockUnlockJob()");
	}

	if (debug) fprintf(stderr,"[%d] got semop(%d,%d) \n",getpid(),n,lock);
	return edg_wll_ResetError(ctx);
}
