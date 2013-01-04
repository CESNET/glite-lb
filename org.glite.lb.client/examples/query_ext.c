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

#define BUFF_LEN		1024
#define MAX_AND_CONDS	20

enum
{
	PQRV_ERR,
	PQRV_EOF,
	PQRV_EMPTY_LINE,
};


static void		printstat(edg_wll_JobStat);
static int		cond_parse(edg_wll_QueryRec ***, int);
static void		dgerr(edg_wll_Context, char *);
static void		free_QueryRec(edg_wll_QueryRec *qr);
static void		printconds(edg_wll_QueryRec **);
static time_t	StrToTime(char *t);
static char	   *TimeToStr(time_t t);

static char	   *myname;
static FILE	   *fin;
static char		buffer[BUFF_LEN+1],
				tmps[500];
static int		query_jobs = 1;
#define 		query_events	(!query_jobs)
static int		query_lbproxy = 0;
#define 		query_bkserver	(!query_lbproxy)
static int		verbose = 0;


static void usage(void)
{
	fprintf(stderr, "Usage: %s [-hvs] [-i file]\n", myname);
	fprintf(stderr, "    -h                 show this help\n");
	fprintf(stderr, "    -v                 increase verbosity level\n");
	fprintf(stderr, "    -s                 results described verbosely (query jobs only)\n");
	fprintf(stderr, "    -m                 server name\n");
	fprintf(stderr, "    -i file            input file (default is stdin)\n\n");
	fprintf(stderr, "    -e                 query events (default is 'query jobs')\n");
	fprintf(stderr, "    -P                 query the L&B Proxy server\n");
	fprintf(stderr, "    -p                 L&B Proxy socket path\n");
	fprintf(stderr, "    -C                 send classad flag with job state queries\n");
	fprintf(stderr, "    -S                 send childstat flag with job state queries\n");
	fprintf(stderr, "    -N                 send no_jobs flag with job state queries\n");
	fprintf(stderr, "    -r type            returned results: limited | all | none\n\n");
	fprintf(stderr, "    -J num             jobs soft limit\n\n");
	fprintf(stderr, "    -E num             events soft limit (query events only)\n\n");
}

