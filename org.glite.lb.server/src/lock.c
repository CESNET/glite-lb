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
