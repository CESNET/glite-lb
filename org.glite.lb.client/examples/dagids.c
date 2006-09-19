#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "glite/wmsutils/jobid/cjobid.h"
#include "glite/lb/producer.h"
#include "glite/lb/events.h"

extern char *optarg;
extern int opterr,optind;

static void usage(char *me)
{
	fprintf(stderr,"usage: %s -m bkserver -n num_subjobs [-s seed]\n", me);
}

int main(int argc, char *argv[])
{
	char	*seed = "seed", *server = NULL,*p;
	int done = 0,num_subjobs = 0,i;
	edg_wll_Context	ctx;
	edg_wlc_JobId	jobid,*subjobs;


	edg_wll_InitContext(&ctx);
	opterr = 0;

	do {
		switch (getopt(argc,argv,"m:n:s:")) {
			case 's': seed = strdup(optarg); break;
			case 'm': server = strdup(optarg); break;
			case 'n': num_subjobs = atoi(optarg); break;
			case '?': usage(argv[0]); exit(EINVAL);
			case -1: done = 1; break;
		}
	} while (!done);

	if (!server) {
		fprintf(stderr,"%s: -m server required\n",argv[0]);
		exit(1);
	}

	if (!num_subjobs) {
		fprintf(stderr,"%s: -n num_subjobs required\n",argv[0]);
		exit(1);
	}

	p = strchr(server,':');
	if (p) *p=0;
	edg_wlc_JobIdCreate(server,p?atoi(p+1):0,&jobid);
	printf("seed=\"%s\"\nnodes=%d\ndag=\"%s\"\n",seed,num_subjobs,edg_wlc_JobIdUnparse(jobid));

	edg_wll_GenerateSubjobIds(ctx,jobid,num_subjobs,seed,&subjobs);

	for (i=0; i<num_subjobs; i++) printf("node[%d]=\"%s\"\n",i,edg_wlc_JobIdUnparse(subjobs[i]));

	return 0;
}