int main(int argc,char *argv[])
{
	edg_wll_Context			ctx;
	edg_wll_QueryResults	rslts		= EDG_WLL_QUERYRES_UNDEF;
	edg_wll_QueryRec	  **jc			= NULL,
						  **ec			= NULL;
	edg_wll_JobStat		   *statesOut	= NULL;
	edg_wlc_JobId		   *jobsOut		= NULL;
	edg_wll_Event		   *eventsOut	= NULL;
	char				   *fname		= NULL,
						   *server		= NULL,
						   *proxy_sock	= NULL,
						   *s;
	int						result		= 0,
							jobsLimit	= 0,
							stdisp		= 0,
							eventsLimit	= 0,
							resultsLimit	= 0,
							i, j, ret,
							errCode,
							flags		= 0;

	myname	= argv[0];
	ret		= 0;
	do {
		switch ( getopt(argc,argv,"hvsePp:i:m:r:J:E:CSNR:") ) {
		case 'h': usage();  exit(0);
		case '?': usage(); exit(EINVAL);
		case 'v': verbose = 1; break;
		case 'i': fname = strdup(optarg); break;
		case 'm': server = strdup(optarg); break;
		case 's': stdisp = 1; break;
		case 'e': query_jobs = 0; break;
		case 'P': query_lbproxy = 1; break;
		case 'p': proxy_sock = strdup(optarg); break;
		case 'r':
			if ( !strcasecmp(optarg, "limited") ) rslts = EDG_WLL_QUERYRES_LIMITED;
			else if ( !strcasecmp(optarg, "none") ) rslts = EDG_WLL_QUERYRES_NONE;
			else if ( !strcasecmp(optarg, "all") ) rslts = EDG_WLL_QUERYRES_ALL;
			else { usage(); exit(EINVAL); }
			break;
		case 'J': jobsLimit = atoi(optarg); break;
		case 'E': eventsLimit = atoi(optarg); break;
		case 'R': resultsLimit = atoi(optarg); break;
		case 'C': flags = flags | EDG_WLL_STAT_CHILDSTAT; break;
		case 'S': flags = flags | EDG_WLL_STAT_CLASSADS; break;
		case 'N': flags = flags | 1024; break; //crude, I know
		case -1: ret = 1; break;
		}
	} while ( !ret );

	if ( edg_wll_InitContext(&ctx) ) {
		if ( verbose ) fprintf(stderr,"%s: cannot initialize edg_wll_Context\n",myname);
		exit(1);
	}

	if ( jobsLimit > 0 ) {
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_JOBS_LIMIT, jobsLimit);
		if ( verbose ) printf("Soft query limit for jobs: %d\n", jobsLimit);
	}
	else if ( verbose ) printf("Soft query limit for jobs not set\n");

	if ( resultsLimit > 0 ) {
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_RESULTS, resultsLimit);
		if ( verbose ) printf("Results limit for jobs: %d\n", resultsLimit);
	}
	else if ( verbose ) printf("Results limit for jobs not set\n");

	if ( query_events ) {
		if ( eventsLimit > 0 ) {
			edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_EVENTS_LIMIT, eventsLimit);
			if ( verbose ) printf("Soft query limit for events: %d\n", eventsLimit);
		}
		else if ( verbose ) printf("Soft query limit for events not set\n");
	}

	if ( rslts != EDG_WLL_QUERYRES_UNDEF )
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_RESULTS, rslts);
	else
		edg_wll_GetParam(ctx, EDG_WLL_PARAM_QUERY_RESULTS, &rslts);

	if ( verbose ) printf("When any limit overflows, the returned result set is: %s\n",
							rslts==EDG_WLL_QUERYRES_LIMITED? "limited":
							(rslts==EDG_WLL_QUERYRES_ALL? "unlimited": "empty"));
	
	if ( server ) {
		char *p = strchr(server, ':');
		if ( p ) {
			edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER_PORT, atoi(p+1));
			*p = 0;
		}
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER, server);
		free(server);
	}

	if ( proxy_sock ) {
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_LBPROXY_SERVE_SOCK, proxy_sock);
		free(proxy_sock);
	}

	if ( fname ) {
		fin = fopen(fname, "r");
		if ( !fin ) {
			if ( verbose ) fprintf(stderr, "Can't open given input file %s. Using stdin.\n", fname);
			fin = stdin;
		}
		free(fname);
	} else {
		if ( verbose ) fprintf(stderr, "No input file given. Using stdin.\n");
		fin = stdin;
	}

	jobsOut		= NULL;
	statesOut	= NULL;
	eventsOut	= NULL;


	do
	{
		if ( verbose && (fin == stdin) ) printf("Enter job conditions:\n");
		
		ret = cond_parse(&jc, 1);
		if ( ret == PQRV_ERR ) { result = 1; goto cleanup; }
		if ( query_events && (ret != PQRV_EOF) ) {
			if ( verbose && (fin == stdin) ) printf("Enter event conditions:\n");
			ret = cond_parse(&ec, 0);
			if ( ret == PQRV_ERR ) { result = 1; goto cleanup; }
		}

		if ( (ret == PQRV_EOF) && !jc && (query_jobs || (query_events && !ec)) ) break;

		if ( verbose ) {
			printf("job conditions list: ");
			printconds(jc);
			if ( query_events ) { printf("event condition list: "); printconds(ec); }
		}

		if ( query_jobs ) {
			if ( query_bkserver ) 
				errCode = edg_wll_QueryJobsExt(ctx,
								(const edg_wll_QueryRec **) jc,
								flags, &jobsOut, stdisp? &statesOut: NULL);
			else
				errCode = edg_wll_QueryJobsExtProxy(ctx,
								(const edg_wll_QueryRec **) jc,
								flags, &jobsOut, stdisp? &statesOut: NULL);
		} else {
			if ( query_bkserver )
				errCode = edg_wll_QueryEventsExt(ctx,
								(const edg_wll_QueryRec **) jc,
								(const edg_wll_QueryRec **) ec,
								&eventsOut);
			else
				errCode = edg_wll_QueryEventsExtProxy(ctx,
								(const edg_wll_QueryRec **) jc,
								(const edg_wll_QueryRec **) ec,
								&eventsOut);
		}

		if ( errCode ) {
			dgerr(ctx, NULL);
			if ( (errCode != EPERM) && (errCode != E2BIG) ) goto cycle_cleanup;
        } else if ( verbose ) {
			if ( query_jobs ) printf("Matched jobs: ");
			else printf("Matched events: ");
		}

		if ( verbose ) {
			if ( (query_jobs && jobsOut && jobsOut[0]) ||
				 (query_events && eventsOut && eventsOut[0].type) )
				putchar('\n');
			else printf("No one matches\n");
		}

		if ( query_jobs && jobsOut && !stdisp ) {
			for ( i = 0; jobsOut[i]; i++ ) {
				s = edg_wlc_JobIdUnparse(jobsOut[i]);
				printf("jobId: %s\n", edg_wlc_JobIdUnparse(jobsOut[i]));
				free(s);
				edg_wlc_JobIdFree(jobsOut[i]);
			}
			free(jobsOut);
			jobsOut = NULL;
		}
		if ( query_jobs && statesOut ) {
			if ( stdisp ) for ( i = 0; statesOut[i].state; i++ ) printstat(statesOut[i]);
			for ( i = 0; statesOut[i].state; i++ ) edg_wll_FreeStatus(&statesOut[i]);
			free(statesOut);
			statesOut = NULL;
		}
		if ( query_events && eventsOut ) {
			for ( i = 0; eventsOut[i].type; i++ ) {
				s = edg_wlc_JobIdUnparse(eventsOut[i].any.jobId);
				printf("event: %-11s (jobid %s)\n", edg_wll_EventToString(eventsOut[i].type), s);
				free(s);
			}
			free(eventsOut);
			eventsOut = NULL;
		}

cycle_cleanup:
		if ( jc ) {
			for ( i = 0; jc[i]; i++ ) {
				for ( j = 0; jc[i][j].attr; j++ ) free_QueryRec(&jc[i][j]);
				free(jc[i]);
			}
			free(jc);
		}

		if ( ec ) {
			for ( i = 0; ec[i]; i++ ) {
				for ( j = 0; ec[i][j].attr; j++ ) free_QueryRec(&ec[i][j]);
				free(ec[i]);
			}
			free(ec);
		}
	} while ( ret != PQRV_EOF );

