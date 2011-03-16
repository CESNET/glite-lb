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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef BUILDING_LB_CLIENT
#include "producer.h"
#else
#include "glite/lb/producer.h"
#endif
#include "glite/jobid/cjobid.h"

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

		if (edg_wll_InitContext(&ctx) != 0) {
			fprintf(stderr, "Couldn't create L&B context.\n");
			return 1;
		}
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
