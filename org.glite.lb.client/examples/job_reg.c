#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "glite/wmsutils/jobid/cjobid.h"
#include "glite/lb/producer.h"
#include "glite/lb/events.h"

extern char *optarg;
extern int opterr,optind;

static void usage(char *me)
{
	fprintf(stderr,"usage: %s [-m bkserver] [-x] [-j dg_jobid] [-s source_id] [-n num_subjobs [-S][-C]] [-l jdl_file] [-e seed]\n", me);
}

int main(int argc, char *argv[])
{
	char *src = NULL,*job = NULL,*server = NULL,*seq,*jdl = NULL, *seed = NULL;
	int lbproxy = 0;
	int done = 0,num_subjobs = 0,reg_subjobs = 0,i, collection = 0, pbs=0;
	edg_wll_Context	ctx;
	edg_wlc_JobId	jobid,*subjobs;


	edg_wll_InitContext(&ctx);
	opterr = 0;

	do {
		switch (getopt(argc,argv,"xs:j:m:n:SCl:e:P")) {
			case 'x': lbproxy = 1; break;
			case 's': src = (char *) strdup(optarg); break;
			case 'j': job = (char *) strdup(optarg); break;
			case 'm': server = strdup(optarg); break;
			case 'n': num_subjobs = atoi(optarg); break;
			case 'S': if (num_subjobs>0) { reg_subjobs = 1; break; }
			case 'C': if (num_subjobs>0) { collection = 1; break; }
			case 'P': pbs = 1; break;
			case 'l': jdl = (char *) strdup(optarg); break;
			case 'e': seed = strdup(optarg); break;
			case '?': usage(argv[0]); exit(EINVAL);
			case -1: done = 1; break;
		}
	} while (!done);

	if (!job && !server) {
		fprintf(stderr,"%s: either -m server or -j jobid has to be specified\n",argv[0]);
		exit(1);
	}

	if (!src) {
		fprintf(stderr,"%s: -s required\n",argv[0]);
		exit(1);
	}

	if (!job) {
		char *p = strchr(server,':');
		if (p) *p=0;
		edg_wlc_JobIdCreate(server,p?atoi(p+1):0,&jobid);
		job = edg_wlc_JobIdUnparse(jobid);
		printf("new jobid: %s\n",job);
	}
	else if ((errno = edg_wlc_JobIdParse(job,&jobid))) {
		perror(job);
		exit(1);
	}

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

	edg_wll_SetParam(ctx,EDG_WLL_PARAM_SOURCE,edg_wll_StringToSource(src));
	if (lbproxy) {
		if (edg_wll_RegisterJobProxy(ctx,jobid,
			pbs ? EDG_WLL_REGJOB_PBS
			    : (num_subjobs ? 
				(collection?EDG_WLL_REGJOB_COLLECTION:EDG_WLL_REGJOB_DAG) 
				:EDG_WLL_REGJOB_SIMPLE
				),
			jdl ? jdl : "blabla", "NNNSSSS",
			num_subjobs,seed,&subjobs))
		{
			char 	*et,*ed;
			edg_wll_Error(ctx,&et,&ed);
			fprintf(stderr,"edg_wll_RegisterJobProxy(%s): %s (%s)\n",job,et,ed);
			exit(1);
		}
	} else {
		if (edg_wll_RegisterJobSync(ctx,jobid,
			pbs ? EDG_WLL_REGJOB_PBS
			    : (num_subjobs ? 
				(collection?EDG_WLL_REGJOB_COLLECTION:EDG_WLL_REGJOB_DAG) 
				:EDG_WLL_REGJOB_SIMPLE
				),
			jdl ? jdl : "blabla", "NNNSSSS",
			num_subjobs,seed,&subjobs))
		{
			char 	*et,*ed;
			edg_wll_Error(ctx,&et,&ed);
			fprintf(stderr,"edg_wll_RegisterJobSync(%s): %s (%s)\n",job,et,ed);
			exit(1);
		}
	}

	seq = edg_wll_GetSequenceCode(ctx);
	printf("\n%s=\"%s\"\n",num_subjobs?(collection?"EDG_WL_COLLECTION_JOBID":"EDG_WL_DAG_JOBID"):"EDG_JOBID",job);
	printf("EDG_WL_SEQUENCE=\"%s\"\n",seq);
	free(seq);
	free(job);

	if (num_subjobs) for (i=0; subjobs[i]; i++) {
		char	*job_s = edg_wlc_JobIdUnparse(subjobs[i]);
		printf("EDG_WL_SUB_JOBID[%d]=\"%s\"\n",i,job_s);
		free(job_s);
	}

	if (reg_subjobs) {
		char ** jdls = (char**) calloc(num_subjobs+1, sizeof(char*));

		for (i=0; subjobs[i]; i++) {
			asprintf(jdls+i, "JDL of subjob #%d\n", i+1);
		}

		if (lbproxy) {
			if (edg_wll_RegisterSubjobsProxy(ctx, jobid, (const char **) jdls, NULL, subjobs)) {
				char 	*et,*ed;
				edg_wll_Error(ctx,&et,&ed);
				fprintf(stderr,"edg_wll_RegisterSubjobsProxy: %s (%s)\n", et, ed);
				exit(1);
			}
		} else {
			if (edg_wll_RegisterSubjobs(ctx, jobid, (const char **) jdls, NULL, subjobs)) {
				char 	*et,*ed;
				edg_wll_Error(ctx,&et,&ed);
				fprintf(stderr,"edg_wll_RegisterSubjobs: %s (%s)\n", et, ed);
				exit(1);
			}
		}

		for (i=0; subjobs[i]; i++) free(jdls[i]);
	}

	edg_wll_FreeContext(ctx);

	return 0;
}