cleanup:
	if ( fin != stdin ) fclose(fin);
	edg_wll_FreeContext(ctx);

	return result;
}

static void free_QueryRec(edg_wll_QueryRec *qr)
{
	switch ( qr->attr )
	{
	case EDG_WLL_QUERY_ATTR_JOBID:
	case EDG_WLL_QUERY_ATTR_PARENT:
		glite_jobid_free((glite_jobid_t)qr->value.j);
		break;

	case EDG_WLL_QUERY_ATTR_STATUS:
	case EDG_WLL_QUERY_ATTR_DONECODE:
	case EDG_WLL_QUERY_ATTR_EXITCODE:
	case EDG_WLL_QUERY_ATTR_RESUBMITTED:
		break;

	case EDG_WLL_QUERY_ATTR_OWNER:
	case EDG_WLL_QUERY_ATTR_LOCATION: 
	case EDG_WLL_QUERY_ATTR_DESTINATION:
	case EDG_WLL_QUERY_ATTR_JDL_ATTR:
		free(qr->value.c);
		break;

	case EDG_WLL_QUERY_ATTR_USERTAG:
		free(qr->attr_id.tag);
		free(qr->value.c);
		break;

	default:
		break;
	}
}

#define isop(c)		((c == '=') || (c == '<') || (c == '>') || (c == '@'))

static char *get_attr_name(char *src, char *dest, int sz)
{
	int		i	= 0,
			ct	= 0;


	while ( (src[i] != '\0') && isblank(src[i]) ) i++;			/* skip whitespaces */
	while ( (src[i] != '\n') && (src[i] != '\0') && !isop(src[i]) && !isblank(src[i]) )
	{
		if ( ct < sz ) 
			dest[ct++] = src[i];
		i++;
	}
	dest[ct] = '\0';

	return src+i;
}

static char *get_op(char *src, edg_wll_QueryOp *op)
{
	int		i = 0;


	while ( (src[i] != '\0') && isblank(src[i]) ) i++;			/* skip whitespaces */

	if ( src[i] == '=' ) *op = EDG_WLL_QUERY_OP_EQUAL;
	else if ( src[i] == '@' ) *op = EDG_WLL_QUERY_OP_WITHIN;
	else if ( src[i] == '>' ) *op = EDG_WLL_QUERY_OP_GREATER;
	else if ( src[i] == '<' )
	{
		if ( (src[i+1] != '\0') && (src[i+1] == '>') ) {
			*op = EDG_WLL_QUERY_OP_UNEQUAL;
			i++;
		} else
			*op = EDG_WLL_QUERY_OP_LESS;
	} 
	else return NULL;

	return src+i+1;
}

