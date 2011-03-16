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
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "glite/jobid/cjobid.h"
#ifdef BUILDING_LB_CLIENT
#include "consumer.h"
#else
#include "glite/lb/consumer.h"
#endif

static void usage(char *me) {
	fprintf(stderr,"usage: %s: YYYY-MM-DD:HH:MI YYYY-MM-DD:HH:MI\n"
			"\t times specify interval to query\n",me
		);
	exit(1);
}

int main(int argc,char **argv) {
	edg_wll_Context	ctx;
	struct tm	tm;
	time_t	from,to;
	int	i, err = 0;

	edg_wll_QueryRec	cond[3],jcond[2];	/* [2] is terminator */
	edg_wll_Event	*events = NULL;

	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		return 1;
	}

	if (argc != 3) usage(argv[0]);

	
	memset(&tm,0,sizeof tm);

	if (sscanf(argv[1],"%d-%d-%d:%d:%d",
		&tm.tm_year,
		&tm.tm_mon,
		&tm.tm_mday,
		&tm.tm_hour,
		&tm.tm_min) != 5) usage(argv[0]);

	tm.tm_mon--;
	tm.tm_year -= 1900;

	from = mktime(&tm);
	
	if (sscanf(argv[2],"%d-%d-%d:%d:%d",
		&tm.tm_year,
		&tm.tm_mon,
		&tm.tm_mday,
		&tm.tm_hour,
		&tm.tm_min) != 5) usage(argv[0]);

	tm.tm_mon--;
	tm.tm_year -= 1900;

	to = mktime(&tm);

	memset(jcond,0,sizeof jcond);

	jcond[0].attr = EDG_WLL_QUERY_ATTR_OWNER;
	jcond[0].op = EDG_WLL_QUERY_OP_UNEQUAL;
	jcond[0].value.c = "nonexistent";

	memset(cond,0,sizeof cond);
	cond[0].attr = EDG_WLL_QUERY_ATTR_TIME;
	cond[0].op = EDG_WLL_QUERY_OP_WITHIN;
	cond[0].value.t.tv_sec = from;
	cond[0].value2.t.tv_sec = to;

	cond[1].attr = EDG_WLL_QUERY_ATTR_EVENT_TYPE;
	cond[1].value.i = EDG_WLL_EVENT_DONE;
	
	if (edg_wll_QueryEvents(ctx,jcond,cond,&events)) {
		char	*et,*ed;
		et = ed = NULL;

		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"edg_wll_QueryEventsProxy: %s, %s\n",
			et,ed ? ed : "");

		free(et); free(ed);
		err = 1;
		goto cleanup;
	}

	for (i=0; events[i].type; i++) {
		edg_wll_Event	*e = events+i;

		if (e->done.status_code == EDG_WLL_DONE_FAILED) {
			char	*job = glite_jobid_unparse(e->any.jobId);

			printf("%s\t%s\t%s\t%s\n",
				job,
				ctime(&e->done.timestamp.tv_sec),
				e->done.host,
				e->done.reason
				);

			free(job);
		}
	}
				
cleanup:
	if (events) {
		for (i=0; events[i].type; i++) edg_wll_FreeEvent(events+i);
		free(events); events = NULL;
	}

	edg_wll_FreeContext(ctx);

	return err;
}
