#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glite/lb/context.h"
#include "glite/lb/consumer.h"

#include "glite/lb/intjobstat.h"
#include "glite/lb/seqcode_aux.h"

/*comment */

int main (int argc, char ** argv)
{
	edg_wll_Context	ctx;
	edg_wll_QueryRec	jc[2];
	edg_wll_Event	*events;
	int	i,n;
	intJobStat	is;

	memset(&jc,0,sizeof jc);
	if (argc != 2 || glite_jobid_parse(argv[1],&jc[0].value.j)) {
		fprintf(stderr,"usage: %s <jobid>\n",argv[0]);
		return 1;
	}


	edg_wll_InitContext(&ctx);
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

	qsort(events,n,sizeof *events,compare_events_by_seq);

	for (i=0; i<n; i++) {
		char	*err;
		if (processEvent(&is,events+i,0,0,&err) == RET_FATAL) {
			fprintf(stderr,"event %d: %s\n",i,err);
			return 1;
		}

		printf ("%s %s\n",edg_wll_EventToString(events[i].type),
				edg_wll_StatToString(is.pub.state));
	}

	return 0;
}