static char *get_attr_value(char *src, char *dest, int sz)
{
	int		i	= 0,
			ct	= 0;


	while ( (src[i] != '\0') && isblank(src[i]) ) i++;			/* skip whitespaces */
	if ( src[i] == '"' )
	{
		i++;
		while ( (src[i] != '\n') && (src[i] != '\0') && (src[i] != '"') )
		{
			if ( ct < sz ) 
				dest[ct++] = src[i];
			i++;
		}
		dest[ct] = '\0';
		if ( src[i] != '"' )
			return NULL;

		return src+i+1;
	}

	while ( (src[i] != '\n') && (src[i] != '\0') && (src[i] != ';') && !isblank(src[i]) )
	{
		if ( ct < sz ) 
			dest[ct++] = src[i];
		i++;
	}
	dest[ct] = '\0';

	return src+i;
}

static char *get_job_condition(char *src, edg_wll_QueryRec *cond)
{
	char	   *s;


	s = get_attr_name(src, tmps, 500);

	if ( tmps[0] == '\0' ) return NULL;

	if ( !strcmp(tmps, "jobid") ) cond->attr = EDG_WLL_QUERY_ATTR_JOBID;
	else if ( !strcmp(tmps, "owner") ) cond->attr = EDG_WLL_QUERY_ATTR_OWNER;
	else if ( !strcmp(tmps, "status") ) cond->attr = EDG_WLL_QUERY_ATTR_STATUS;
	else if ( !strcmp(tmps, "location") ) cond->attr = EDG_WLL_QUERY_ATTR_LOCATION;
	else if ( !strcmp(tmps, "destination") ) cond->attr = EDG_WLL_QUERY_ATTR_DESTINATION;
	else if ( !strcmp(tmps, "done_code") ) cond->attr = EDG_WLL_QUERY_ATTR_DONECODE;
	else if ( !strcmp(tmps, "exit_code") ) cond->attr = EDG_WLL_QUERY_ATTR_EXITCODE;
	else if ( !strcmp(tmps, "parent_job") ) cond->attr = EDG_WLL_QUERY_ATTR_PARENT;
	else if ( !strcmp(tmps, "time") ) cond->attr = EDG_WLL_QUERY_ATTR_TIME;
	else if ( !strcmp(tmps, "state_enter_time") ) cond->attr = EDG_WLL_QUERY_ATTR_STATEENTERTIME;
	else if ( !strcmp(tmps, "last_update_time") ) cond->attr = EDG_WLL_QUERY_ATTR_LASTUPDATETIME;
	else if ( !strcmp(tmps, "jdl_attr") ) cond->attr = EDG_WLL_QUERY_ATTR_JDL_ATTR;
	else if ( !strcmp(tmps, "job_type") ) cond->attr = EDG_WLL_QUERY_ATTR_JOB_TYPE;
	else if ( !strcmp(tmps, "vm_status") ) cond->attr = EDG_WLL_QUERY_ATTR_VM_STATUS; 


       /**< When entered current status */
        /**< Time of the last known event of the job */
         /**< Network server aka RB aka WMproxy endpoint */

	else
	{
		cond->attr = EDG_WLL_QUERY_ATTR_USERTAG;
		cond->attr_id.tag = strdup(tmps);
	}

	if ( !(s = get_op(s, &(cond->op))) ) return NULL;

	if ( !(s = get_attr_value(s, tmps, 500)) ) return NULL;

	switch ( cond->attr )
	{
	case EDG_WLL_QUERY_ATTR_JOBID:
	case EDG_WLL_QUERY_ATTR_PARENT:
		if ( glite_jobid_parse(tmps, (glite_jobid_t *)&cond->value.j) )
		{
			fprintf(stderr,"%s: %s: cannot parse jobId\n", myname, tmps);
			return NULL;
		}
		break;

	case EDG_WLL_QUERY_ATTR_OWNER:
		if ( !strcmp("NULL", tmps) )
			cond->value.c = NULL;
		else if ( !(cond->value.c = strdup(tmps)) )
			return 0;
		break;

	case EDG_WLL_QUERY_ATTR_LOCATION:
	case EDG_WLL_QUERY_ATTR_DESTINATION:
	case EDG_WLL_QUERY_ATTR_USERTAG:
		if ( !(cond->value.c = strdup(tmps)) )
			return 0;
		break;

	case EDG_WLL_QUERY_ATTR_JDL_ATTR:
		if ( !(cond->value.c = strdup(tmps)) )
			return 0;
		if ( !(s = get_attr_value(s, tmps, 500)) ) return NULL;
		if ( tmps[0] == '\0' )
		{
			fprintf(stderr,"%s: you need to specify attribute name\n", myname);
			return NULL;
		}
		if ( !(cond->attr_id.tag = strdup(tmps)) )
			return 0;
		break;

	case EDG_WLL_QUERY_ATTR_STATUS:
		if ( 0 > (cond->value.i = edg_wll_StringToStat(tmps))) {
			fprintf(stderr,"%s: invalid status value (%s)\n", myname, tmps);
			return 0;
		}
		break;

	case EDG_WLL_QUERY_ATTR_DONECODE:
	case EDG_WLL_QUERY_ATTR_EXITCODE:
	case EDG_WLL_QUERY_ATTR_RESUBMITTED:
		cond->value.i = atoi(tmps);
		if ( cond->op == EDG_WLL_QUERY_OP_WITHIN )
		{
			if ( !(s = get_attr_value(s, tmps, 500)) ) return NULL;
			if ( tmps[0] == '\0' )
			{
				fprintf(stderr,"%s: second interval boundary not set\n", myname);
				return NULL;
			}
			cond->value2.i = atoi(tmps);
		}
		break;
	case EDG_WLL_QUERY_ATTR_TIME:
		cond->value.t.tv_sec = StrToTime(tmps);
		if ( cond->op == EDG_WLL_QUERY_OP_WITHIN )
		{
			if ( !(s = get_attr_value(s, tmps, 500)) ) return NULL;
			if ( tmps[0] == '\0' )
			{
				fprintf(stderr,"%s: second interval boundary not set\n", myname);
				return NULL;
			}
			cond->value2.t.tv_sec = StrToTime(tmps);
		}
		if ( !(s = get_attr_value(s, tmps, 500)) ) return NULL;
		if ( tmps[0] == '\0' )
		{
			fprintf(stderr,"%s: time condition must be associated with state condition\n", myname);
			return NULL;
		}
		if ( 0 > (cond->value.i = edg_wll_StringToStat(tmps))) {
			fprintf(stderr,"%s: invalid status value (%s)\n", myname, tmps);
			return 0;
		}
        	break;

	case EDG_WLL_QUERY_ATTR_LASTUPDATETIME:
	case EDG_WLL_QUERY_ATTR_STATEENTERTIME:
		cond->value.t.tv_sec = StrToTime(tmps);
		if ( cond->op == EDG_WLL_QUERY_OP_WITHIN )
		{
			if ( !(s = get_attr_value(s, tmps, 500)) ) return NULL;
			if ( tmps[0] == '\0' )
			{
				fprintf(stderr,"%s: second interval boundary not set\n", myname);
				return NULL;
			}
			cond->value2.t.tv_sec = StrToTime(tmps);
		}
		break;

	case EDG_WLL_QUERY_ATTR_JOB_TYPE:
		if ( 0 > (cond->value.i = edg_wll_JobtypeStrToCode(tmps))) {
			fprintf(stderr,"%s: invalid job type (%s)\n", myname, tmps);
			return 0;
		}
		break;
	
	case EDG_WLL_QUERY_ATTR_VM_STATUS:
		if ( 0 > (cond->value.i = edg_wll_StringToVMStat(tmps))) {
			fprintf(stderr,"%s: invalid VM status value (%s)\n", myname, tmps);
			return 0;
		}
		break;

	default:
		break;
	}

	while ( isblank(*s) || (*s == ';') ) s++;			/* skip whitespaces */

	return s;
}

