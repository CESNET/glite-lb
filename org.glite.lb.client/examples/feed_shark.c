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
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <sysexits.h>

#ifdef BUILDING_LB_CLIENT
#include "notification.h"
#else
#include "glite/lb/notification.h"
#endif

static void usage(const char *);
static void printstat(edg_wll_JobStat, int);

int main(int argc,char *argv[])
{
	int	o;
	edg_wll_Context	ctx;
	edg_wll_NotifId	notif;
	time_t	valid = 0;
	struct timeval	timeout;

	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		return 1;
	}

/* parse options, reflect them in the LB context */
	while ((o = getopt(argc,argv,"s:")) >= 0) switch (o) {
		case 's': {
			char	*server = strdup(optarg),
				*port_s = strchr(server,':');

			int	port;

			if (port_s) {
				*port_s = 0;
				port = atoi(port_s+1);
			}
			
			edg_wll_SetParam(ctx,EDG_WLL_PARAM_NOTIF_SERVER,server);
			if (port_s) edg_wll_SetParam(ctx,EDG_WLL_PARAM_NOTIF_SERVER_PORT,port);
			free(server);
		} break;
			  
		case '?': usage(argv[0]); exit(EX_USAGE);
	}

/* no notification Id supplied -- create a new one */
	if (argc == optind) {
		edg_wll_QueryRec	const *empty[] = { NULL };
		char	*notif_s;

		if (edg_wll_NotifNew(ctx,empty,0,-1,NULL,&notif,&valid)) {
			char	*et,*ed;

			edg_wll_Error(ctx,&et,&ed);
			fprintf(stderr,"edg_wll_NotifNew(): %s (%s)\n",et,ed);
			exit(EX_UNAVAILABLE);
		}

		notif_s = edg_wll_NotifIdUnparse(notif);
		printf("notification registered:\n\tId: %s\n\tExpires: %s",
				notif_s,ctime(&valid));
		free(notif_s);
	}
/* notification Id supplied -- bind to it */
	else if (argc == optind + 1) {
		if (edg_wll_NotifIdParse(argv[optind],&notif)) {
			fprintf(stderr,"%s: invalid notification Id\n",
					argv[optind]);
			exit(EX_DATAERR);
		}

		if (edg_wll_NotifBind(ctx,notif,-1,NULL,&valid)) {
			char	*et,*ed;

			edg_wll_Error(ctx,&et,&ed);
			fprintf(stderr,"edg_wll_NotifBind(): %s (%s)\n",et,ed);
			exit(EX_UNAVAILABLE);
		}
		printf("bound to %s\n\t Expires: %s",argv[optind],ctime(&valid));
	}
	else { usage(argv[0]); exit(EX_USAGE); }

/* main loop */
	while (1) {
		edg_wll_JobStat	stat;
		char	*et,*ed;

/* calculate time left for this notification */
		gettimeofday(&timeout,NULL);
		timeout.tv_sec = valid - timeout.tv_sec;
		assert(timeout.tv_sec >= 0);	/* XXX: hope we are no late */

/* half time before notification renewal */
		timeout.tv_sec /= 2;

		switch (edg_wll_NotifReceive(ctx,-1,&timeout,&stat,NULL)) {
			case 0: /* OK, got it */
				printstat(stat,0);
				edg_wll_FreeStatus(&stat);
				break;
			case EAGAIN: 	/* timeout */
				if (edg_wll_NotifRefresh(ctx,notif,&valid)) {
					edg_wll_Error(ctx,&et,&ed);
					fprintf(stderr,"edg_wll_NotifRefresh(): %s (%s)\n",et,ed);
					exit(EX_UNAVAILABLE);
				}
				printf("Notification refreshed, expires %s",ctime(&valid));
				break;
			default:
				edg_wll_Error(ctx,&et,&ed);
				fprintf(stderr,"edg_wll_NotifReceive(): %s (%s)\n",et,ed);
				exit(EX_UNAVAILABLE);
		}
	}
}


static void usage(const char *me) 
{
	fprintf(stderr,"usage: %s [ -s server[:port] ] [notif_id]\n",me);
}



