#ident "$Header:"

#include "lb_rss.h"
#include "lb_proto.h"

#include "glite/lb/context-int.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif

#define TR(type, field) \
	if (field){ \
		int l = asprintf(&pomA, type, (field)); \ 
		pomB = realloc(pomB, sizeof(*pomB)*(pomL+l+1));	\
		strcpy(pomB+pomL, pomA); \
		pomL += l; \
		free(pomA); pomA = NULL; \
	}

#define TRC(str) \
	{ \
		int l = asprintf(&pomA, str); \
		pomB = realloc(pomB, sizeof(*pomB)*(pomL+l+1)); \
		strcpy(pomB+pomL, pomA); \
                pomL += l; \
                free(pomA); pomA = NULL; \ 
	}


int edg_wll_RSSFeed(edg_wll_Context ctx UNUSED_VAR, edg_wll_JobStat *states, char *request, char **message){
	char *pomA = NULL, *pomB = NULL;
	int pomL = 0;
	int i;
	char *chid, *chstat, *rss_link, *tmp;
	time_t time;
	
	TRC("<?xml version=\"1.0\"?>\n<rss version=\"2.0\">\n<channel>\n");
	TRC("<title>LB feed</title>\n");
	asprintf(&rss_link, "https://%s:%i%s", ctx->srvName, ctx->srvPort, request);
	for (tmp = rss_link; *tmp && !isspace(*tmp); tmp++);
	*tmp = 0;
	TR("<link>%s</link>\n", rss_link);
	TRC("<description>List of jobs.</description>");
	if (states) for (i = 0; states[i].state != EDG_WLL_JOB_UNDEF; i++){
		TRC("<item>\n");
		TR("<title>%s</title>\n", (chid = edg_wlc_JobIdUnparse(states[i].jobId)));
		TR("<link>%s</link>\n", chid);
		TR("<guid>%s</guid>\n", chid);
		// strip description to allow new lines
		TR("<description>Status: %s\n", (chstat = edg_wll_StatToString(states[i].state)));
		TRC("<![CDATA[<br/>]]>\n");
		time = states[i].stateEnterTime.tv_sec;
		TR("State entered: %s\n", ctime(&time));
		TRC("<![CDATA[<br/>]]>\n");
		TR("Destination: %s</description>\n", states[i].destination);
		TRC("</item>\n");
		
		
		free(chid);
		free(chstat);
	}
	TRC("</channel>\n</rss>");

	*message = pomB;

	return 0;
}

