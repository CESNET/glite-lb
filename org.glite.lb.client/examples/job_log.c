#ident "$Header$"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "glite/lb/events_parse.h"
#include "glite/lb/consumer.h"
#include "glite/wmsutils/jobid/cjobid.h"
#ifdef USE_CALLBACKS
  #include "glite/lb/consumer_fake.h"
#endif

static void free_events(edg_wll_Event *);

static void help(const char* n)
{
    fprintf(stderr,"usage: %s [-r repeat] [-d delay] [-x [local socket]] <jobid>\n", n);
    exit(1);
}

#ifdef USE_CALLBACKS
static int query_events_cb(edg_wll_Context context, edg_wll_Event **events) {
  int i;
  edg_wll_Event *event;

  i = 0;
  while ((event = (*events) + i)->type != EDG_WLL_EVENT_UNDEF) {
    event->any.timestamp.tv_sec = (double)rand() / RAND_MAX * 1000000000;
    event->any.timestamp.tv_usec = (double)rand() / RAND_MAX * 1000000;
    i++;
  }

  return 0;
}
#endif

int main(int argc,char **argv)
{
	edg_wll_Context	ctx;
	char		*errt,*errd, *sock = NULL;
	edg_wll_Event	*events = NULL;
	edg_wlc_JobId	job;
	int		i,opt,delay = 0,count = 0;
	int 		(*jobLog)(edg_wll_Context,
        			edg_wlc_JobId,
	        		edg_wll_Event **);

	if (argc < 2)
	    help(argv[0]);

	while ((opt=getopt(argc,argv,"r:d:x::")) != -1)
	    switch (opt) {
	    case 'd': delay = atoi(optarg); break;
	    case 'r': count = atoi(optarg); break;
	    case 'x':
		if (optarg) sock=strdup(count);
		else sock=strdup("/tmp/lb_proxy_serve.sock");
		break;
	    default:
                help(argv[0]);
	    }

	edg_wll_InitContext(&ctx);
	if (edg_wlc_JobIdParse(argv[optind],&job)) {
		fprintf(stderr,"%s: can't parse job ID\n",argv[1]);
		return 1;
	}

#ifdef USE_CALLBACKS
	edg_wll_RegisterTestQueryEvents(query_events_cb);
#endif
	if (sock) {
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_LBPROXY_SERVE_SOCK, sock);
	}
	jobLog = (sock) ? edg_wll_JobLogProxy : edg_wll_JobLog;

	if ( jobLog(ctx,job,&events) )
	{
		edg_wll_Error(ctx,&errt,&errd);
		fprintf(stderr,"%s: %s (%s)\n",argv[0],errt,errd);
	}

	for ( i = 0; events && events[i].type != EDG_WLL_EVENT_UNDEF; i++ )
	{
		char	*e = edg_wll_UnparseEvent(ctx,events+i);
		fputs(e,stdout);
		fputs("\n",stdout);
		free(e);
	}

	free_events(events);
	printf("\nFound %d events\n",i);

	while (count--) {
		puts("Sleeping ...");
		sleep(delay);
		if (jobLog(ctx,job,&events)) {
			edg_wll_Error(ctx,&errt,&errd);
			fprintf(stderr,"%s: %s (%s)\n",argv[0],errt,errd);
			free(errt); free(errd); errt = errd = NULL;
			free_events(events);
		}
		else puts("OK");
	}

	edg_wlc_JobIdFree(job);
	edg_wll_FreeContext(ctx); 

#ifdef USE_CALLBACKS
	edg_wll_UnregisterTestQueryEvents();
#endif
	
	return 0;

err:
#ifdef USE_CALLBACKS
	edg_wll_UnregisterTestQueryEvents();
#endif
	switch (edg_wll_Error(ctx,&errt,&errd)) {
		case 0: break;
		case ENOENT:
			puts("No events found");
			break;
		default:
			fprintf(stderr,"%s: %s (%s)\n",argv[0],errt,errd);
			return 1;
	}

	edg_wlc_JobIdFree(job);
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
