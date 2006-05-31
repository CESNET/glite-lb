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
	{ NULL, 0, NULL, 0}
};


void
usage(char *program_name)
{
	fprintf(stderr, "Usage: %s [-d destname] [-t testname] -f filename -n numjobs \n"
		"-h, --help               display this help and exit\n"
                "-d, --dst <destname>     destination component\n"
		"-m, --machine <hostname> destination host\n"
		"-t, --test <testname>    name of the test\n"
		"-f, --file <filename>    name of the file with prototyped job events\n"
		"-n, --num <numjobs>      number of jobs to generate\n",
	program_name);
}


int edg_wll_DoLogEventIl(
	edg_wll_Context context,
	edg_wll_LogLine logline)
{
	static int filepos = 0;
	int ret = 0;
	struct timeval timeout;
	
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	ret = edg_wll_log_event_send(context, DEFAULT_SOCKET, filepos, logline, strlen(logline), 1, &timeout);
	filepos += strlen(logline);

	return(ret);
}


int
main(int argc, char *argv[])
{

	char 	*destname= NULL,*hostname=NULL,*testname = NULL,*filename = NULL;
	char	*event;
	int 	num_jobs = 1;
	int 	opt;
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

	while ((opt = getopt_long(argc,argv,"hd:t:f:n:x",
		long_options, (int *) 0)) != EOF) {

		switch (opt) {
		case 'd': destname = (char *) strdup(optarg); break;
		case 't': testname = (char *) strdup(optarg); break;
		case 'f': filename = (char *) strdup(optarg); break;
		case 'm': hostname = (char *) strdup(optarg); break;
		case 'n': num_jobs = atoi(optarg); break;
		case 'h': 
		default:
			usage(argv[0]); exit(0);
		}
	} 

	if(destname) {
		if(!strncasecmp(destname, "pr", 2)) 
			dest=DEST_PROXY;
		else if(!strncasecmp(destname, "lo", 2))
			dest=DEST_LL;
		else if(!strncasecmp(destname, "in", 2) || !strncasecmp(destname, "il", 2))
			dest=DEST_IL;
		else if(!strncasecmp(destname, "bk", 2) || !strncasecmp(destname, "se", 2))
			dest=DEST_BKSERVER;
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
	}

	if(dest) {
		while (glite_wll_perftest_produceEventString(&event)) {
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
				if (edg_wll_DoLogEventDirect(ctx, event)) {
					char    *et,*ed;
					edg_wll_Error(ctx,&et,&ed);
					fprintf(stderr,"edg_wll_DoLogEventDirect(): %s (%s)\n",et,ed);
					exit(1);
				}

			case DEST_IL:
				ctx->p_tmp_timeout = ctx->p_log_timeout;
				if (edg_wll_DoLogEventIl(ctx, event)) {
					char    *et,*ed;
					edg_wll_Error(ctx,&et,&ed);
					fprintf(stderr,"edg_wll_DoLogEventIl(): %s (%s)\n",et,ed);
					exit(1);
				}

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
			free(jobid);
		}
	}
	edg_wll_FreeContext(ctx);
	return 0;

}
