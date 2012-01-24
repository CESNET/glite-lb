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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sysexits.h>
#include <getopt.h>

#include "glite/security/glite_gss.h"
#include "glite/lb/context.h"
#include "glite/lb/connpool.h"
#include "notification.h"

#include "stat_fields.h"


static char *me;

static void usage(char *cmd)
{
	if ( !cmd )
	{
		fprintf(stderr,"usage: %s command [options]\n"
			"  where commands are:\n"
			"    new         Create new notification reg.\n"
			"    bind        Binds an notification reg. to a client.\n"
			"    change      Changes notification reg. params.\n"
			"    refresh     Enlarge notification reg. validity.\n"
			"    receive     Binds to an existing notif. registration and listen to server.\n"
			"    drop        Drop the notification reg.\n"
			"    help        Prints this message\n",
			me);
	}
	if ( !cmd || !strcmp(cmd, "new") )
		fprintf(stderr,"\n'new' command usage: %s new [ { -s socket_fd | -a fake_addr } -t requested_validity -j jobid  { -o owner | -O }  -n network_server -v virtual_organization --state state1,state2,... -c -J -B -T -H -f flags]\n"
			"    jobid		Job ID to connect notif. reg. with\n"
			"    owner		Match this owner DN\n"
			"    requested_validity	Validity of notification req. in seconds\n"
			"    flags		Numeric flags, can be also represented by cmdline options bellow\n"
			"    network_server	Match only this network server (WMS entry point)\n"
			"    -O			Match owner - credentials are retrieved from environment\n"
			"    -c			Match only on state change\n"
			"    -S | --state	Match on events resulting in listed (coma-delimited) states\n"
			"    -J | --jdl		Attach JDL to job status being returned\n"
			"    -B | --bootstrap	Also send past events matching conditions\n"
			"    -T | --terminal	Notify only when a job reaches terminal state\n"
			"    -H | --history	Same as -T plus attach a history of all job's Events\n"
			"    -N | --aNonymize	Anonymize all owner data in all messages under this registration\n"
			, me);
	if ( !cmd || !strcmp(cmd, "bind") )
		fprintf(stderr,"\n'bind' command usage: %s bind [ { -s socket_fd | -a fake_addr } -t requested_validity ] notifids \n"
			"    requested_validity	Validity of notification req. in seconds\n"
			"    notifids     	One or more notification ID's\n"
			"    fake_addr   	Fake the client address\n", me);

	if ( !cmd || !strcmp(cmd, "change") )
		fprintf(stderr,"\n'change' command usage: %s change notifid jobid\n"
			"    notifid   	Notification ID.\n"
			"    jobid      Job ID to connect notif. reg. with.\n", me);

	if ( !cmd || !strcmp(cmd, "refresh") )
		fprintf(stderr,"\n'refresh' command usage: %s refresh [-t requested_validity ] notifid\n"
			"    requested_validity	Validity of notification req. in seconds\n"
			"    notifid     	Notification ID.\n", me);
	if ( !cmd || !strcmp(cmd, "receive") ) {
		fprintf(stderr,"\n'receive' command usage: %s receive [ { -s socket_fd | -a fake_addr } ] [-t requested_validity ] [-i timeout] [-r ] [-f field1,field2,...] [notifid]\n"
			"    requested_validity	Validity of notification req. in seconds (default 3600)\n"
			"    notifid     	Notification ID (not used if -s specified).\n"
			"    fake_addr   	Fake the client address.\n"
			"    field1,field2,...	List of status fields to print (only owner by default)\n"
			"    timeout     	Timeout to receive operation in seconds.\n"
			"    -r 	    	Attempt to refresh the notification handle in 1/2 of current validity.\n"
			"\n    -a, -t, and -r are unusable with -s\n"
			, me);
		fprintf(stderr,"\navailable fields:\n\t");
		glite_lb_dump_stat_fields();
		putc(10,stderr);
	}
	if ( !cmd || !strcmp(cmd, "drop") )
		fprintf(stderr,"\n'drop' command usage: %s drop notifid\n"
			"    notifid     Notification to remove.\n", me);
}

