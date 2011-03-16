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
#include <fcntl.h>

#include "glite/jobid/cjobid.h"
#ifdef BUILDING_LB_CLIENT
#include "producer.h"
#else
#include "glite/lb/producer.h"
#endif
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


	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		return 1;
	}

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
