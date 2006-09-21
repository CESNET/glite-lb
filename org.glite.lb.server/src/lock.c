#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include "glite/lb-utils/cjobid.h"
#include "glite/lb/context-int.h"
#include "lock.h"

extern int debug;

int edg_wll_LockUnlockJob(const edg_wll_Context ctx,const glite_lbu_JobId job,int lock)
{
	struct sembuf	s;
	char	*un = glite_lbu_JobIdGetUnique(job);
	int	n,i;
	static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	
	if (!un) return edg_wll_SetError(ctx,EINVAL,"jobid");

	for (n=0; n<sizeof b64 && b64[n] != un[0]; n++);
	for (i=0; i<sizeof b64 && b64[i] != un[1]; i++);
	n += i<<6;

	if (debug) fprintf(stderr,"[%d] semop(%d,%d) \n",getpid(),n % ctx->semaphores,lock);

	s.sem_num = n % ctx->semaphores;
	s.sem_op = lock;
	s.sem_flg = SEM_UNDO;

	if (semop(ctx->semset,&s,1)) return edg_wll_SetError(ctx,errno,"edg_wll_LockUnlockJob()");
	return edg_wll_ResetError(ctx);
}