static char *get_event_condition(char *src, edg_wll_QueryRec *cond)
{
	char	   *s;


	s = get_attr_name(src, tmps, 500);

	if ( tmps[0] == '\0' ) return NULL;

	if ( !strcmp(tmps, "time") ) cond->attr = EDG_WLL_QUERY_ATTR_TIME;
	else if ( !strcmp(tmps, "state_enter_time") ) cond->attr = EDG_WLL_QUERY_ATTR_STATEENTERTIME;
	else if ( !strcmp(tmps, "last_update_time") ) cond->attr = EDG_WLL_QUERY_ATTR_LASTUPDATETIME;
	else if ( !strcmp(tmps, "jdl_attr") ) cond->attr = EDG_WLL_QUERY_ATTR_JDL_ATTR;
	else if ( !strcmp(tmps, "level") ) cond->attr = EDG_WLL_QUERY_ATTR_LEVEL;
	else if ( !strcmp(tmps, "host") ) cond->attr = EDG_WLL_QUERY_ATTR_HOST;
	else if ( !strcmp(tmps, "source") ) cond->attr = EDG_WLL_QUERY_ATTR_SOURCE;
	else if ( !strcmp(tmps, "instance") ) cond->attr = EDG_WLL_QUERY_ATTR_INSTANCE;
	else if ( !strcmp(tmps, "event_type") ) cond->attr = EDG_WLL_QUERY_ATTR_EVENT_TYPE;
	else
	{
		cond->attr = EDG_WLL_QUERY_ATTR_USERTAG;
		cond->attr_id.tag = strdup(tmps);
	}

	if ( !(s = get_op(s, &(cond->op))) ) return NULL;

	if ( !(s = get_attr_value(s, tmps, 500)) ) return NULL;

	switch ( cond->attr )
	{
	case EDG_WLL_QUERY_ATTR_USERTAG:
	case EDG_WLL_QUERY_ATTR_HOST:
	case EDG_WLL_QUERY_ATTR_INSTANCE:
	case EDG_WLL_QUERY_ATTR_JDL_ATTR:
		if ( !(cond->value.c = strdup(tmps)) )
			return 0;
		break;

	case EDG_WLL_QUERY_ATTR_SOURCE:
		if ( !strcasecmp(tmps, "UserInterface") ) cond->value.i = EDG_WLL_SOURCE_USER_INTERFACE;
		else if ( !strcasecmp(tmps, "NetworkServer") ) cond->value.i = EDG_WLL_SOURCE_NETWORK_SERVER;
		else if ( !strcasecmp(tmps, "WorkloadManager") ) cond->value.i = EDG_WLL_SOURCE_WORKLOAD_MANAGER;
		else if ( !strcasecmp(tmps, "BigHelper") ) cond->value.i = EDG_WLL_SOURCE_BIG_HELPER;
		else if ( !strcasecmp(tmps, "JobController") ) cond->value.i = EDG_WLL_SOURCE_JOB_SUBMISSION;
		else if ( !strcasecmp(tmps, "LogMonitor") ) cond->value.i = EDG_WLL_SOURCE_LOG_MONITOR;
		else if ( !strcasecmp(tmps, "LRMS") ) cond->value.i = EDG_WLL_SOURCE_LRMS;
		else if ( !strcasecmp(tmps, "Application") ) cond->value.i = EDG_WLL_SOURCE_APPLICATION;
		else
		{
			fprintf(stderr,"%s: invalid source value (%s)\n", myname, tmps);
			return NULL;
		}
		break;

	case EDG_WLL_QUERY_ATTR_EVENT_TYPE:
	case EDG_WLL_QUERY_ATTR_LEVEL:
		cond->value.i = atoi(tmps);
		if ( cond->op == EDG_WLL_QUERY_OP_WITHIN )
		{
			if ( !(s = get_attr_value(s, tmps, 500)) ) return NULL;
			if ( tmps[0] == '\0' )
			{
				fprintf(stderr,"%s: second interval boundary not set\n", myname);
				return NULL;
			}
			cond->value2.i = atoi(tmps);
		}
		break;

	case EDG_WLL_QUERY_ATTR_TIME:
		cond->value.t.tv_sec = StrToTime(tmps);
		if ( cond->op == EDG_WLL_QUERY_OP_WITHIN )
		{
			if ( !(s = get_attr_value(s, tmps, 500)) ) return NULL;
			if ( tmps[0] == '\0' )
			{
				fprintf(stderr,"%s: second interval boundary not set\n", myname);
				return NULL;
			}
			cond->value2.t.tv_sec = StrToTime(tmps);
		}
		break;

	default:
		break;
	}

	while ( isblank(*s) || (*s == ';') ) s++;			/* skip whitespaces */

	return s;
}

