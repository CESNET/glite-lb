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

#ident "$Header:"

#include "lb_rss.h"
#include "lb_proto.h"

#include "glite/lb/context-int.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>

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
		TR("Destination: %s\n", states[i].destination);
		TRC("</description>\n</item>\n");
		
		
		free(chid);
		free(chstat);
	}
	TRC("</channel>\n</rss>");

	*message = pomB;

	return 0;
}

