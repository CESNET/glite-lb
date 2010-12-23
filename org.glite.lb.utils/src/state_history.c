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
#include <string.h>
#include <unistd.h>

#include "glite/lb/context.h"
#include "glite/lb/consumer.h"

#include "glite/lb/intjobstat.h"
#include "glite/lb/seqcode_aux.h"
#include "glite/lb/stat_fields.h"
#include "glite/lb/process_event.h"

#define usage() { \
	fprintf(stderr,"usage: %s [-f fields] <jobid>\n",argv[0]); \
	fprintf(stderr,"\navailable fields:\n\t"); \
	glite_lb_dump_stat_fields(); \
	putc(10,stderr); \
}

int main (int argc, char ** argv)
{
	edg_wll_Context	ctx;
	edg_wll_QueryRec	jc[2];
	edg_wll_Event	*events;
	int	i,n,opt;
	intJobStat	is;
	char	*farg = "network_server,jdl:VirtualOrganisation,destination,done_code,reason";
	void	*fields;

	while ((opt = getopt(argc,argv,"f:")) != -1) switch (opt) {
		case 'f': farg = optarg; break;
		default: usage(); return 1;
	}


	memset(&jc,0,sizeof jc);
	if (optind+1 != argc
			|| glite_jobid_parse(argv[optind],&jc[0].value.j)
			|| glite_lb_parse_stat_fields(farg,&fields))
	{
		usage();
		return 1;
	}


	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		exit(1);
	}
	jc[0].attr = EDG_WLL_QUERY_ATTR_JOBID;
	jc[0].op = EDG_WLL_QUERY_OP_EQUAL;

	if (edg_wll_QueryEvents(ctx,jc,NULL,&events)) {
		char	*ed,*et;
		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"%s: %s (%s)\n",argv[1],et,ed);
		return 1;
	}

	for (n=0; events[n].type; n++);

	init_intJobStat(&is);
	glite_jobid_dup(jc[0].value.j,&is.pub.jobId);

	qsort(events,n,sizeof *events,compare_events_by_seq);

	for (i=0; i<n; i++) {
		char	*err,*evnt = NULL;
		if (processEvent(&is,events+i,0,0,&err) == RET_FATAL) {
			fprintf(stderr,"event %d: %s\n",i,err);
			return 1;
		}

		printf("%s\t",evnt = edg_wll_EventToString(events[i].type));
		free(evnt); evnt = NULL;
		glite_lb_print_stat_fields(fields,&is.pub);   
	}

	return 0;
}
