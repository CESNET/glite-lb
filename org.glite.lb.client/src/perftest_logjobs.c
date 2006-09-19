#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include "glite/lb/context-int.h"
#include "glite/lb/lb_perftest.h"
#include "glite/lb/log_proto.h"

extern int edg_wll_DoLogEvent(edg_wll_Context context, edg_wll_LogLine logline);
extern int edg_wll_DoLogEventProxy(edg_wll_Context context, edg_wll_LogLine logline);
extern int edg_wll_DoLogEventDirect(edg_wll_Context context, edg_wll_LogLine logline);

#define DEFAULT_SOCKET "/tmp/interlogger.sock"
#define DEFAULT_PREFIX EDG_WLL_LOG_PREFIX_DEFAULT

/*
extern char *optarg;
extern int opterr,optind;

Vzorem by mel byt examples/stresslog.c, ovsem s mirnymi upravami:

1) nacte udalosti jednoho jobu z daneho souboru, rozparsuje a ulozi do pameti
2) je-li pozadan, zaregistruje dany pocet jobid
3) vypise timestamp
4) odesle rozparsovane udalosti pod ruznymi jobid (tolika, kolik ma
   byt jobu - argument) na dany cil (localloger, interlogger, bkserver, proxy)
5) odesle specialni ukoncovaci udalost
6) exit

        read_job("small_job",&event_lines);
        gettimeoday(&zacatek)
        for (i=0; i<1000; i++) {
                for (e=0; event_lines[e]; e++)
                        edg_wll_ParseEvent(event_lines[e],&event[e]);
        }
        gettimeofday(&konec);

        printf("parsovani",konec-zacatek/1000);
                        
        gettimeofday(&zacatek);
                switch (event[e].type) {
                        ...
                        edg_wll_LogBlaBla()
                

*/

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "dst", required_argument, NULL, 'd' },
	{ "test", required_argument, NULL, 't' },
	{ "file", required_argument, NULL, 'f'},
	{ "num", required_argument, NULL, 'n'},
	{ "machine", required_argument, NULL, 'm'},
	{ "nofile", no_argument, NULL, 'N'},
	{ "skip", optional_argument, NULL, 'P'},
	{ "sock", required_argument, NULL, 's'},
	{ "file-prefix", required_argument, NULL, 'p'},
	{ NULL, 0, NULL, 0}
};


int nofile = 0;
char *logfile_prefix = DEFAULT_PREFIX;
char *il_socket = NULL;

void
usage(char *program_name)
{
	fprintf(stderr, "Usage: %s [-d destname] [-t testname] -f filename -n numjobs \n"
		"-h, --help               display this help and exit\n"
                "-d, --dst <destname>     destination component\n"
		"-s, --sock <sockname>    socket name for IL\n"
		"-m, --machine <hostname> destination host\n"
		"-t, --test <testname>    name of the test\n"
		"-f, --file <filename>    name of the file with prototyped job events\n"
		"-n, --num <numjobs>      number of jobs to generate\n"
		"-P, --skip [<numevents>] number of events to skip when sending to IL by IPC\n"
		"-p, --file-prefix <name> file prefix of event files\n"
		"-N, --nofile             do not store events in file for interlogger (if dst==IL)\n",
	program_name);
}


#define FCNTL_ATTEMPTS          5
#define FCNTL_TIMEOUT           1

int edg_wll_DoLogEventIl(
	edg_wll_Context context,
	edg_wll_LogLine logline,
	const char *jobid,
	int len,
	int skip)
{
	static long filepos = 0;
	int ret = 0;
	edg_wlc_JobId jid;
	char *unique, *event_file;
	static int num_event = 0;
	char *event;
	const char *user = " DG.USER=\"michal\"\n";

	/* we must fill in DG.USER */
	/* we need room for user (=strlen(user)), terminating \0 (+1),
	   but we overwrite trailing \n (-1) */
	event = realloc(logline, len + strlen(user));
	if(event == NULL) {
		return(edg_wll_SetError(context, ENOMEM, "realloc()"));
	}
	/* it really does not matter WHERE the key is, so append it at the end */
	memmove(event + len - 1, user, strlen(user) + 1);
	len += strlen(user) - 1;

	if(!nofile) {
		ret = edg_wlc_JobIdParse(jobid, &jid);
		if(ret != 0) 
			return(edg_wll_SetError(context, ret, "edg_wlc_JobIdParse()"));
		unique = edg_wlc_JobIdGetUnique(jid);
		if(unique == NULL) {
			edg_wlc_JobIdFree(jid);
			return(edg_wll_SetError(context, ENOMEM, "edg_wlc_JobIdGetUnique()"));
		}
		asprintf(&event_file, "%s.%s", logfile_prefix, unique);
		if(!event_file) {
			free(unique);
			edg_wlc_JobIdFree(jid);
			return(edg_wll_SetError(context, ENOMEM, "asprintf()"));
		}
		if(edg_wll_log_event_write(context, event_file, event,
					   context->p_tmp_timeout.tv_sec > FCNTL_ATTEMPTS ? context->p_tmp_timeout.tv_sec : FCNTL_ATTEMPTS,
					   FCNTL_TIMEOUT,
					   &filepos)) {
			edg_wll_UpdateError(context, 0, "edg_wll_log_event_write()");
			free(unique);
			edg_wlc_JobIdFree(jid);
			free(event_file);
		}
	}
	if(nofile || 
	   (skip < 0) || 
	   ((skip > 0) && (++num_event % skip == 0)))
		ret = edg_wll_log_event_send(context, il_socket, filepos, 
					     event, len, 1, 
					     &context->p_tmp_timeout);
	if(!nofile) {
		free(unique);
		edg_wlc_JobIdFree(jid);
		free(event_file);
	} else {
		filepos += len;
	}

	return(ret);
}


