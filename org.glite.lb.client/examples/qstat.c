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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "glite/lb/context-int.h"
#ifdef BUILDING_LB_CLIENT
#include "consumer.h"
#else
#include "glite/lb/consumer.h"
#endif
#include "glite/lb/xml_conversions.h"
#include "glite/lb/jobstat.h"

extern char *edg_wll_TagListToString(edg_wll_TagValue *);

static void dgerr(edg_wll_Context,char *);
static void printstat(edg_wll_JobStat,int);

#define MAX_SERVERS	20

/* these are defined in pbs_ifl.h or qstat.c */
#define PBS_MAXSEQNUM           8
#define PBS_MAXJOBARRAYLEN      6
#define PBS_MINNAMELEN         16
#define PBS_NAMELEN            16
#define OWNERL                 15
#define TIMEUL                  8
#define STATEL                  1
#define LOCL                   15

/* original qstat options */
#define GETOPT_ARGS "aeE:filn1qrsu:xGMQRBW:-:"

/* defines for alternative display formats */
#define ALT_DISPLAY_a 1 /* -a option - show all jobs */
#define ALT_DISPLAY_i 2 /* -i option - show not running */
#define ALT_DISPLAY_r 4 /* -r option - show only running */
#define ALT_DISPLAY_u 8 /* -u option - list user's jobs */
#define ALT_DISPLAY_n 0x10 /* -n option - add node list */
#define ALT_DISPLAY_s 0x20 /* -s option - add scheduler comment */
#define ALT_DISPLAY_R 0x40 /* -R option - add SRFS info */
#define ALT_DISPLAY_q 0x80 /* -q option - alt queue display */
#define ALT_DISPLAY_Mb 0x100 /* show sizes in MB */
#define ALT_DISPLAY_Mw 0x200 /* -M option - show sizes in MW */
#define ALT_DISPLAY_G  0x400 /* -G option - show sizes in GB */
#define ALT_DISPLAY_o  0x800   /* -1 option - add node list on same line */
#define ALT_DISPLAY_l 0x1000 /* -l option - show long job name */

#define MAXLINESIZE 65536

static char 	*myname;

/* globals */
int linesize = 77;
int alias_opt = 0;

static void usage(char *);
static int query_all(edg_wll_Context, edg_wll_JobStat **, edg_wlc_JobId **);

/*
 * print a attribute value string, formating to break a comma if possible
 */

void prt_attr(
	
	char *n,  /* I name */
	char *r,  /* I resource (optional) */
	char *v)  /* I value */
	
{
	char *c;
	char *comma = ",";
	int   first = 1;
	int   l;
	int   start;
	
	start = strlen(n) + 7; /* 4 spaces + ' = ' is 7 */
	
	printf("    %s",  n);
	
	if (r != NULL)
	{
		start += strlen(r) + 1;
		
		printf(".%s", r);
	}
	
	printf(" = ");
	
	c = strtok(v, comma);
	
	while (c != NULL)
	{
		if ((l = strlen(c)) + start < linesize)
		{
			printf("%s", c);
			start += l;
		}
		else
		{
			if (!first)
			{
				printf("\n\t");
				start = 9;
			}
			
			while (*c)
			{
				putchar(*c++);
				
				if (++start > linesize)
				{
					start = 8;
					printf("\n\t");
				}
			}
		}

		if ((c = strtok(NULL, comma)) != NULL)
		{
			first = 0;
			putchar(',');
		}
	}
	
	return;
}  /* END prt_attr() */


/* display when a normal "qstat" is executed */

void display_statjob(

	edg_wll_JobStat      *status,    /* I (data) */
	int                  prtheader, /* I (boolean) */
	int                  full)      /* I (boolean) */
	
