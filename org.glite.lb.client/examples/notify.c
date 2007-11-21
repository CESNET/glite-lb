#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "glite/security/glite_gss.h"
#include "glite/lb/context.h"
#include "notification.h"


static char *me;

static char     tbuf[256];

char *TimeToStr(time_t t)
{
	struct tm   *tm = gmtime(&t);

	sprintf(tbuf,"'%4d-%02d-%02d %02d:%02d:%02d'",
			tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
			tm->tm_hour,tm->tm_min,tm->tm_sec);

	return tbuf;
}

static void usage(char *cmd)
{
	if ( !cmd )
	{
		printf("usage: %s command [options]\n"
			"  where commands are:\n"
			"    new         Create new notification reg.\n"
			"    bind        Binds an notification reg. to a client.\n"
			"    change      Changes notification reg. params.\n"
			"    refresh     Enlarge notification reg. validity.\n"
			"    receive     Binds to an existing notif. registration and listen to server.\n"
			"    test        Creates new notif., waits for events and drops notif. after timeout.\n"
			"    drop        Drop the notification reg.\n"
			"    help        Prints this message\n",
			me);
	}
	if ( !cmd || !strcmp(cmd, "new") )
		printf("\n'new' command usage: %s new [-j jobid] [-o owner]\n"
			"    jobid      job ID to connect notif. reg. with\n"
			"    owner	match this owner DN\n"
			, me);
	if ( !cmd || !strcmp(cmd, "bind") )
		printf("\n'bind' command usage: %s bind notifid [fake_addr]\n"
			"    notifid     Notification ID\n"
			"    fake_addr   Fake the client address\n", me);
	if ( !cmd || !strcmp(cmd, "change") )
		printf("\n'change' command usage: %s change notifid jobid\n"
			"    notifid     Notification ID.\n"
			"    jobid       Job ID to connect notif. reg. with.\n", me);
	if ( !cmd || !strcmp(cmd, "refresh") )
		printf("\n'refresh' command usage: %s refresh notifid\n"
			"    notifid     Notification ID.\n", me);
	if ( !cmd || !strcmp(cmd, "receive") )
		printf("\n'receive' command usage: %s receive notifid [-a fake_addr] [-t timeout]\n"
			"    notifid     Notification ID.\n"
			"    fake_addr   Fake the client address.\n"
			"    timeout     Timeout to receive operation in seconds.\n", me);
	if ( !cmd || !strcmp(cmd, "test") )
		printf("\n'test' command usage: %s test jobid\n"
			"    jobid       job ID to connect notif. reg. with\n", me);
	if ( !cmd || !strcmp(cmd, "drop") )
		printf("\n'drop' command usage: %s drop notifid\n"
			"    notifid     Notification to remove.\n", me);
}

