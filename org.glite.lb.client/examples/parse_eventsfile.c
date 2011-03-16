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

#ifdef BUILDING_LB_CLIENT
#include "producer.h"
#else
#include "glite/lb/producer.h"
#endif
#include "glite/lb/events.h"
#include "glite/lb/events_parse.h"

#define	MAXMSGSIZE	10240

extern char *optarg;
extern int opterr,optind;

static const char *me;

static void usage()
{
	fprintf(stderr,"usage: %s [-n] -f file_name\n", me);
}

int main(int argc, char *argv[])
{
	char 	*filename = NULL;
	char 	buf[MAXMSGSIZE];
	int     done = 0,i=0,notif=0;
	edg_wll_Context	ctx;
	edg_wll_Event   *event = NULL;
	FILE	*f;
	edg_wll_ErrorCode (*parse)(edg_wll_Context context,edg_wll_LogLine logline,edg_wll_Event **event);
	edg_wll_LogLine (*unparse)(edg_wll_Context context,edg_wll_Event *event);
	const char *parse_str,*unparse_str;

	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		return 1;
	}

	opterr = 0;

	me = strdup(argv[0]);

	do {
		switch (getopt(argc,argv,"nf:")) {
			case 'n': notif = 1; break;
			case 'f': filename = (char *) strdup(optarg); break;
			case '?': usage(); exit(EINVAL);
			case -1: done = 1; break;
		}
	} while (!done);

	/* choose the right (un)parser */
	if (notif) {
		parse = edg_wll_ParseNotifEvent;
		parse_str = "edg_wll_ParseNotifEvent";
		unparse = edg_wll_UnparseNotifEvent;
		unparse_str = "edg_wll_UnparseNotifEvent";
	} else {
		parse = edg_wll_ParseEvent;
		parse_str = "edg_wll_ParseEvent";
		unparse = edg_wll_UnparseEvent;
		unparse_str = "edg_wll_UnparseEvent";
	}

	if (!filename) {
		fprintf(stderr,"%s: -f required\n",me);
		usage();
		exit(1);
	}

	if ( (f = fopen(filename,"r")) == NULL) {
		perror(filename);
		exit(1);
	} else {
		fprintf(stderr,"Parsing file '%s' for correctness:\n",filename);
	}
	
	/* parse events */
	i = 1;
	while (!feof(f)) {
		if (!fgets(buf,sizeof(buf),f)) break;
		if (strcmp(buf,"\n")) {
			// fprintf(stdout,"%d: %s\n",i,buf);

			if (parse(ctx,buf,&event) != 0) {
				/* Parse ERROR: */
				char    *et=NULL,*ed=NULL;

				edg_wll_Error(ctx,&et,&ed);
				fprintf(stderr,"line %d: %s() error: %s (%s)\n",i,parse_str,et,ed);
				if (et) free(et);
				if (ed) free(ed);
			} else {
				/* Parse OK : */
				char	*es=NULL;
				edg_wll_LogLine logline = NULL;

				es=edg_wll_EventToString(event->type);
				logline = unparse(ctx,event);
				fprintf(stderr,"line %d: %s() o.k. (event %s), ",i,parse_str,es);
				if (logline) {
					fprintf(stderr,"%s() o.k.\n",unparse_str);
					free(logline);
				} else {
					fprintf(stderr,"%s() error\n",unparse_str);
				}
				if (es) free(es);
			}
			if (event) edg_wll_FreeEvent(event);
		}
		i++;
	}
	fclose(f);

	edg_wll_FreeContext(ctx);

	return 0;
}