static int cond_parse(edg_wll_QueryRec ***pcond, int jobs)
{
	edg_wll_QueryRec  **cond = NULL;
	int					n,
						icond,
						ret;
	char			   *s;


	
	n = 0;
	if ( !(cond = malloc(sizeof(*cond))) )
	{
		perror("cond_parse()");
		exit(ENOMEM);
	}

	while ( 1 )
	{
		if ( !fgets(buffer, BUFF_LEN, fin) )
		{
			if ( !feof(fin) )
			{
				fprintf(stderr, "parse_query_cond(): file reading error\n");
				exit(EINVAL);
			}

			ret = PQRV_EOF;
			goto cleanup;
		}

		if ( buffer[0] == '\n' )
		{
			ret = PQRV_EMPTY_LINE;
			goto cleanup;
		}

		if ( !(cond[n] = (edg_wll_QueryRec *) malloc(sizeof(edg_wll_QueryRec))) )
		{
			perror("cond_parse()");
			exit(ENOMEM);
		}
		s = buffer;
		icond = 0;
		while ( 1 )
		{
			if ( jobs )
				s = get_job_condition(s, &(cond[n][icond]));
			else
				s = get_event_condition(s, &(cond[n][icond]));

			if ( !s )
			{
				free(cond[n]);
				cond[n] = NULL;
				ret = PQRV_ERR;
				goto cleanup;
			}
			icond++;
			if ( !(cond[n] = realloc(cond[n], (icond+1)*sizeof(edg_wll_QueryRec))) )
			{
				perror("cond_parse()");
				exit(ENOMEM);
			}
			cond[n][icond].attr = EDG_WLL_QUERY_ATTR_UNDEF;
			if ( (*s == '\n') || (*s == '\0') )
				break;
		}

		n++;
		if ( !(cond = realloc(cond, (n+1)*sizeof(*cond))) )
		{
			perror("cond_parse()");
			exit(ENOMEM);
		}
		cond[n] = NULL;
	}

cleanup:
	if ( !n )
	{
		free(cond);
		*pcond = NULL;
	}
	else
		*pcond = cond;

	return ret;
}