int main(int argc,char **argv)
{
	edg_wll_Context		ctx;
	edg_wll_QueryRec  **conditions = NULL;
	time_t				valid;
	char			   *errt, *errd;
	struct timeval		tout = {220, 0};
		


	me = argv[0];
	edg_wll_InitContext(&ctx);

	if ( argc < 2 ) {
		usage(NULL); goto cleanup;
	}
	if ( (argc < 2) || 
	!strcmp(argv[1], "help") || !strcmp(argv[1], "--help") || 
	!strcmp(argv[1], "-h") || !strcmp(argv[1], "-?")) 
	{
		usage(NULL); goto cleanup;
	}
	else if ( !strcmp(argv[1], "test") ) {
		edg_wll_NotifId		nid = NULL;
		edg_wlc_JobId		jid = NULL;
		edg_wll_JobStat		stat;

		if ( (argc < 3) || edg_wlc_JobIdParse(argv[2], &jid) ) {
			printf("Job ID parameter not set propperly!\n");
			usage("new");
			goto cleanup;
		}

		memset(&stat, 0, sizeof(stat));

		conditions = (edg_wll_QueryRec **)calloc(2,sizeof(edg_wll_QueryRec *));
		conditions[0] = (edg_wll_QueryRec *)calloc(2,sizeof(edg_wll_QueryRec));
	
		conditions[0][0].attr = EDG_WLL_QUERY_ATTR_JOBID;
		conditions[0][0].op = EDG_WLL_QUERY_OP_EQUAL;
		conditions[0][0].value.j = jid;

		
		if ( edg_wll_NotifNew(ctx,
					(edg_wll_QueryRec const* const*)conditions,
					-1, NULL,
					&nid, &valid)) goto err;
			
		printf("notification ID: %s\nvalid: %s (%ld)\n",
				edg_wll_NotifIdUnparse(nid),
				TimeToStr(valid),
				valid);

		do {
			edg_wll_NotifId		recv_nid = NULL;

			printf("waiting...\n");
			if ( edg_wll_NotifReceive(ctx, -1, &tout, &stat, &recv_nid) ) {
				edg_wll_NotifIdFree(recv_nid);
				printf("timeout\n");
				break;
			}
			
			printf("Notification received:\n");
			printf("  - notification ID: %s\n", edg_wll_NotifIdUnparse(recv_nid));
			printf("  - job ID: %s\n", edg_wlc_JobIdUnparse(stat.jobId));
			
			if (stat.state != EDG_WLL_JOB_UNDEF) {
				printf("  - job status is: %s\n\n", edg_wll_StatToString(stat.state));
				edg_wll_FreeStatus(&stat);
				stat.state = EDG_WLL_JOB_UNDEF;
			}
			if (recv_nid) { edg_wll_NotifIdFree(recv_nid); recv_nid = NULL; }
		} while (1); // till timeout....
err:		
		if (nid) {
			edg_wll_NotifDrop(ctx, nid);
			edg_wll_NotifIdFree(nid);
			edg_wll_NotifCloseFd(ctx);
		}
		if (stat.state != EDG_WLL_JOB_UNDEF) edg_wll_FreeStatus(&stat);
		if (jid) edg_wlc_JobIdFree(jid);
	}
	else if ( !strcmp(argv[1], "new") )
	{
		int	c;
		edg_wlc_JobId		jid;
		edg_wll_NotifId		id_out;
		char	*job = NULL,*owner = NULL;

		while ((c = getopt(argc-1,argv+1,"j:o:")) > 0) switch (c) {
			case 'j': job = optarg; break;
			case 'o': owner = optarg; break;
			default: usage("new"); goto cleanup;
		}

		if ((!job && !owner) || (job && owner)) { 
			usage("new");
			goto cleanup;
		}

		if ( job && edg_wlc_JobIdParse(job, &jid) ) {
			printf("Job ID parameter not set propperly!\n");
			usage("new");
			goto cleanup;
		}

		conditions = (edg_wll_QueryRec **)calloc(2,sizeof(edg_wll_QueryRec *));
		conditions[0] = (edg_wll_QueryRec *)calloc(2,sizeof(edg_wll_QueryRec));
	
		conditions[0][0].attr = job ? EDG_WLL_QUERY_ATTR_JOBID : EDG_WLL_QUERY_ATTR_OWNER;
		conditions[0][0].op = EDG_WLL_QUERY_OP_EQUAL;
		if (job) conditions[0][0].value.j = jid;
		else conditions[0][0].value.c = owner;

		if ( !edg_wll_NotifNew(ctx,
					(edg_wll_QueryRec const* const*)conditions,
					-1, NULL, &id_out, &valid))
			printf("notification ID: %s\nvalid: %s (%ld)\n",
					edg_wll_NotifIdUnparse(id_out),
					TimeToStr(valid),
					valid);
		edg_wll_NotifIdFree(id_out);
		if (job) edg_wlc_JobIdFree(jid);
	}
	else if ( !strcmp(argv[1], "bind") )
	{
		edg_wll_NotifId		nid;

		if ( (argc < 3) || edg_wll_NotifIdParse(argv[2], &nid) )
		{
			printf("Notification ID parameter not set propperly!\n");
			usage("bind");
			return 1;
		}
		if ( !edg_wll_NotifBind(ctx, nid, -1, (argc<4)? NULL: argv[3], &valid) )
			printf("valid until: %s (%ld)\n", TimeToStr(valid), valid);
		edg_wll_NotifIdFree(nid);
	}
	else if ( !strcmp(argv[1], "receive") )
	{
		edg_wll_JobStat		stat;
		edg_wll_NotifId		nid = NULL;
		char			   *addr = NULL;
		int					i;

		if ( (argc < 3) || edg_wll_NotifIdParse(argv[2], &nid) )
		{
			printf("Notification ID parameter not set propperly!\n");
			usage("receive");
			return 1;
		}

		for ( i = 3; i < argc; i++ )
		{
			if ( !strcmp(argv[i], "-t") )
			{
				if ( argc < i+1 )
				{
					printf("Timeout value not set\n");
					usage("receive");
					return 1;
				}
				tout.tv_sec = atoi(argv[++i]);
			}
			else if ( !strcmp(argv[i], "-a") )
			{
				if ( argc < i+1 )
				{
					printf("Address value not set\n");
					usage("receive");
					return 1;
				}
				addr = strdup(argv[++i]);
			}
			else
			{
				printf("unrecognized option: %s\n", argv[i]);
				usage("receive");
				return 1;
			}
		}

		memset(&stat,0,sizeof stat);

		if ( edg_wll_NotifBind(ctx, nid, -1, addr, &valid) )
			goto receive_err;

		printf("notification is valid until: %s (%ld)\n", TimeToStr(valid), valid);

		do {
			edg_wll_NotifId		recv_nid = NULL;
			
			if ( edg_wll_NotifReceive(ctx, -1, &tout, &stat, &recv_nid) ) {
				edg_wll_NotifIdFree(recv_nid);
				break;
			}
			
			printf("\nnotification ID: %s\n", edg_wll_NotifIdUnparse(recv_nid));
			
			if (stat.state != EDG_WLL_JOB_UNDEF) {
				printf("Job status is : %s\n", 
						edg_wll_StatToString(stat.state));
				edg_wll_FreeStatus(&stat);
				stat.state = EDG_WLL_JOB_UNDEF;
			}
			
			if (recv_nid) {
				edg_wll_NotifIdFree(recv_nid);
				recv_nid = NULL;
			}
		} while (1); // till timeout....

receive_err:		
		if (addr) free(addr);
		if (stat.state != EDG_WLL_JOB_UNDEF) edg_wll_FreeStatus(&stat);
		if (nid) edg_wll_NotifIdFree(nid);
		edg_wll_NotifCloseFd(ctx);
	}
	else if ( !strcmp(argv[1], "change") )
	{
		edg_wlc_JobId		jid;
		edg_wll_NotifId		nid;

		if ( (argc < 3) || edg_wll_NotifIdParse(argv[2], &nid) )
		{
			printf("Notification ID parameter not set propperly!\n");
			usage("bind");
			return 1;
		}
		if ( (argc < 4) || edg_wlc_JobIdParse(argv[3], &jid) )
		{
			printf("Job ID parameter not set propperly!\n");
			usage("change");
			goto cleanup;
		}

		conditions = (edg_wll_QueryRec **)calloc(3,sizeof(edg_wll_QueryRec *));
		conditions[0] = (edg_wll_QueryRec *)calloc(2,sizeof(edg_wll_QueryRec));
		conditions[1] = (edg_wll_QueryRec *)calloc(3, sizeof(edg_wll_QueryRec));
	
		conditions[0][0].attr = EDG_WLL_QUERY_ATTR_JOBID;
		conditions[0][0].op = EDG_WLL_QUERY_OP_EQUAL;
		conditions[0][0].value.j = jid;

		conditions[1][0].attr = EDG_WLL_QUERY_ATTR_STATUS;
		conditions[1][0].op = EDG_WLL_QUERY_OP_EQUAL;
		conditions[1][0].value.i = EDG_WLL_JOB_DONE;

		conditions[1][1].attr = EDG_WLL_QUERY_ATTR_STATUS;
		conditions[1][1].op = EDG_WLL_QUERY_OP_EQUAL;
		conditions[1][1].value.i = EDG_WLL_JOB_RUNNING;

		edg_wll_NotifChange(ctx, nid,
						(edg_wll_QueryRec const * const *) conditions,
						EDG_WLL_NOTIF_REPLACE);
		edg_wlc_JobIdFree(jid);
		edg_wll_NotifIdFree(nid);
	}
	else if ( !strcmp(argv[1], "refresh") )
	{
		edg_wll_NotifId		nid;

		if ( (argc < 3) || edg_wll_NotifIdParse(argv[2], &nid) )
		{
			printf("Notification ID parameter not set propperly!\n");
			usage("bind");
			return 1;
		}
		if ( !edg_wll_NotifRefresh(ctx, nid, &valid) )
			printf("valid until: %s (%ld)\n", TimeToStr(valid), valid);
		edg_wll_NotifIdFree(nid);
	}
	else if ( !strcmp(argv[1], "drop") )
	{
		edg_wll_NotifId		nid;

		if ( (argc < 3) || edg_wll_NotifIdParse(argv[2], &nid) )
		{
			printf("Notification ID parameter not set propperly!\n");
			usage("bind");
			return 1;
		}
		edg_wll_NotifDrop(ctx, nid);
		edg_wll_NotifIdFree(nid);
	}
	else
		printf("bad acction\n");
	
	
cleanup:
	
	if ( conditions )
	{
		/*
		for ( i = 0; conditions[i][0].attr; i++ )
			free(conditions[1]);
		*/
		free(conditions);
	}
	
	edg_wll_NotifCloseFd(ctx);
	
	if (edg_wll_Error(ctx,&errt,&errd))
		fprintf(stderr, "%s: %s (%s)\n", me, errt, errd);

	edg_wll_FreeContext(ctx);

	
	return 0;
}