int
main(int argc, char *argv[])
{

	char 	*destname= NULL,*hostname=NULL,*testname = NULL,*filename = NULL;
	char	*event;
	int 	num_jobs = 1;
	int 	opt;
	int     skip = -1;
	edg_wll_Context ctx;
	enum {
		DEST_UNDEF = 0,
		DEST_LL,
		DEST_IL,
		DEST_PROXY,
		DEST_BKSERVER
	} dest = 0;


	edg_wll_InitContext(&ctx);

	opterr = 0;

	while ((opt = getopt_long(argc,argv,"hd:t:f:m:n:Ns:P::p:",
		long_options, (int *) 0)) != EOF) {

		switch (opt) {
		case 'd': destname = (char *) strdup(optarg); break;
		case 't': testname = (char *) strdup(optarg); break;
		case 'f': filename = (char *) strdup(optarg); break;
		case 'm': hostname = (char *) strdup(optarg); break;
		case 'p': logfile_prefix = (char*) strdup(optarg); break;
		case 's': il_socket = (char*) strdup(optarg); break;
		case 'n': num_jobs = atoi(optarg); break;
		case 'N': nofile = 1; break;
		case 'P': skip = optarg ? atoi(optarg) : 0; break;
		case 'h': 
		default:
			usage(argv[0]); exit(0);
		}
	} 

	if(destname) {
		if(!strncasecmp(destname, "pr", 2)) 
			dest=DEST_PROXY;
		else if(!strncasecmp(destname, "lo", 2) || !strncasecmp(destname, "ll", 2))
			dest=DEST_LL;
		else if(!strncasecmp(destname, "in", 2) || !strncasecmp(destname, "il", 2))
			dest=DEST_IL;
		else if(!strncasecmp(destname, "bk", 2) || !strncasecmp(destname, "se", 2))
			dest=DEST_BKSERVER;
		else {
			fprintf(stderr,"%s: wrong destination\n",argv[0]);
			usage(argv[0]);
			exit(1);
		}
	} 

	if((dest == DEST_IL) && (il_socket == NULL)) 
		il_socket = DEFAULT_SOCKET;
	if((dest == DEST_PROXY) && il_socket) {
		char store_sock[256], serve_sock[256];
		sprintf(store_sock, "%s%s", il_socket, "store.sock");
		sprintf(serve_sock, "%s%s", il_socket, "serve.sock");
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_LBPROXY_STORE_SOCK, store_sock);
		edg_wll_SetParam(ctx, EDG_WLL_PARAM_LBPROXY_SERVE_SOCK, serve_sock);
	}

	if (num_jobs <= 0) {
		fprintf(stderr,"%s: wrong number of jobs\n",argv[0]);
		usage(argv[0]);
		exit(1);
	}

	if (!filename && destname) {
		fprintf(stderr,"%s: -f required\n",argv[0]);
		usage(argv[0]);
		exit(1);
	}

	if (glite_wll_perftest_init(hostname, NULL, testname, filename, num_jobs) < 0) {
		fprintf(stderr,"%s: glite_wll_perftest_init failed\n",argv[0]);
		exit(1);
	}

	if(dest) {
		char *jobid;
		int len;
		while ((len=glite_wll_perftest_produceEventString(&event, &jobid)) > 0) {
			switch(dest) {
			case DEST_PROXY:
				ctx->p_tmp_timeout = ctx->p_sync_timeout;
				if (edg_wll_DoLogEventProxy(ctx,event)) {
					char    *et,*ed;
					edg_wll_Error(ctx,&et,&ed);
					fprintf(stderr,"edg_wll_DoLogEventProxy(): %s (%s)\n",et,ed);
					exit(1);
				}
				break;
				
			case DEST_LL:
				ctx->p_tmp_timeout = ctx->p_log_timeout;
				if (edg_wll_DoLogEvent(ctx,event)) {
					char    *et,*ed;
					edg_wll_Error(ctx,&et,&ed);
					fprintf(stderr,"edg_wll_DoLogEvent(): %s (%s)\n",et,ed);
					exit(1);
				}
				break;

			case DEST_BKSERVER:
				ctx->p_tmp_timeout = ctx->p_log_timeout;
				edg_wlc_JobIdParse(jobid, &ctx->p_jobid);
				if (edg_wll_DoLogEventDirect(ctx, event)) {
					char    *et,*ed;
					edg_wll_Error(ctx,&et,&ed);
					fprintf(stderr,"edg_wll_DoLogEventDirect(): %s (%s)\n",et,ed);
					exit(1);
				}
				break;

			case DEST_IL:
				ctx->p_tmp_timeout = ctx->p_log_timeout;
				if (edg_wll_DoLogEventIl(ctx, event, jobid, len, skip)) {
					char    *et,*ed;
					edg_wll_Error(ctx,&et,&ed);
					fprintf(stderr,"edg_wll_DoLogEventIl(): %s (%s)\n",et,ed);
					exit(1);
				}
				break;

			default:
				break;
			}
			free(event);
		}
	} else {
		/* no destination - only print jobid's that would be used */
		char *jobid;

		while(jobid = glite_wll_perftest_produceJobId()) {
			fprintf(stdout, "%s\n", jobid);
		}
	}
	edg_wll_FreeContext(ctx);
	return 0;

}