int main(int argc,char **argv)
{
	edg_wll_Context		ctx;
	edg_wll_QueryRec  **conditions = NULL;
	time_t				valid = time(NULL) + 999999999;
	char			   *errt, *errd;
	void		*fields = NULL;

	int	sock = -1, flags = 0;
	char	*fake_addr = NULL;
		
//	sleep(20);

	me = argv[0];
	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		exit(1);
	}

	if ( argc < 2 ) {
		usage(NULL); goto cleanup;
	}
	if ( (argc < 2) || 
	!strcmp(argv[1], "help") || !strcmp(argv[1], "--help") || 
	!strcmp(argv[1], "-h") || !strcmp(argv[1], "-?")) 
	{
		usage(NULL); goto cleanup;
	}
	else if ( !strcmp(argv[1], "new") )
	{
		int	c;
		edg_wlc_JobId		jid;
		edg_wll_NotifId		id_out;
		int			attr = 0, i = 0, excl = 0;
		edg_wll_GssCred 	mycred;
                edg_wll_GssStatus	gss_code;
		static struct option long_options[] = {
			{"state", required_argument, 0, 'S'},
			{"jdl", no_argument, 0, 'J'},
			{"bootstrap", no_argument, 0, 'B'},
			{"terminal", no_argument, 0, 'T'},
			{"history", no_argument, 0, 'H'},
			{"anonymize", no_argument, 0, 'N'},
			{0, 0, 0, 0}};
           	int option_index = 0;
		char *single, *statelist, *notif_server;
		edg_wll_JobStatCode single_code;
		int statno, stdelims, sti;
		unsigned int notif_server_port;

#define MAX_NEW_CONDS 7
		conditions = (edg_wll_QueryRec **)calloc(MAX_NEW_CONDS + 1,sizeof(edg_wll_QueryRec *));
		conditions[0] = (edg_wll_QueryRec *)calloc(2,sizeof(edg_wll_QueryRec));

		while ((c = getopt_long(argc-1,argv+1,"j:o:v:n:s:a:t:f:cOS:JBTHN",long_options,&option_index)) > 0) { switch (c) {
			case 'j':
				conditions[i] = (edg_wll_QueryRec *)calloc(2,sizeof(edg_wll_QueryRec));
				conditions[i][0].attr = EDG_WLL_QUERY_ATTR_JOBID;
				conditions[i][0].op = EDG_WLL_QUERY_OP_EQUAL;
				if (edg_wlc_JobIdParse(optarg, &jid) ) {
					fprintf(stderr,"Job ID parameter not set propperly!\n");
					usage("new");
					goto cleanup;
				}
				conditions[i][0].value.j = jid;
				i++;
				edg_wll_GetParam(ctx, EDG_WLL_PARAM_NOTIF_SERVER, &notif_server);
				if (!notif_server) {
					glite_jobid_getServerParts(jid, &notif_server, &notif_server_port); 
					edg_wll_SetParam(ctx, EDG_WLL_PARAM_NOTIF_SERVER, notif_server);
					edg_wll_SetParamInt(ctx, EDG_WLL_PARAM_NOTIF_SERVER_PORT, notif_server_port);
				}
				free(notif_server);
				break;
			case 'o':
				if (excl) { usage("new"); return EX_USAGE; } else excl = 1;
				conditions[i] = (edg_wll_QueryRec *)calloc(2,sizeof(edg_wll_QueryRec));
				conditions[i][0].attr = EDG_WLL_QUERY_ATTR_OWNER;
				conditions[i][0].op = EDG_WLL_QUERY_OP_EQUAL;
				conditions[i][0].value.c = optarg;
				i++;
				break;
			case 'v':
				conditions[i] = (edg_wll_QueryRec *)calloc(2,sizeof(edg_wll_QueryRec));
				conditions[i][0].attr = EDG_WLL_QUERY_ATTR_JDL_ATTR;
				conditions[i][0].op = EDG_WLL_QUERY_OP_EQUAL;
				conditions[i][0].value.c = optarg;
				asprintf(&(conditions[i][0].attr_id.tag), "VirtualOrganisation");
				i++;
				break;
			case 'n':
				conditions[i] = (edg_wll_QueryRec *)calloc(2,sizeof(edg_wll_QueryRec));
				conditions[i][0].attr = EDG_WLL_QUERY_ATTR_NETWORK_SERVER;
				conditions[i][0].op = EDG_WLL_QUERY_OP_EQUAL;
				conditions[i][0].value.c = optarg;
				i++;
				break;
			case 's':
				if (fake_addr) { usage("new"); return EX_USAGE; }
				sock = atoi(optarg); break;
			case 'a':
				if (sock >= 0) { usage("new"); return EX_USAGE; }
				fake_addr = optarg; break;
			case 't':
				valid = time(NULL) + atol(optarg); break;
			case 'f':
				flags |= atoi(optarg); break;
			case 'J':
				flags |= EDG_WLL_STAT_CLASSADS; break;
			case 'B':
				flags |= EDG_WLL_NOTIF_BOOTSTRAP; break;
			case 'T':
				flags |= EDG_WLL_NOTIF_TERMINAL_STATES; break;
			case 'H':
				flags |= EDG_WLL_NOTIF_TERMINAL_STATES | EDG_WLL_NOTIF_HISTORY; break;
			case 'N':
				flags |= EDG_WLL_NOTIF_ANONYMIZE; break;
			case 'c':
				conditions[i] = (edg_wll_QueryRec *)calloc(2,sizeof(edg_wll_QueryRec));
				conditions[i][0].attr = EDG_WLL_QUERY_ATTR_STATUS;
				conditions[i][0].op = EDG_WLL_QUERY_OP_CHANGED;
				i++;
				break;
			case 'O':
				if (excl) { usage("new"); return EX_USAGE; } else excl = 1;
				if ( !edg_wll_gss_acquire_cred_gsi(NULL, NULL, &mycred, &gss_code) )
				{
					conditions[i] = (edg_wll_QueryRec *)calloc(2,sizeof(edg_wll_QueryRec));
					conditions[i][0].attr = EDG_WLL_QUERY_ATTR_OWNER;
					conditions[i][0].op = EDG_WLL_QUERY_OP_EQUAL;
					conditions[i][0].value.c = strdup(mycred->name);
					edg_wll_gss_release_cred(&mycred, NULL);
					i++;
				}
				else {
					fprintf(stderr,"No credentials found! Exiting. \n");	
					goto cleanup;
				}
				break;
			case 'S':
				statelist = optarg;

				statno = 0;
				stdelims = 0;
				for (sti = 0; sti < strlen(statelist); sti++) 
					if (statelist[sti] == ',') stdelims++;
				conditions[i] = (edg_wll_QueryRec *)calloc(stdelims+2,sizeof(edg_wll_QueryRec));
				while((single = strtok(statelist, ","))) {
					single_code = edg_wll_StringToStat(single);
					if (single_code != -1) {
						conditions[i][statno].attr = EDG_WLL_QUERY_ATTR_STATUS;
						conditions[i][statno].op = EDG_WLL_QUERY_OP_EQUAL;
						conditions[i][statno].value.i = single_code;
						statelist = NULL;
						statno++;
					}
					else {
						fprintf(stderr,"'%s' is not a valid state name! Exitting.\n", single);	
						goto cleanup;
					}
				}
				i++;
				break; 
			default:
				usage("new"); return EX_USAGE;
			}
			if (i > MAX_NEW_CONDS) {
				usage("new"); return EX_USAGE;
			}
		}

		if ( !edg_wll_NotifNew(ctx,
					(edg_wll_QueryRec const* const*)conditions,
					flags, sock, fake_addr, &id_out, &valid))
			fprintf(stdout,"notification ID: %s\nvalid: %s (%ld)\n",
					edg_wll_NotifIdUnparse(id_out),
					TimeToStr(valid),
					valid);
		edg_wll_NotifIdFree(id_out);
		if (attr == EDG_WLL_QUERY_ATTR_JOBID) edg_wlc_JobIdFree(jid);
		if (edg_wll_Error(ctx,&errt,&errd)) {
			fprintf(stderr, "%s: %s (%s)\n", me, errt, errd);
			return EX_IOERR;
		}
	}
	else if ( !strcmp(argv[1], "bind") )
	{
		edg_wll_NotifId		nid;
		int			c;

		while ((c = getopt(argc-1,argv+1,"s:a:t:")) > 0) switch (c) {
			case 's':
				if (fake_addr) { usage("bind"); return EX_USAGE; }
				sock = atoi(optarg); break;
			case 'a':
				if (sock >= 0) { usage("bind"); return EX_USAGE; }
				fake_addr = optarg; break;
			case 't':
				valid = time(NULL) + atol(optarg); break;
			default:
				usage("bind"); return EX_USAGE;
		}

		if ( (optind+1 == argc) || edg_wll_NotifIdParse(argv[optind+1], &nid) )
		{
			fprintf(stderr,"Notification ID parameter not set propperly!\n\n");
			usage("bind");
			return EX_USAGE;
		}
		if ( !edg_wll_NotifBind(ctx, nid, sock, fake_addr, &valid) )
			printf("valid until: %s (%ld)\n", TimeToStr(valid), valid);
		edg_wll_NotifIdFree(nid);
		if (edg_wll_Error(ctx,&errt,&errd)) {
			fprintf(stderr, "%s: %s (%s)\n", me, errt, errd);
			return EX_IOERR;
		}
	}
	else if ( !strcmp(argv[1], "receive") )
	{
		edg_wll_JobStat		stat;
		edg_wll_NotifId		nid = NULL;
		int			c;
		char	*field_arg = "owner",*err;
		time_t	client_tout = time(NULL) + 600;
	        int	refresh = 0;
		struct timeval		tout;
		time_t	opt_valid = 0,do_refresh = client_tout + 999999999,now;

		while ((c = getopt(argc-1,argv+1,"s:a:i:f:t:r")) > 0) switch (c) {
			case 's':
				if (fake_addr || refresh || opt_valid) { usage("receive"); return EX_USAGE; }
				sock = atoi(optarg); break;
			case 'a':
				if (sock >= 0) { usage("receive"); return EX_USAGE; }
				fake_addr = optarg; break;
			case 'i':
				client_tout = time(NULL) + atoi(optarg); break;
			case 'f':
				field_arg = optarg; break;
			case 't':
				if (sock >= 0) { usage("receive"); return EX_USAGE; }
				opt_valid = atol(optarg); break;
			case 'r':
				if (sock >= 0) { usage("receive"); return EX_USAGE; }
				refresh = 1; break;
			default:
				usage("receive"); return EX_USAGE;
		}

		if (opt_valid == 0) opt_valid = 3600;

		if ((err = glite_lb_parse_stat_fields(field_arg,&fields))) {
			fprintf(stderr,"%s: invalid argument\n",err);
			return EX_USAGE;
		}

		memset(&stat,0,sizeof stat);

		if (sock == -1) {
			int	param = optind+1;

			if (param == argc)
			{
				fprintf(stderr, "Notification ID parameter not set propperly!\n\n");
				usage("receive");
				return EX_USAGE;
			}

			valid = time(NULL) + opt_valid;

			while (param < argc) {
				if (edg_wll_NotifIdParse(argv[param], &nid)) {
					fprintf(stderr, "Notification ID parameter not set propperly!\n\n");
					usage("receive");
					return EX_USAGE;
				}

				if (edg_wll_NotifBind(ctx, nid, -1, fake_addr, &valid) )
					goto receive_err;
				fprintf(stderr,"notification is valid until: %s (%ld)\n", TimeToStr(valid), valid);
				
				if (nid) edg_wll_NotifIdFree(nid); nid = NULL;
				param++;
			}
			now = time(NULL);
			do_refresh = now + (refresh ? (valid - now)/2 : 999999999);
			if (refresh) fprintf(stderr,"next refresh %s (%ld)\n",
					TimeToStr(do_refresh),do_refresh);
		}

		do {
			edg_wll_NotifId		recv_nid = NULL;
			int	err;

			tout.tv_sec = (client_tout < do_refresh ? 
					client_tout : do_refresh)
					- time(NULL);
			if (tout.tv_sec < 0) tout.tv_sec = 0;
			tout.tv_usec = 0;

			if ( (err = edg_wll_NotifReceive(ctx, sock, &tout, &stat, &recv_nid)) ) {
				edg_wll_NotifIdFree(recv_nid);
				recv_nid = NULL; 

				if (err != ETIMEDOUT) goto receive_err;
			}
			else {
				glite_lb_print_stat_fields(fields,&stat);
				edg_wll_FreeStatus(&stat);
				stat.state = EDG_WLL_JOB_UNDEF;
			}

			if ((now = time(NULL)) >= client_tout) goto cleanup;

			if (refresh && now >= do_refresh) {
				valid = now + opt_valid;
				if (!edg_wll_NotifRefresh(ctx,nid,&valid)) {
					do_refresh = now + (valid - now)/2;
					fprintf(stderr,"notification is valid until: %s (%ld)\n",TimeToStr(valid), valid);
					fprintf(stderr,"next refresh %s (%ld)\n", TimeToStr(do_refresh),do_refresh);
				}
				else {
					char	*et,*ed;
					edg_wll_Error(ctx,&et,&ed);
					do_refresh = now + (valid - now)/2;
					fprintf(stderr,"warning: edg_wll_NotifRefresh failed (%s, %s)\n"
							"next refresh %s (%ld)\n",
							et,ed,TimeToStr(do_refresh),do_refresh);
				}
			}
/* original example */
#if 0
			printf("\nnotification ID: %s\n", edg_wll_NotifIdUnparse(recv_nid)); 
			
			if (stat.state != EDG_WLL_JOB_UNDEF) {
				char	*jobid_s;

				jobid_s = edg_wlc_JobIdUnparse(stat.jobId);
				printf("Jobid:\t%s\nStatus:\t%s\n", 
						jobid_s,
						edg_wll_StatToString(stat.state));
				edg_wll_FreeStatus(&stat);
				free(jobid_s);
				stat.state = EDG_WLL_JOB_UNDEF;
			}
#endif
			
			if (recv_nid) {
				edg_wll_NotifIdFree(recv_nid);
				recv_nid = NULL;
			}
		} while (1); // till timeout....
		return 0;

receive_err:		
		if (stat.state != EDG_WLL_JOB_UNDEF) edg_wll_FreeStatus(&stat);
		if (nid) edg_wll_NotifIdFree(nid);
		edg_wll_NotifCloseFd(ctx);
		edg_wll_NotifClosePool(ctx);

		if (edg_wll_Error(ctx,&errt,&errd))
			fprintf(stderr, "%s: %s (%s)\n", me, errt, errd);

		return EX_IOERR;
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
			fprintf(stderr,"Job ID parameter not set propperly!\n");
			usage("change");
			goto cleanup;
		}

		conditions = (edg_wll_QueryRec **)calloc(2,sizeof(edg_wll_QueryRec *));
	
		conditions[0] = (edg_wll_QueryRec *)calloc(2,sizeof(edg_wll_QueryRec));
		conditions[0][0].attr = EDG_WLL_QUERY_ATTR_JOBID;
		conditions[0][0].op = EDG_WLL_QUERY_OP_EQUAL;
		conditions[0][0].value.j = jid;

		conditions[1] = NULL;

		/*conditions[1] = (edg_wll_QueryRec *)calloc(3, sizeof(edg_wll_QueryRec));
		conditions[1][0].attr = EDG_WLL_QUERY_ATTR_STATUS;
		conditions[1][0].op = EDG_WLL_QUERY_OP_EQUAL;
		conditions[1][0].value.i = EDG_WLL_JOB_DONE;

		conditions[1][1].attr = EDG_WLL_QUERY_ATTR_STATUS;
		conditions[1][1].op = EDG_WLL_QUERY_OP_EQUAL;
		conditions[1][1].value.i = EDG_WLL_JOB_RUNNING;*/

		edg_wll_NotifChange(ctx, nid,
						(edg_wll_QueryRec const * const *) conditions,
						EDG_WLL_NOTIF_REPLACE);
		edg_wlc_JobIdFree(jid);
		edg_wll_NotifIdFree(nid);
	}
	else if ( !strcmp(argv[1], "refresh") )
	{
		edg_wll_NotifId		nid;
		int			c;

		while ((c = getopt(argc-1,argv+1,"t:")) > 0) switch (c) {
			case 't':
				valid = time(NULL) + atol(optarg); break;
			default:
				usage("refresh"); return EX_USAGE;
		}

		if ( (optind+1 == argc) || edg_wll_NotifIdParse(argv[optind+1], &nid) )
		{
			fprintf(stderr,"Notification ID parameter not set propperly!\n\n");
			usage("refresh");
			return EX_USAGE;
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
			fprintf(stderr,"Notification ID parameter not set propperly!\n");
			usage("drop");
			return EX_USAGE;
		}
		edg_wll_NotifDrop(ctx, nid);
		edg_wll_NotifIdFree(nid);
	}
	else {
		usage(NULL);
		return EX_USAGE;
	}
	
cleanup:
	
	if ( conditions )
	{
		/*
		for ( i = 0; conditions[i][0].attr; i++ )
			free(conditions[1]);
		*/
		free(conditions);
	}

	int retval;
	if (retval = edg_wll_Error(ctx,&errt,&errd)) 
		fprintf(stderr, "%s: %s (%s)\n", me, errt, errd);

	edg_wll_NotifCloseFd(ctx);
	edg_wll_FreeContext(ctx);
	edg_wll_poolFree();

	return retval;
}
