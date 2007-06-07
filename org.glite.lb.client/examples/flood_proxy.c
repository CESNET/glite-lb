#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "producer.h"
#include "glite/wmsutils/jobid/cjobid.h"

static void slave();

int main(int argc,char **argv)
{
	int	i,nproc;

	if (argc != 2) {
		fprintf(stderr,"usage: %s nproc\n",argv[0]);
		return 1;
	}
	
	nproc = atoi(argv[1]);
	if (nproc < 1) {
		fprintf(stderr,"%s: nproc must be >= 1\n",argv[0]);
		return 1;
	}

	for (i=0; i<nproc; i++) {
		switch (fork()) {
			case -1: perror("fork()"); return 1;
			case 0: slave();
			default: break;
		}
	}

	while (nproc) {
		int	stat;
		wait(&stat);
		if (WIFEXITED(stat)) nproc--;
	}

	puts("done");
	return 0;
}


static void slave()
{
	edg_wll_Context	ctx;
	edg_wlc_JobId	job;
	int	i,pid = getpid(),noent = 0;

	for (i=0; i<100; i++) {
		int	err;
		char	*et,*ed;

		edg_wll_InitContext(&ctx);
		edg_wlc_JobIdParse("https://fake.server/fakejob",&job);

		if ((err = edg_wll_SetLoggingJobProxy(ctx,job,NULL,"some user",0))) edg_wll_Error(ctx,&et,&ed);
		else et = ed = "none";

		printf("[%d] %d: %s (%s)\n",pid,i,
				err == 0 || err == ENOENT ? "OK" : et,
				ed);

		if (err == ENOENT) noent++;

		edg_wll_LogUserTagProxy(ctx,"test","x");

		edg_wll_FreeContext(ctx);
	}
	printf("[%d] done, ENOENTs %d\n",pid,noent);
	exit(0);
}