{
	edg_wll_TagValue *a;
	int i;

	int l;
	char *c;
	char *jid;
	char *name;
	char *owner;
	char *timeu;
	char *state;
	char *location;
	char format[80];
	char long_name[17];
	time_t epoch;
		
	if (!full)
	{
		sprintf(format, "%%-%ds %%-%ds %%-%ds %%%ds %%%ds %%-%ds\n",
			PBS_MAXSEQNUM + PBS_MAXJOBARRAYLEN + 11,
			PBS_MINNAMELEN,
			OWNERL,
			TIMEUL,
			STATEL,
			LOCL);
		
		if (prtheader)
		{
			/* display summary header TODO - the sizes of these fields should be determined from
			   #defines in pbs_ifl.h */
			printf("Job id                    Name             User            Time Use S Queue\n");
			printf("------------------------- ---------------- --------------- -------- - -----\n");
		}
	}    /* END if (!full) */
		

	for(i = 0; status[i].state; i++)
	{
		jid = NULL;
		name = NULL;
		owner = NULL;
		timeu = NULL;
		state = NULL;
		location = NULL;
		
		if (full)
		{
			printf("Job Id: %s\n", jid = glite_jobid_getUnique(status[i].jobId));
			free(jid);
			
			printstat(status[i], 1);

		}   /* END if (full) */
		else
		{
			/* display summary data */

			/* Job ID (PBS style, so truncated unique part of LB jobid) */
			jid = glite_jobid_getUnique(status[i].jobId);
			if(jid != NULL) {

				c = jid;
				while ((*c != '.') && (*c != '\0'))
					c++;
				
				if (alias_opt)
				{
					/* show the alias as well as the first part of the server name */
					if (*c == '.')
					{
						c++;
						while((*c != '.') && (*c != '\0'))
							c++;
					}
				}
				
				c++;    /* List the first part of the server name, too. */
				while ((*c != '.') && (*c != '\0'))
					c++;
				*c = '\0';
				
				l = strlen(jid);
				
				if (l > (PBS_MAXSEQNUM + PBS_MAXJOBARRAYLEN + 8))
				{
					/* truncate job name */
					c = jid + PBS_MAXSEQNUM + PBS_MAXJOBARRAYLEN + 14;
					
					*c = '\0';
				}
			}
			
			if(status[i].pbs_name) 
			{
				l = strlen(status[i].pbs_name);
					
				/* truncate AName */
				if (l > PBS_NAMELEN)
				{
					l = l - PBS_NAMELEN + 3;
					c = status[i].pbs_name + l;
						
					while ((*c != '/') && (*c != '\0'))
						c++;
						
					if (*c == '\0')
						c = a->value + l;
					
					strcpy(long_name, "...");
					strcat(long_name, c);
					c = long_name;
				}
				else
				{
					c = status[i].pbs_name;
				}
					
				name = c;
			}
			
			if(status[i].pbs_owner)
			{
				c = status[i].pbs_owner;
						
				while ((*c != '@') && (*c != '\0'))
					c++;
				
				*c = '\0';
						
				l = strlen(status[i].pbs_owner);
				if (l > OWNERL)
				{
					c = status[i].pbs_owner + OWNERL;
					*c = '\0';
				}
				owner = status[i].pbs_owner;
			}

			if(status[i].pbs_resource_usage) {
				edg_wll_TagValue *tag;

				for(tag = status[i].pbs_resource_usage; tag->tag; tag++) {
					if(!strcmp(tag->tag, "cput")) {

						l = strlen(tag->value);
						if (l > TIMEUL)
						{
							c = tag->value + TIMEUL;
								*c = '\0';
						}
						timeu = tag->value;
						break;
					}
				}
			}

			if(status[i].pbs_state) {
				l = strlen(status[i].pbs_state);
				if (l > STATEL)
				{
					c = status[i].pbs_state + STATEL;
					*c = '\0';
				}
				state = status[i].pbs_state;
			}

			if(status[i].pbs_queue) {
				c = status[i].pbs_queue;
					
				while ((*c != '@') && (*c != '\0'))
					c++;
				
				*c = '\0';
					
				l = strlen(status[i].pbs_queue);
				if (l > LOCL)
				{
					c = status[i].pbs_queue + LOCL;
					*c = '\0';
				}
				location = status[i].pbs_queue;
				
			}

			if (timeu == NULL)
				timeu = "0";
			
			/* display summary data */
			
			printf(format,
			       jid,
			       name,
			       owner,
			       timeu,
			       state,
			       location);
		}  /* END else (full) */
		
		if (full)
			printf("\n");

	}  /* END for */
	
	return;
}  /* END display_statjob() */


