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

#include "glite/lb/context-int.h"
#ifdef BUILDING_LB_CLIENT
#include "consumer.h"
#else
#include "glite/lb/consumer.h"
#endif
#include "glite/lb/xml_conversions.h"
#include "glite/lb/jobstat.h"

static void dgerr(edg_wll_Context,char *);
static void printstat(edg_wll_JobStat,int);

#define MAX_SERVERS	20

/* these are defined in pbs_ifl.h or qstat.c */
#define PBS_MAXSEQNUM           8
#define PBS_MAXJOBARRAYLEN      6
#define PBS_MINNAMELEN         16
#define OWNERL                 15
#define TIMEUL                  8
#define STATEL                  1
#define LOCL                   15


static char 	*myname;

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

	edg_wll_JobStat      status,    /* I (data) */
	int                  prtheader, /* I (boolean) */
	int                  full)      /* I (boolean) */
	
{
	
	edg_wll_TagValue *a;
	
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
	
#if 0
	mxml_t *DE;
	mxml_t *JE;
	mxml_t *AE;
	mxml_t *RE1;
	mxml_t *JI;
#endif
	
	/* XML only support for full output */
	
	if (DisplayXML == TRUE)
		full = 1;
	
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
	
	if (DisplayXML == TRUE)
	{
		/* create parent */
		
#if 0
		DE = NULL;
		
		MXMLCreateE(&DE, "Data"); 
#endif
	}
	
	{
		jid = NULL;
		name = NULL;
		owner = NULL;
		timeu = NULL;
		state = NULL;
		location = NULL;
		
		if (full)
		{
			if (DisplayXML == TRUE)
			{
#if 0
				JE = NULL;
				MXMLCreateE(&JE, "Job");
				MXMLAddE(DE, JE);
				JI = NULL;
				MXMLCreateE(&JI, "Job_Id");
				MXMLSetVal(JI, p->name,mdfString);
				MXMLAddE(JE, JI);
#endif
			}
			else
			{
				printf("Job Id: %s\n",
				       stat.pbs_name);
			}
			
			/* TODO: no attributes yet a = p->attribs; */
			a = NULL;
			
#if 0
			RE1 = NULL;
#endif
			
			while (a != NULL && a->tag != NULL)
			{
				if (DisplayXML == TRUE)
				{
					/* lookup a->name -> XML attr name */
#if 0
					
					AE = NULL;
					
					if (a->resource != NULL)
					{
						if (RE1 == NULL)
						{
							MXMLCreateE(&RE1, a->name);
							MXMLAddE(JE, RE1);
						}
						
						MXMLCreateE(&AE, a->resource);
						
						MXMLSetVal(AE, a->value, mdfString);
						MXMLAddE(RE1, AE);
					}
					else
					{
						RE1 = NULL;
						MXMLCreateE(&AE, a->name);
						MXMLSetVal(AE, a->value, mdfString);
						MXMLAddE(JE, AE);
					}
#endif
				}
				else
				{
					if (!strcmp(a->tag, ATTR_ctime) ||
					    !strcmp(a->tag, ATTR_etime) ||
					    !strcmp(a->tag, ATTR_mtime) ||
					    !strcmp(a->tag, ATTR_qtime) ||
					    !strcmp(a->tag, ATTR_start_time) ||
					    !strcmp(a->tag, ATTR_comp_time) ||
					    !strcmp(a->tag, ATTR_checkpoint_time) ||
					    !strcmp(a->tag, ATTR_a))
					{
						epoch = (time_t)atoi(a->value);
						prt_attr(a->tag, NULL /* a->resource */, ctime(&epoch));
					}
					else
					{
						prt_attr(a->tag, NULL /* a->resource */, a->value);
						printf("\n");
					}
				}
				
				a++;
			}
		}   /* END if (full) */
		else
		{
			/* display summary data */
			
			if (status.pbs_name != NULL)
			{
				c = status.pbs_name;
				
				while ((*c != '.') && (*c != '\0'))
					c++;
				
				if (alias_opt == TRUE)
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
				
				l = strlen(status.pbs_name);
				
				if (l > (PBS_MAXSEQNUM + PBS_MAXJOBARRAYLEN + 8))
				{
					/* truncate job name */
					
					c = p->name + PBS_MAXSEQNUM + PBS_MAXJOBARRAYLEN + 14;
					
					*c = '\0';
				}
				
				jid = status.pbs_name;
			}
			
			/* TODO: attributes missing a = p->attribs; */
			a = NULL;

			while (a != NULL && a->tag != NULL)
			{
				if (strcmp(a->tag, ATTR_name) == 0)
				{
					l = strlen(a->value);
					
					/* truncate AName */
					if (l > PBS_NAMELEN)
					{
						l = l - PBS_NAMELEN + 3;
						c = a->value + l;
						
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
						c = a->value;
					}
					
					name = c;
				}
				else if (!strcmp(a->tag, ATTR_owner))
				{
					c = a->value;
						
					while ((*c != '@') && (*c != '\0'))
						c++;
					
					*c = '\0';
						
					l = strlen(a->value);
					if (l > OWNERL)
					{
						c = a->value + OWNERL;
						*c = '\0';
					}
					
					owner = a->value;
				}
				else if (!strcmp(a->tag, ATTR_used))
				{
					if (!strcmp(a->resource, "cput"))
					{
						l = strlen(a->value);
						
						if (l > TIMEUL)
							{
								c = a->value + TIMEUL;
								
								*c = '\0';
							}
						
						timeu = a->value;
					}
				}
				else if (!strcmp(a->name, ATTR_state))
				{
					l = strlen(a->value);
					
					if (l > STATEL)
					{
						c = a->value + STATEL;
						
						*c = '\0';
					}
					
					state = a->value;
				}
				else if (!strcmp(a->name, ATTR_queue))
				{
					c = a->value;
					
					while ((*c != '@') && (*c != '\0'))
						c++;
					
					*c = '\0';
					
					l = strlen(a->value);
					
					if (l > LOCL)
					{
						c = a->value + LOCL;
						
						*c = '\0';
					}
					
					location = a->value;
				}
			
				a++;
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
		
		if (DisplayXML != TRUE)
		{
#if 0
			if (full)
				printf("\n");
#endif
		}
	}  /* END for (p = status) */
	
	if (DisplayXML == TRUE)
	{
#if 0
		char *tmpBuf = NULL, *tail = NULL;
		int  bufsize;
		
		MXMLToXString(DE, &tmpBuf, &bufsize, INT_MAX, &tail, TRUE);
		
		MXMLDestroyE(&DE);
		
		fprintf(stdout, "%s\n",
			tmpBuf);
#endif
	}

	return;
}  /* END display_statjob() */


int main(int argc,char *argv[])
{
	edg_wll_Context	sctx[MAX_SERVERS];
	char		*servers[MAX_SERVERS];
	int		i, result=0, nsrv=0, histflags = 0;

	
	myname = argv[0];
	printf("\n");

	if ( argc < 2 || strcmp(argv[1],"--help") == 0 ) { usage(argv[0]); return 0; }

	if ( edg_wll_InitContext(&sctx[0]) ) {
		fprintf(stderr,"cannot initialize edg_wll_Context\n");
		exit(1);
	}

	if ( !strcmp(argv[1], "-all" ) ) {
		edg_wll_JobStat		*statesOut;
		edg_wlc_JobId		*jobsOut;

		jobsOut = NULL;
		statesOut = NULL;
		if ( (result = query_all(sctx[0], &statesOut, &jobsOut)) ) dgerr(sctx[0], "edg_wll_QueryJobs");
		else for ( i = 0; statesOut[i].state; i++ ) printstat(statesOut[i],0);

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

	if ( !strcmp(argv[1], "-x") ) {
		edg_wlc_JobId 	job;		
		edg_wll_JobStat status;

		if ( argc < 3 ) { usage(argv[0]); return 1; }
		if ( edg_wll_InitContext(&sctx[0]) ) {
			fprintf(stderr,"%s: cannot initialize edg_wll_Context\n",myname);
			exit(1);
		}
		edg_wll_SetParam(sctx[0], EDG_WLL_PARAM_LBPROXY_SERVE_SOCK, argv[2]);
		for ( i = 3; i < argc; i++ ) {
			memset(&status, 0, sizeof status);
			if (edg_wlc_JobIdParse(argv[i],&job)) {
				fprintf(stderr,"%s: %s: cannot parse jobId\n", myname, argv[i]);
				continue;
			}
			if ( edg_wll_JobStatusProxy(sctx[0], job, EDG_WLL_STAT_CLASSADS | EDG_WLL_STAT_CHILDREN |  EDG_WLL_STAT_CHILDSTAT, &status)) {
				dgerr(sctx[0], "edg_wll_JobStatusProxy"); result = 1;
			} else printstat(status, 0);

			if ( job ) edg_wlc_JobIdFree(job);
			if ( status.state ) edg_wll_FreeStatus(&status);
		}
		edg_wll_FreeContext(sctx[0]);

		return result;
	}

	for ( i = 1; i < argc; i++ ) {
                if ( !strcmp(argv[i], "-fullhist") ) {
                        histflags = EDG_WLL_STAT_CHILDHIST_THOROUGH;
                        printf("\nFound a FULLHIST flag\n\n");
                }

                if ( !strcmp(argv[i], "-fasthist") ) {
                        histflags = EDG_WLL_STAT_CHILDHIST_FAST;
                        printf("\nFound a FASTHIST flag\n\n");
                }    
	}

	for ( i = 1; i < argc; i++ ) {
		int		j;
		char		*bserver;
		edg_wlc_JobId 	job;		
		edg_wll_JobStat status;

		memset(&status,0,sizeof status);
	
		if ((strcmp(argv[i], "-fullhist"))&&(strcmp(argv[i], "-fasthist"))) {
			if (edg_wlc_JobIdParse(argv[i],&job)) {
				fprintf(stderr,"%s: %s: cannot parse jobId\n", myname,argv[i]);
				continue;
			}
        	        bserver = edg_wlc_JobIdGetServer(job);
                	if (!bserver) {
                        	fprintf(stderr,"%s: %s: cannot extract bookkeeping server address\n", myname,argv[i]);
				edg_wlc_JobIdFree(job);
        	                continue;
                	}
	                for ( j = 0; j < nsrv && strcmp(bserver, servers[j]); j++ );
        	        if ( j == nsrv ) {
                	        if ( i > 0 ) edg_wll_InitContext(&sctx[j]);
                        	nsrv++;
	                        servers[j] = bserver;
        	        }

			if (edg_wll_JobStatus(sctx[j], job, EDG_WLL_STAT_CLASSADS | EDG_WLL_STAT_CHILDREN |  EDG_WLL_STAT_CHILDSTAT | histflags, &status)) {
				dgerr(sctx[j],"edg_wll_JobStatus"); result = 1; 
			} else printstat(status,0);

			if (job) edg_wlc_JobIdFree(job);
			if (status.state) edg_wll_FreeStatus(&status);
		}
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
		"%s [-a|-i|-r|-e] [-l] [-n [-1]] [-s]  [-G|-M]  [-R]  [-u  user_list] [job_identifier... |  destination...]\n"
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
		printf("%spbs_resource_usage : %s%s\n", ind,
		       (stat.pbs_resource_usage) ? "\n" : "", edg_wll_TagListToString(stat.pbs_resource_usage));
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

