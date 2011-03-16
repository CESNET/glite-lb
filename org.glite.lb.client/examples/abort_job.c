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


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "glite/lb/events_parse.h"
#ifdef BUILDING_LB_CLIENT
#include "consumer.h"
#include "producer.h"
#else
#include "glite/lb/consumer.h"
#include "glite/lb/producer.h"
#endif
#include "glite/jobid/cjobid.h"
#include "glite/lb/context-int.h"

static void free_events(edg_wll_Event *);

static void help(const char* n)
{
    fprintf(stderr,"usage: %s <jobid> <source>\n", n);
    exit(1);
}

int main(int argc,char **argv)
{
	edg_wll_Context	ctx;
	char		*errt,*errd,*e,a;
	edg_wll_Event	*events;
	int		i;
	edg_wll_QueryRec	jq[2],eq[2];
	edg_wlc_JobId	job;
	edg_wll_Source	src;

	if (argc != 3) help(argv[0]);

	puts(
"*\n"
"* USE WITH CARE, AT YOUR OWN RISK, UNDER ABNORMAL CONDITIONS ONLY,\n"
"* AND BE ALWAYS SURE WHAT YOU ARE DOING.\n"
"* \n"
"* THIS PROGRAM IS A PERFECT EXAMPLE HOW L&B SEQENCE CODES SHOULD NOT BE USED\n"
"* IN NORMAL OPERATION.\n"
"\n"
"Do you want to proceed?"
	);

	scanf("%c",&a);
	if (a != 'y' && a != 'Y') return 1;

	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		return 1;
	}

	if (edg_wlc_JobIdParse(argv[1],&job)) {
		fprintf(stderr,"%s: can't parse job ID\n",argv[1]);
		return 1;
	}

	if (( src = edg_wll_StringToSource(argv[2])) == EDG_WLL_SOURCE_NONE) {
		fprintf(stderr,"%s: unknown event source\n",argv[2]);
		return 1;
	}

	jq[0].attr = EDG_WLL_QUERY_ATTR_JOBID;
	jq[0].op = EDG_WLL_QUERY_OP_EQUAL;
	jq[0].value.j = job;
	jq[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	eq[0].attr = EDG_WLL_QUERY_ATTR_SOURCE;
	eq[0].op = EDG_WLL_QUERY_OP_EQUAL;
	eq[0].value.i = src;
	eq[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	if ( edg_wll_QueryEvents(ctx,jq,eq,&events) )
	{
		if ( edg_wll_Error(ctx, &errt, &errd) != E2BIG )
			goto err;

		fprintf(stderr,"%s: %s (%s)\n",argv[0],errt,errd);
	}

	for ( i = 0; events[i].type != EDG_WLL_EVENT_UNDEF; i++ );

	e = edg_wll_UnparseEvent(ctx,events+i-1);

	fputs(e,stdout);
	fputs("\n",stdout);

	if (edg_wll_SetParam(ctx,EDG_WLL_PARAM_SOURCE,src) || 
		edg_wll_SetLoggingJob(ctx,job,events[i-1].any.seqcode,EDG_WLL_SEQ_NORMAL) ||
		edg_wll_IncSequenceCode(ctx) || /* necessary to simulate this
						* call in last event logging 
						* _after_ current seq. was used */
		edg_wll_LogAbort(ctx,"manual abort")) goto err;


	free(e);
	free_events(events);

	edg_wll_FreeContext(ctx); 

	return 0;

err:
	switch (edg_wll_Error(ctx,&errt,&errd)) {
		case 0: break;
		case ENOENT:
			puts("No events found");
			break;
		default:
			fprintf(stderr,"%s: %s (%s)\n",argv[0],errt,errd);
			return 1;
	}

	edg_wll_FreeContext(ctx); 

	return 0;
}

static void free_events(edg_wll_Event *events)
{
	int	i;

	if (events) {
		for (i=0; events[i].type != EDG_WLL_EVENT_UNDEF; i++) edg_wll_FreeEvent(&(events[i]));
		edg_wll_FreeEvent(&(events[i])); /* free last line */
		free(events);	
		events = NULL;
	}
}