int main(int argc,char *argv[])
{
	edg_wll_Context	sctx[MAX_SERVERS];
	char		*pc, *servers[MAX_SERVERS];
	int		c, i, result=0, nsrv=0, histflags = 0;
	int             f_opt = 0, alt_opt = 0;
	
	myname = argv[0];
	printf("\n");

	while((c = getopt(argc, argv, GETOPT_ARGS)) != EOF) {
		switch(c) {
		case '1':
			/* do not break lines in output */
			alt_opt |= ALT_DISPLAY_o;
			break;

		case 'a':
			/* alternative display */
			alt_opt |= ALT_DISPLAY_a;
			break;

		case 'e':
			/* display only jobs in executable queues */
			break;

		case 'E':
			/* not documented */
			break;

		case 'i':
			/* do not display running jobs */
			alt_opt |= ALT_DISPLAY_i;
			break;
			
		case 'n':
			/* display also allocated nodes */
			alt_opt |= ALT_DISPLAY_n;
			break;

		case 'q':
			/* display queue status */
			alt_opt |= ALT_DISPLAY_q;
			break;

		case 'r':
			/* display only running (or suspended) jobs */
			alt_opt |= ALT_DISPLAY_r;
			break;
		       
		case 's':
			/* display also job comments */
			alt_opt |= ALT_DISPLAY_s;
			break;

		case 'u':
			/* display jobs owned by given users */
			alt_opt |= ALT_DISPLAY_u;
			break;

		case 'R':
			/* display also disk reservation information */
			alt_opt |= ALT_DISPLAY_R;
			break;

		case 'G':
			/* show size in GB units */
			alt_opt |= ALT_DISPLAY_G;
			break;

		case 'M':
			/* display size in Megawords */
			alt_opt |= ALT_DISPLAY_Mw;
			break;

		case 'f':
			/* display full job status */
			if(alt_opt) {
				fprintf(stderr, "%s: option -f conflicts with other display options\n", myname);
				usage(myname);
				exit(2);
			}
			f_opt = 1;
			break;

		case 'x':
			/* display output in XML (implies -f) */
			break;

		case 'B':
			/* display batch server status */
			break;

		case 'l':
			/* display long name of job */
			alt_opt |= ALT_DISPLAY_l;
			break;

		case 'Q':
			/* display queue status */
			break;

		case '-':
			/* handle '--' options */
			if(optarg && !strcmp(optarg, "version")) {
			}
			if(optarg && !strcmp(optarg, "about")) {
			}
			if(optarg && !strcmp(optarg, "help")) {
			}
			break;

		case 'W':
			/* display format options */
			pc = optarg;
			
			while (*pc) {
				switch (*pc) {
					
				case 'a':
					alt_opt |= ALT_DISPLAY_a;
					break;

				case 'i':
					alt_opt |= ALT_DISPLAY_i;
					break;

				case 'r':
					alt_opt |= ALT_DISPLAY_r;
					break;

				case 'u':
					/* note - u option is assumed to be last in  */
					/* string and all remaining is the name list */
					alt_opt |= ALT_DISPLAY_u;
					while (*++pc == ' ');
					pc = pc + strlen(pc) - 1; /* for the later incr */
					break;
					
				case 'n':
					alt_opt |= ALT_DISPLAY_n;
					break;

				case 's':
					alt_opt |= ALT_DISPLAY_s;
					break;

				case 'q':
					break;

				case 'R':
					alt_opt |= ALT_DISPLAY_R;
					break;

				case 'G':
					alt_opt |= ALT_DISPLAY_G;
					break;

				case 'M':
					alt_opt |= ALT_DISPLAY_Mw;
					break;

				case ' ':
					break;  /* ignore blanks */
					
				default:
					break;
				}
				
				++pc;
			}

		case '?':
			break;

		default:
			break;
		}
	}

	/* certain combinations are not allowed */

	c = alt_opt & (ALT_DISPLAY_a | ALT_DISPLAY_i | ALT_DISPLAY_r | ALT_DISPLAY_q);

	if ((c != 0) &&
	    (c != ALT_DISPLAY_a) &&
	    (c != ALT_DISPLAY_i) &&
	    (c != ALT_DISPLAY_r) &&
	    (c != ALT_DISPLAY_q)) {
		fprintf(stderr, "%s: conflicting options\n", myname);
		usage(myname);
		exit(2);
	}

	c = alt_opt & (ALT_DISPLAY_Mw | ALT_DISPLAY_G);
	if (c == (ALT_DISPLAY_Mw | ALT_DISPLAY_G))  {
		fprintf(stderr, "%s: conflicting options\n", myname);
		usage(myname);
		exit(2);
	}

	if ((alt_opt & ALT_DISPLAY_q) && (f_opt == 1)) {
		fprintf(stderr, "%s: conflicting options\n", myname);
		usage(myname);
		exit(2);
	}

	if ((alt_opt & ALT_DISPLAY_o) && !((alt_opt & ALT_DISPLAY_n) || (f_opt)))
	{
		fprintf(stderr, "%s: conflicting options\n", myname);
		usage(myname);
		exit(2);
	}

	if (alt_opt & ALT_DISPLAY_o)
	{
		linesize = MAXLINESIZE;
		alt_opt &= ~ALT_DISPLAY_o;
	}

	if ( edg_wll_InitContext(&sctx[0]) ) {
		fprintf(stderr,"cannot initialize edg_wll_Context\n");
		exit(1);
	}

	if ( optind >= argc )  {
		edg_wll_JobStat		*statesOut;
		edg_wlc_JobId		*jobsOut;

		jobsOut = NULL;
		statesOut = NULL;
		if ( (result = query_all(sctx[0], &statesOut, &jobsOut)) ) 
			dgerr(sctx[0], "edg_wll_QueryJobs");
		else 
			display_statjob(statesOut, 1, f_opt);

		if ( jobsOut ) {
			for (i=0; jobsOut[i]; i++) edg_wlc_JobIdFree(jobsOut[i]);
			free(jobsOut);
		}
		if ( statesOut ) {
			for (i=0; statesOut[i].state; i++) edg_wll_FreeStatus(&statesOut[i]);
			free(statesOut);
		}
		edg_wll_FreeContext(sctx[0]);

		return result;
	} 

	for (i = optind ; i < argc; i++ ) {
		int		j;
		char		*bserver;
		edg_wlc_JobId 	job;		
		edg_wll_JobStat status[2];

		memset(&status, 0, sizeof status);
	
		if (edg_wlc_JobIdParse(argv[i], &job)) {
			fprintf(stderr,"%s: %s: cannot parse jobId\n", myname, argv[i]);
			continue;
		}
		bserver = edg_wlc_JobIdGetServer(job);
		if (!bserver) {
			fprintf(stderr,"%s: %s: cannot extract bookkeeping server address\n", myname, argv[i]);
			edg_wlc_JobIdFree(job);
			continue;
		}
		for ( j = 0; j < nsrv && strcmp(bserver, servers[j]); j++ );
		if ( j == nsrv ) {
			if ( i > optind ) 
				edg_wll_InitContext(&sctx[j]);
			nsrv++;
			servers[j] = bserver;
		}

		if (edg_wll_JobStatus(sctx[j], job, EDG_WLL_STAT_CLASSADS | EDG_WLL_STAT_CHILDREN |  EDG_WLL_STAT_CHILDSTAT | histflags, status)) {
			dgerr(sctx[j],"edg_wll_JobStatus"); result = 1; 
		} else display_statjob(status, 1, f_opt);

		if (job) edg_wlc_JobIdFree(job);
		if (status[0].state) edg_wll_FreeStatus(status);
		
	}
	for ( i = 0; i < nsrv; i++ ) edg_wll_FreeContext(sctx[i]);
	
	return result;
}