static void printconds(edg_wll_QueryRec **cond)
{
	int		i, j;
	int		any = 0;
	char   *s;


	if ( !cond )
	{
		printf("empty\n");
		return ;
	}

	for ( i = 0; cond[i]; i++ )
	{
		if ( cond[i][0].attr )
		{
			any = 1;
			if ( i )
				printf(" AND (");
			else
				printf("(");
		}
		for ( j = 0; cond[i][j].attr; j++ )
		{
			if ( j )
				printf(" OR ");
			printf("%s", edg_wll_QueryAttrNames[cond[i][j].attr]);
			switch ( cond[i][j].op )
			{
			case EDG_WLL_QUERY_OP_EQUAL: printf("="); break;
			case EDG_WLL_QUERY_OP_UNEQUAL: printf("<>"); break;
			case EDG_WLL_QUERY_OP_LESS: printf("<"); break;
			case EDG_WLL_QUERY_OP_GREATER: printf(">"); break;
			case EDG_WLL_QUERY_OP_WITHIN: printf("@"); break;
			case EDG_WLL_QUERY_OP_CHANGED: printf("->"); break;
			}
			switch ( cond[i][j].attr )
			{
			case EDG_WLL_QUERY_ATTR_JOBID:
			case EDG_WLL_QUERY_ATTR_PARENT:
				s = edg_wlc_JobIdUnparse(cond[i][j].value.j);
				printf("%s", s);
				free(s);
				break;
			case EDG_WLL_QUERY_ATTR_OWNER:
			case EDG_WLL_QUERY_ATTR_LOCATION: 
			case EDG_WLL_QUERY_ATTR_DESTINATION:
			case EDG_WLL_QUERY_ATTR_HOST:
			case EDG_WLL_QUERY_ATTR_INSTANCE:
			case EDG_WLL_QUERY_ATTR_USERTAG:
			case EDG_WLL_QUERY_ATTR_JDL_ATTR:
				printf("%s", cond[i][j].value.c);
				break;
			case EDG_WLL_QUERY_ATTR_STATUS:
			case EDG_WLL_QUERY_ATTR_DONECODE:
			case EDG_WLL_QUERY_ATTR_EXITCODE:
			case EDG_WLL_QUERY_ATTR_RESUBMITTED:
			case EDG_WLL_QUERY_ATTR_SOURCE:
				if ( cond[i][j].op == EDG_WLL_QUERY_OP_WITHIN )
					printf("[%d,%d]", cond[i][j].value.i, cond[i][j].value2.i);
				else
					printf("%d", cond[i][j].value.i);
				break;
			case EDG_WLL_QUERY_ATTR_TIME:
				if ( cond[i][j].op == EDG_WLL_QUERY_OP_WITHIN )
				{
					printf("[%s,", TimeToStr(cond[i][j].value.t.tv_sec));
					printf("%s] & state = %d", TimeToStr(cond[i][j].value2.t.tv_sec), cond[i][j].attr_id.state);
				}
				else
					printf("%s & state = %d", TimeToStr(cond[i][j].value.t.tv_sec), cond[i][j].attr_id.state);
				break;
			default:
				break;
			}
		}
		if ( j )
			printf(")");
	}
	if ( any ) printf("\n");
}

