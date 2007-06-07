#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>

#include "glite/wmsutils/jobid/cjobid.h"
#include "producer.h"
#include "glite/lb/events.h"

char	*outfile = "context_errors";
edg_wlc_JobId	job;

static int stop;

struct {
	int	err;
	char	*text,*desc;
} *errors;

int	nerrors;

static void killslaves(int sig)
{
	kill(0,SIGTERM);
	exit(0);
}

static void terminate(int sig)
{
	stop = sig;
}

static void slave(int num)
{
	int	good = 0,i;
	char	fname[PATH_MAX];
	FILE	*errf;
	edg_wll_Context	ctx;

	signal(SIGINT,terminate);
	signal(SIGTERM,terminate);
	signal(SIGHUP,terminate);


	while (!stop) {
		edg_wll_InitContext(&ctx);
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_SOURCE, EDG_WLL_SOURCE_WORKLOAD_MANAGER);

		if (edg_wll_SetLoggingJobProxy(ctx,job,NULL,"/I/am/the/user",EDG_WLL_SEQ_NORMAL) && !stop) {
			errors = realloc(errors,(nerrors+1) * sizeof(*errors));
			errors[nerrors].err = edg_wll_Error(ctx,&errors[nerrors].text,&errors[nerrors].desc);
			nerrors++;
		}
		else good++;

		edg_wll_FreeContext(ctx);
	}

	sprintf(fname,"%s_%03d",outfile,num);
	errf = fopen(fname,"w");
	if (!errf) { perror(fname); exit(1); }

	for (i=0; i<nerrors; i++)
		fprintf(errf,"%d %s %s\n",errors[i].err,errors[i].text,errors[i].desc);

	fprintf(errf,"\n%d errors, %d successful attempts\n",nerrors,good);

	fclose(errf);
	exit(0);
}

static void usage(const char *me)
{
	fprintf(stderr,"usage: %s [-n nproc] [-o outfile] jobid\n",me);
}

int main(int argc,char **argv)
{
	int	nproc = 10,opt,i;

	while  ((opt = getopt(argc,argv,"n:o:")) != -1) switch (opt) {
		case 'n': nproc = atoi(optarg); break;
		case 'o': outfile = optarg; break;
		case '?': usage(argv[0]); exit(1);
	}

	if (optind != argc-1) { usage(argv[0]); exit(1); }

	if (edg_wlc_JobIdParse(argv[optind],&job)) {
		fprintf(stderr,"%s: can't parse\n",argv[optind]);
		exit(1);
	}

	signal(SIGTERM,killslaves);
	signal(SIGINT,killslaves);
	signal(SIGHUP,killslaves);
	signal(SIGCHLD,SIG_IGN);

	if (nproc == 1) slave(0);
	else for (i=0; i<nproc; i++) switch(fork()) {
		case -1: perror("fork()"); exit(1);
		case 0: slave(i); /* don't return */
		default: break;
	}

	puts("Slaves started, press Ctrl-C to stop");

	while(1) pause();
}