static void
usage(char *name)
{
	fprintf(stderr, 
		"Usage: \n\n"
		"%s [-f [-1]] [-l] [-W site_specific] [-x] [job_identifier... | destination...]\n\n"
		"%s [-a|-i|-r|-e] [-l] [-n [-1]] [-s]  [-G|-M]  [-R]  [-u  user_list] [job_identifier... |  destination...]\n",
		name, name);
}


static int
query_all(edg_wll_Context ctx, edg_wll_JobStat **statesOut, edg_wlc_JobId **jobsOut)
{
	edg_wll_QueryRec        jc[2];
	int			ret;

	memset(jc, 0, sizeof jc);
	jc[0].attr = EDG_WLL_QUERY_ATTR_OWNER;
       	jc[0].op = EDG_WLL_QUERY_OP_EQUAL;
	jc[0].value.c = NULL;	/* is NULL, peerName filled in on server side */
	jc[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	if ( (ret = edg_wll_QueryJobs(ctx, jc, 0, jobsOut, statesOut)) ) {
		if ( ret == E2BIG ) {
			int r;
			if ( edg_wll_GetParam(ctx, EDG_WLL_PARAM_QUERY_RESULTS, &r) ) return ret;
			if ( r != EDG_WLL_QUERYRES_LIMITED ) return ret;

			printf("Warning: only limited result returned!\n");
			return 0;
		} else return ret;
	}

	return ret;
}

static void
dgerr(edg_wll_Context ctx,char *where)
{
	char 	*etxt,*edsc;

	edg_wll_Error(ctx,&etxt,&edsc);
	fprintf(stderr,"%s: %s: %s",myname,where,etxt);
	if (edsc) fprintf(stderr," (%s)",edsc);
	putc('\n',stderr);
	free(etxt); free(edsc);
}

static void printstat(edg_wll_JobStat stat, int level)
{
	char		*s, *j1,*j2, ind[10];
	int 		i;


	for (i=0; i < level; i++)
		ind[i]='\t';
	ind[i]='\0';
	
	s = edg_wll_StatToString(stat.state); 
/* print whole flat structure */
	printf("%sstate : %s\n", ind, s);
	printf("%sjobId : %s\n", ind, j1 = edg_wlc_JobIdUnparse(stat.jobId)); free(j1);
	printf("%sowner : %s\n", ind, stat.owner);
	printf("%spayload_owner : %s\n", ind, stat.payload_owner);
	switch (stat.jobtype) {
		case EDG_WLL_STAT_SIMPLE:
			printf("%sjobtype : SIMPLE\n", ind);
			break;
		case EDG_WLL_STAT_DAG:
			printf("%sjobtype : DAG\n", ind);
                        break;
		case EDG_WLL_STAT_COLLECTION:
			printf("%sjobtype : COLLECTION\n", ind);
                        break;
		case EDG_WLL_STAT_PBS:
			printf("%sjobtype : PBS\n", ind);
                        break;
		case EDG_WLL_STAT_CONDOR:
			printf("%sjobtype : CONDOR\n", ind);
                        break;
		case EDG_WLL_STAT_CREAM:
			printf("%sjobtype : CREAM\n", ind);
                        break;
		case EDG_WLL_STAT_FILE_TRANSFER:
			printf("%sjobtype : FILE_TRANSFER\n", ind);
                        break;
		case EDG_WLL_STAT_FILE_TRANSFER_COLLECTION:
			printf("%sjobtype : FILE_TRANSFER_COLLECTION\n", ind);
                        break;
		default:
			printf("%sjobtype : UNKNOWN\n", ind);
			break;
	}
	printf("%sparent_job : %s\n", ind,
			j2 = edg_wlc_JobIdUnparse(stat.parent_job));
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
	printf("%sce_node : %s\n", ind, stat.ce_node);
	printf("%ssubjob_failed : %d\n", ind, stat.subjob_failed);
	printf("%sdone_code : %s\n", ind, edg_wll_done_codeToString(stat.done_code));
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
                for (i=1; i<=stat.stateEnterTimes[0]; i++) {
			time_t	st = stat.stateEnterTimes[i];

			printf("%s%14s  %s", ind, edg_wll_StatToString(i-1), st == 0 ? 
			"    - not available -\n" : ctime(&st));
		}
	printf("%slastUpdateTime : %ld.%06ld\n", ind, stat.lastUpdateTime.tv_sec,stat.lastUpdateTime.tv_usec);
	printf("%sexpectUpdate : %d\n", ind, stat.expectUpdate);
	printf("%sexpectFrom : %s\n", ind, stat.expectFrom);
	printf("%sacl : %s\n", ind, stat.acl);
	printf("%saccess rights : \n%s", ind, stat.access_rights);
	printf("%spayload_running: %d\n", ind, stat.payload_running);
	if (stat.possible_destinations) {
		printf("%spossible_destinations : \n", ind);
		for (i=0; stat.possible_destinations[i]; i++) 
			printf("%s\t%s \n", ind, stat.possible_destinations[i]);
	}
	if (stat.possible_ce_nodes) {
		printf("%spossible_ce_nodes : \n", ind);
		for (i=0; stat.possible_ce_nodes[i]; i++) 
			printf("%s\t%s \n", ind, stat.possible_ce_nodes[i]);
	}
	printf("%ssuspended : %d\n", ind, stat.suspended);
	printf("%ssuspend_reason : %s\n", ind, stat.suspend_reason);
	printf("%sfailure_reasons : %s\n", ind, stat.failure_reasons);
	printf("%sremove_from_proxy : %d\n", ind, stat.remove_from_proxy);
	printf("%sui_host : %s\n", ind, stat.ui_host);
	if (stat.user_fqans) {
                printf("%suser_fqans : \n", ind);
                for (i=0; stat.user_fqans[i]; i++) 
                        printf("%s\t%s \n", ind, stat.user_fqans[i]);
        }
	printf("%ssandbox_retrieved : %d\n", ind, stat.sandbox_retrieved);
	printf("%sjw_status : %s\n", ind, edg_wll_JWStatToString(stat.jw_status));
	
	printf("%sisb_transfer : %s\n", ind, j1 = edg_wlc_JobIdUnparse(stat.isb_transfer)); free(j1);
	printf("%sosb_transfer : %s\n", ind, j1 = edg_wlc_JobIdUnparse(stat.osb_transfer)); free(j1);
	
	/* PBS state section */
	if (stat.jobtype == EDG_WLL_STAT_PBS) {
		printf("%spbs_state : %s\n", ind, stat.pbs_state);
		printf("%spbs_queue : %s\n", ind, stat.pbs_queue);
		printf("%spbs_owner : %s\n", ind, stat.pbs_owner);
		printf("%spbs_name : %s\n", ind, stat.pbs_name);
		printf("%spbs_reason : %s\n", ind, stat.pbs_reason);
		printf("%spbs_scheduler : %s\n", ind, stat.pbs_scheduler);
		printf("%spbs_dest_host : %s\n", ind, stat.pbs_dest_host);
		printf("%spbs_pid : %d\n", ind, stat.pbs_pid);
		printf("%spbs_resource_usage : %s\n", ind, edg_wll_TagListToString(stat.pbs_resource_usage));
		printf("%spbs_exit_status : %d\n", ind, stat.pbs_exit_status);
		printf("%spbs_error_desc : %s%s\n", ind, 
			(stat.pbs_error_desc) ? "\n" : "", stat.pbs_error_desc);
	}

	/* CREAM state section */
	char *cream_stat_name = edg_wll_CreamStatToString(stat.cream_state);
	if (stat.jobtype == EDG_WLL_STAT_CREAM || cream_stat_name) {
		printf("%scream_state : %s\n", ind, cream_stat_name);
		printf("%scream_owner : %s\n", ind, stat.cream_owner);
		printf("%scream_endpoint : %s\n", ind, stat.cream_endpoint);
		printf("%scream_jdl : %s\n", ind, stat.cream_jdl);
		printf("%scream_reason : %s\n", ind, stat.cream_reason);
		printf("%scream_lrms_id : %s\n", ind, stat.cream_lrms_id);
		printf("%scream_node : %s\n", ind, stat.cream_node);
		printf("%scream_done_code : %d\n", ind, stat.cream_done_code);
		printf("%scream_exit_code : %d\n", ind, stat.cream_exit_code);
		printf("%scream_cancelling : %d\n", ind, stat.cream_cancelling);
		printf("%scream_cpu_time : %d\n", ind, stat.cream_cpu_time);
		printf("%scream_jw_status : %s\n", ind,  edg_wll_JWStatToString(stat.cream_jw_status));
	}
	if (cream_stat_name) free(cream_stat_name);

	/* File Transfer section */
	printf("%sft_compute_job : %s\n", ind, j1 = edg_wlc_JobIdUnparse(stat.ft_compute_job)); free(j1);
	if (stat.ft_sandbox_type == EDG_WLL_STAT_INPUT)
		printf("%sft_sandbox_type : INPUT\n", ind);
	else  if (stat.ft_sandbox_type == EDG_WLL_STAT_OUTPUT)
		printf("%sft_sandbox_type : OUTPUT\n", ind);
	else
		printf("%sft_sandbox_type : UNKNOWN\n", ind);
	printf("%sft_src : %s\n", ind, stat.ft_src);
	printf("%sft_dest : %s\n", ind, stat.ft_dest);
	

	printf("\n");	
	
	free(j2);
	free(s);
}