static void printstat(edg_wll_JobStat stat, int level)
{
    char        *s, *j, ind[10];
    int         i;


    for (i=0; i < level; i++)
        ind[i]='\t';
    ind[i]='\0';

    s = edg_wll_StatToString(stat.state);
/* print whole flat structure */
    printf("%sstate : %s\n", ind, s);
    printf("%sjobId : %s\n", ind, j = edg_wlc_JobIdUnparse(stat.jobId));
    printf("%sowner : %s\n", ind, stat.owner);
    printf("%sjobtype : %s\n", ind, (stat.jobtype ? "DAG" : "SIMPLE") );
    printf("%sparent_job : %s\n", ind,
            j = edg_wlc_JobIdUnparse(stat.parent_job));
    if (stat.jobtype) {;
        printf("%sseed : %s\n", ind, stat.seed);
        printf("%schildren_num : %d\n", ind, stat.children_num);
        printf("%schildren :\n", ind);
        if (stat.children)
			for  (i=0; stat.children[i]; i++)
                printf("%s\tchildren : %s\n", ind, stat.children[i]);
        printf("%schildren_states :\n", ind);
        if (stat.children_states)
            for  (i=0; stat.children_states[i].state; i++)
                printstat(stat.children_states[i], level+1);
        printf("%schildren_hist :\n",ind);
        if (stat.children_hist)
            for (i=1; i<=stat.children_hist[0]; i++)
                printf("%s%14s  %d\n", ind, edg_wll_StatToString(i-1),stat.children_hist[i]);
    }
    printf("%scondorId : %s\n", ind, stat.condorId);
    printf("%sglobusId : %s\n", ind, stat.globusId);
    printf("%slocalId : %s\n", ind, stat.localId);
    printf("%sjdl : %s\n", ind, stat.jdl);
    printf("%smatched_jdl : %s\n", ind, stat.matched_jdl);
    printf("%sdestination : %s\n", ind, stat.destination);
    printf("%snetwork server : %s\n", ind, stat.network_server);
    printf("%scondor_jdl : %s\n", ind, stat.condor_jdl);
    printf("%srsl : %s\n", ind, stat.rsl);
    printf("%sreason : %s\n", ind, stat.reason);
    printf("%slocation : %s\n", ind, stat.location);
	printf("%ssubjob_failed : %d\n", ind, stat.subjob_failed);
    printf("%sdone_code : %d\n", ind, stat.done_code);
    printf("%sexit_code : %d\n", ind, stat.exit_code);
    printf("%sresubmitted : %d\n", ind, stat.resubmitted);
    printf("%scancelling : %d\n", ind, stat.cancelling);
    printf("%scancelReason : %s\n", ind, stat.cancelReason);
    printf("%scpuTime : %d\n", ind, stat.cpuTime);
    printf("%suser_tags :\n",ind);
    if (stat.user_tags)
        for (i=0; stat.user_tags[i].tag; i++) printf("%s%14s = \"%s\"\n", ind,
                              stat.user_tags[i].tag,stat.user_tags[i].value);
    printf("%sstateEnterTime : %ld.%06ld\n", ind, stat.stateEnterTime.tv_sec,stat.stateEnterTime.tv_usec);
    printf("%sstateEnterTimes : \n",ind);
    if (stat.stateEnterTimes)
                for (i=1; i<=stat.stateEnterTimes[0]; i++)
            printf("%s%14s  %s", ind, edg_wll_StatToString(i-1), (stat.stateEnterTimes[i] == 0) ?
            "    - not available -\n" : ctime((time_t *) &stat.stateEnterTimes[i]));
    printf("%slastUpdateTime : %ld.%06ld\n", ind, stat.lastUpdateTime.tv_sec,stat.lastUpdateTime.tv_usec);
	printf("%sexpectUpdate : %d\n", ind, stat.expectUpdate);
    printf("%sexpectFrom : %s\n", ind, stat.expectFrom);
    printf("%sacl : %s\n", ind, stat.acl);
    printf("\n");

    free(j);
    free(s);
}
