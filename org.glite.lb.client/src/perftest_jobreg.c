#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>

#include "glite/wmsutils/jobid/cjobid.h"
#include "glite/lb/producer.h"
#include "glite/lb/events.h"

#define PROXY_SERVER "localhost:9000"

#define tv_sub(a,b) {\
        (a).tv_usec -= (b).tv_usec;\
        (a).tv_sec -= (b).tv_sec;\
        if ((a).tv_usec < 0) {\
                (a).tv_sec--;\
                (a).tv_usec += 1000000;\
        }\
}

#define dprintf(x)              { if (verbose) printf x; }


extern char *optarg;
extern int opterr,optind;

static void usage(char *me)
{
	fprintf(stderr,"usage: %s [-m bkserver] [-x] [-n num_subjobs [-S]] [-l jdl_file] [-N num_repeat]\n"
		"	-m <bkserver>		address:port of bkserver\n"
		"	-x  	           	use LBProxy\n"
		"	-n <num_subjobs>	number of subjobs of DAG\n"
		"	-S			register subjobs\n"
		"	-l <jdl_file>		file with JDL\n"
		"	-N <num_repeat>		repeat whole registration N-times\n"
		"	-v			verbose output\n",
		me);
}

int main(int argc, char *argv[])
{
	char 		*server = NULL,*jdl = NULL;
	int 		lbproxy = 0, N = 1, verbose = 0;
	int 		done = 0,num_subjobs = 0,reg_subjobs = 0, i, j;
	edg_wll_Context	ctx;
	edg_wlc_JobId	*jobids,*subjobs;
	struct timeval 	start, stop;


	edg_wll_InitContext(&ctx);
	opterr = 0;

	do {
		switch (getopt(argc,argv,"xm:n:Sl:N:v")) {
			case 'x': lbproxy = 1; break;
			case 'm': server = strdup(optarg); break;
			case 'n': num_subjobs = atoi(optarg); break;
			case 'S': if (num_subjobs>0) { reg_subjobs = 1; break; }
			case 'l': jdl = (char *) strdup(optarg); break;
			case 'N': N = atoi(optarg); break;
			case 'v': verbose = 1; break;
			case '?': usage(argv[0]); exit(EINVAL);
			case -1: done = 1; break;
		}
	} while (!done);

	if (!server && !lbproxy) {
		fprintf(stderr,"%s: either -m server or -x has to be specified\n",argv[0]);
		exit(1);
	}

	/* prepare set of N jobid before starting timer */
	jobids = (edg_wlc_JobId *) malloc(N * sizeof(edg_wlc_JobId));
	dprintf(("generating jobids..."));
	{
		char *name=server?server:strdup(PROXY_SERVER);
		char *p = strchr(name,':');
		int  port;

		if (p)  { *p=0; port = atoi(p+1); }
		else  port = 0;
		for (i=0; i<N; i++)
			edg_wlc_JobIdCreate(name,port,&(jobids[i]));
	}
	dprintf(("done.\n"));

	/* probably not useful now, but can be handy with register subjobs */
	if (jdl) {
		int	f = open(jdl,O_RDONLY,0);
		off_t	l,p,c;

		if (f<0) { perror(jdl); exit(1); }
		l = lseek(f,0,SEEK_END);
		lseek(f,0,SEEK_SET);

		jdl = malloc(l+1);

		for (p=0; p < l && (c = read(f,jdl+p,l-p)) > 0; p += c);
		if (c<0) {
			perror("read()");
			exit (1);
		}
		jdl[p] = 0;
	}

	edg_wll_SetParam(ctx,EDG_WLL_PARAM_SOURCE,edg_wll_StringToSource("Application"));

	/* start measurement */
	gettimeofday(&start, NULL);

	for (i=0; i<N; i++) {
		if (server) {
			if (edg_wll_RegisterJobSync(ctx,jobids[i],
				num_subjobs?EDG_WLL_REGJOB_DAG:EDG_WLL_REGJOB_SIMPLE,
				jdl ? jdl : "blabla", "NNNSSSS",
				num_subjobs,NULL,&subjobs))
			{
				char 	*et,*ed;
				edg_wll_Error(ctx,&et,&ed);
				fprintf(stderr,"edg_wll_RegisterJobSync(): %s (%s)\n",et,ed);
				exit(1);
			}
		}
		if (lbproxy) {
			if (edg_wll_RegisterJobProxyOnly(ctx,jobids[i],
				num_subjobs?EDG_WLL_REGJOB_DAG:EDG_WLL_REGJOB_SIMPLE,
				jdl ? jdl : "blabla", "NNNSSSS",
				num_subjobs,NULL,&subjobs))
			{
				char 	*et,*ed;
				edg_wll_Error(ctx,&et,&ed);
				fprintf(stderr,"edg_wll_RegisterJobProxyOnly(): %s (%s)\n",et,ed);
				exit(1);
			}
		} 

		if (reg_subjobs) {
			char ** jdls = (char**) malloc(num_subjobs*sizeof(char*));

			for (j=0; subjobs[j]; j++) {
				asprintf(jdls+j, "JDL of subjob #%d\n", j+1);
			}

			if (server) {
				if (edg_wll_RegisterSubjobs(ctx, jobids[i], (const char **) jdls, NULL, subjobs)) {
					char 	*et,*ed;
					edg_wll_Error(ctx,&et,&ed);
					fprintf(stderr,"edg_wll_RegisterSubjobs: %s (%s)\n", et, ed);
					exit(1);
				}
			}
			if (lbproxy) {
				if (edg_wll_RegisterSubjobsProxy(ctx, jobids[i], (const char **) jdls, NULL, subjobs)) {
					char 	*et,*ed;
					edg_wll_Error(ctx,&et,&ed);
					fprintf(stderr,"edg_wll_RegisterSubjobsProxy: %s (%s)\n", et, ed);
					exit(1);
				}
			} 

			for (j=0; subjobs[j]; j++) free(jdls[j]);
		}
	}

	/* stop measurement */
	gettimeofday(&stop, NULL);

	{
		float sum;

		tv_sub(stop,start);
		sum=stop.tv_sec + (float) stop.tv_usec/1000000;
		dprintf(("Test duration (seconds) "));
		printf("%6.6f\n", sum); 
	}

	for (i=0; i<N; i++) 
		edg_wlc_JobIdFree(jobids[i]);
	free(jobids);

	edg_wll_FreeContext(ctx);

	return 0;
}
