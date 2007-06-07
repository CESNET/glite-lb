#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "glite/wmsutils/jobid/cjobid.h"
#include "producer.h"
#include "glite/lb/events.h"

#define	MAXMSGSIZE	10240

extern char *optarg;
extern int opterr,optind;

extern int edg_wll_DoLogEvent(edg_wll_Context context, edg_wll_LogLine logline);
extern int edg_wll_DoLogEventProxy(edg_wll_Context context, edg_wll_LogLine logline);

static const char *me;

static void usage()
{
	fprintf(stderr,"usage: %s -m bkserver [-x] [-N numjobs] [-n subjobs (each)] -f file_name \n", me);
}

int main(int argc, char *argv[])
{
	char 	*job = NULL,*server = NULL,*seq = NULL,*filename = NULL;
	char 	buf[MAXMSGSIZE];
	int 	lbproxy = 0, num_subjobs = 0;
	int 	done = 0, njobs = 1,i;
	edg_wll_Context	ctx;
	edg_wlc_JobId	jobid,*subjobs;
	FILE	*f;

	edg_wll_InitContext(&ctx);

	opterr = 0;

	me = strdup(argv[0]);

	do {
		switch (getopt(argc,argv,"m:xN:n:f:")) {
			case 'm': server = strdup(optarg); break;
			case 'x': lbproxy = 1; break;
			case 'N': njobs = atoi(optarg); break;
			case 'n': num_subjobs = atoi(optarg); break;
			case 'f': filename = (char *) strdup(optarg); break;
			case '?': usage(); exit(EINVAL);
			case -1: done = 1; break;
		}
	} while (!done);

	if (!server) {
		fprintf(stderr,"%s: -m required\n",me);
		usage();
		exit(1);
	}

	if ((njobs <= 0) || (num_subjobs)) {
		fprintf(stderr,"%s: wrong number of jobs\n",me);
		usage();
		exit(1);
	}

	if (!filename) {
		fprintf(stderr,"%s: -f required\n",me);
		usage();
		exit(1);
	}

	if ( (f = fopen(filename,"r")) == NULL) {
		perror(filename);
		exit(1);
	}
	
/* MAIN LOOP */
for (i = 1; i<njobs; i++) {
	/* create jobid */
	if (!job) {
		char *p = strchr(server,':');
		if (p) *p=0;
		edg_wlc_JobIdCreate(server,p?atoi(p+1):0,&jobid);
		job = edg_wlc_JobIdUnparse(jobid);
		fprintf(stdout,"new jobid: %s\n",job);
	}
	else if ((errno = edg_wlc_JobIdParse(job,&jobid))) {
		perror(job);
		exit(1);
	}

	/* register */
	edg_wll_SetParam(ctx,EDG_WLL_PARAM_SOURCE,EDG_WLL_SOURCE_USER_INTERFACE);
	// edg_wll_SetParam(ctx,EDG_WLL_PARAM_SOURCE,edg_wll_StringToSource(src));

	if (lbproxy) {
		if (edg_wll_RegisterJobProxy(ctx,jobid,
			num_subjobs?EDG_WLL_REGJOB_COLLECTION:EDG_WLL_REGJOB_SIMPLE,
			"JDL: blabla", "NNNSSSS",
			num_subjobs,NULL,&subjobs))
		{
			char 	*et,*ed;
			edg_wll_Error(ctx,&et,&ed);
			fprintf(stderr,"edg_wll_RegisterJobProxy(%s): %s (%s)\n",job,et,ed);
			exit(1);
		}
	} else {
		if (edg_wll_RegisterJobSync(ctx,jobid,
			num_subjobs?EDG_WLL_REGJOB_COLLECTION:EDG_WLL_REGJOB_SIMPLE,
			"JDL: blabla", "NNNSSSS",
			num_subjobs,NULL,&subjobs))
		{
			char 	*et,*ed;
			edg_wll_Error(ctx,&et,&ed);
			fprintf(stderr,"edg_wll_RegisterJobSync(%s): %s (%s)\n",job,et,ed);
			exit(1);
		}
	}

	/* log events */
	while (!feof(f)) {
		edg_wll_LogLine logline;

		if (!fgets(buf,sizeof(buf),f)) break;
		if (strcmp(buf,"\n")) {
			// fprintf(stdout,"%d: %s\n",i,buf);
			asprintf(&logline,"DG.JOBID=\"%s\" %s",job,buf);
			if (lbproxy) {
				if (edg_wll_DoLogEventProxy(ctx,logline)) {
					char    *et,*ed;
					edg_wll_Error(ctx,&et,&ed);
					fprintf(stderr,"edg_wll_DoLogEventProxy(): %s (%s)\n",et,ed);
					exit(1);
				}
			} else {
				if (edg_wll_DoLogEvent(ctx,logline)) {
					char    *et,*ed;
					edg_wll_Error(ctx,&et,&ed);
					fprintf(stderr,"edg_wll_DoLogEvent(): %s (%s)\n",et,ed);
					exit(1);
				}
			}
			if (logline) free(logline);
		}
	}
	rewind(f);
	if (job) free(job); job = NULL;

	/* seq. code */
	seq = edg_wll_GetSequenceCode(ctx);
	fprintf(stdout,"\n%s=\"%s\"\n",num_subjobs?"EDG_WL_COLLECTION_JOBID":"EDG_JOBID",job);
	fprintf(stdout,"EDG_WL_SEQUENCE=\"%s\"\n",seq);
	free(seq);

	if (num_subjobs) for (i=0; subjobs[i]; i++) {
		char	*job_s = edg_wlc_JobIdUnparse(subjobs[i]);
		fprintf(stdout,"EDG_WL_SUB_JOBID[%d]=\"%s\"\n",i,job_s);
		free(job_s);
	}

} /* MAIN LOOP */

	fclose(f);
	edg_wll_FreeContext(ctx);

	return 0;
}