static void printstat(edg_wll_JobStat stat)
{
	char		*s, *j;


	s = edg_wll_StatToString(stat.state); 
	printf("STATUS:\t%s\n\n",s);
/* print whole flat structure */
	printf("state : %s\n", s);
	printf("jobId : %s\n", j = edg_wlc_JobIdUnparse(stat.jobId));
	printf("owner : %s\n", stat.owner);
	printf("jobtype : %d\n", stat.jobtype);
	if (stat.jobtype) {;
		printf("seed : %s\n", stat.seed);
		printf("children_num : %d\n", stat.children_num);
// XXX deal with subjobs (children, children_hist, children_states)
	}
	printf("condorId : %s\n", stat.condorId);
	printf("globusId : %s\n", stat.globusId);
	printf("localId : %s\n", stat.localId);
	printf("jdl : %s\n", stat.jdl);
	printf("matched_jdl : %s\n", stat.matched_jdl);
	printf("destination : %s\n", stat.destination);
	printf("network server : %s\n", stat.network_server);
	printf("condor_jdl : %s\n", stat.condor_jdl);
	printf("rsl : %s\n", stat.rsl);
	printf("reason : %s\n", stat.reason);
	printf("location : %s\n", stat.location);
	printf("ce_node : %s\n", stat.ce_node);
	printf("subjob_failed : %d\n", stat.subjob_failed);
	printf("done_code : %d\n", stat.done_code);
	printf("exit_code : %d\n", stat.exit_code);
	printf("resubmitted : %d\n", stat.resubmitted);
	printf("cancelling : %d\n", stat.cancelling);
	printf("cancelReason : %s\n", stat.cancelReason);
	printf("cpuTime : %d\n", stat.cpuTime);
// XXX user_tags user_values
	printf("stateEnterTime : %ld.%06ld\n", stat.stateEnterTime.tv_sec,stat.stateEnterTime.tv_usec);
	printf("lastUpdateTime : %ld.%06ld\n", stat.lastUpdateTime.tv_sec,stat.lastUpdateTime.tv_usec);
	printf("expectUpdate : %d\n", stat.expectUpdate);
	printf("expectFrom : %s\n", stat.expectFrom);
	
	free(j);
	free(s);
}

time_t StrToTime(char *t)
{
	struct tm       tm;

	memset(&tm,0,sizeof(tm));
	setenv("TZ","UTC",1);
	tzset();
	sscanf(t,"%4d-%02d-%02d %02d:%02d:%02d",
			&tm.tm_year,&tm.tm_mon,&tm.tm_mday,
			&tm.tm_hour,&tm.tm_min,&tm.tm_sec);
	tm.tm_year -= 1900;
	tm.tm_mon--;

	return mktime(&tm);
}

static char     tbuf[256];

char *TimeToStr(time_t t)
{
	struct tm	*tm = gmtime(&t);

	sprintf(tbuf,"'%4d-%02d-%02d %02d:%02d:%02d'",
			tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
			tm->tm_hour,tm->tm_min,tm->tm_sec);

	return tbuf;
}

static void dgerr(edg_wll_Context ctx, char *where)
{
	char	   *errText,
			   *errDesc;


	edg_wll_Error(ctx, &errText, &errDesc);
	if ( where )
		fprintf(stderr, ": %s", where);
	if ( errDesc )
		fprintf(stderr, " (%s)\n", errDesc);
	else
		putc('\n', stderr);
	free(errText);
	free(errDesc);
}

