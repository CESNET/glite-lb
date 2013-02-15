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
#include <unistd.h>
#include <assert.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>


#include "glite/jobid/cjobid.h"
#ifdef BUILDING_LB_CLIENT
#include "producer.h"
#else
#include "glite/lb/producer.h"
#endif
#include "glite/lb/events.h"

static void usage(char *me) 
{
	fprintf(stderr,"usage %s: \n"
			"	-j,--jobid jobid\n"
			"	-i,--input or -o,--output	(mutually exclusive)\n"
			"	-f,--from URI\n"
			"	-t,--to URI\n"
			"	[ -s,--source lb-source ]	(default LRMS)\n"
			"	[ -c,--sequence lb-seqcode ]\n"
			"	[ -n,--subjobs n]\n"
			,me);
}

#define check_log(fun,_jobid,arg) \
	if (fun arg) {	\
		char	*et,*ed;	\
		edg_wll_Error(ctx,&et,&ed);	\
		fprintf(stderr,#fun "(%s): %s (%s)\n",_jobid,et,ed);	\
		exit(1);	\
	}


int main(int argc,char **argv)
{
	struct option opts[] = {
		{ "jobid",1,NULL,'j' },
		{ "input",0,NULL,'i' },
		{ "output",0,NULL,'o' },
		{ "from",1,NULL,'f' },
		{ "to",1,NULL,'t' },
		{ "source",1,NULL,'s' },
		{ "sequence",1,NULL,'c' },
		{ "subjobs",1,NULL,'n' },
		{ NULL, 0, NULL, 0 },
	};

	char	*jobid_s = NULL, *ftjobid_s = NULL, *from = NULL, *to = NULL, *source_s = "LRMS",
		*sequence = NULL, *srv, *uniq, type_c = 'x';

	unsigned int	port;
	int		o, i;
	unsigned int 	nsubjobs = 0;

	glite_jobid_t	jobid, ftjobid, *subjobs;
	edg_wll_Source	source;

	enum edg_wll_SandboxSandbox_type type =
		EDG_WLL_SANDBOX_SANDBOX_TYPE_UNDEFINED;

	edg_wll_Context	ctx;

	while ((o = getopt_long(argc,argv,"j:iof:t:s:c:n:",opts,NULL)) != EOF) switch(o) {
		case 'j':	jobid_s = optarg; break;
		case 'i':	if (type != EDG_WLL_SANDBOX_SANDBOX_TYPE_UNDEFINED) { usage(argv[0]); exit(1); }
				type = EDG_WLL_SANDBOX_INPUT;
				break;
		case 'o':	if (type != EDG_WLL_SANDBOX_SANDBOX_TYPE_UNDEFINED) { usage(argv[0]); exit(1); }
				type = EDG_WLL_SANDBOX_OUTPUT;
				break;
		case 'f':	from = optarg; break;
		case 't':	to = optarg; break;
		case 's':	source_s = optarg; break;
		case 'c':	sequence = optarg; break;
		case 'n':	nsubjobs = atoi(optarg); break;
		default:	usage(argv[0]); exit(1);
	}

	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		exit(1);
	}

	if (!jobid_s || type == EDG_WLL_SANDBOX_SANDBOX_TYPE_UNDEFINED
		|| (nsubjobs==0 && (!from || !to)) 
		|| (nsubjobs>0 && ((!from && to) || (from && !to))))
	{
		usage(argv[0]); 
		exit(1);
	}

	type_c = type == EDG_WLL_SANDBOX_INPUT ? 'I' : 'O';

	if (glite_jobid_parse(jobid_s,&jobid)) {
		fprintf(stderr,"%s: can't parse\n",jobid_s);
		exit(1);
	}

	if ((source = edg_wll_StringToSource(source_s)) == EDG_WLL_SOURCE_NONE) {
		fprintf(stderr,"%s: invalid source\n",source_s);
		exit(1);
}
;
	edg_wll_SetParam(ctx, EDG_WLL_PARAM_SOURCE, source);

	glite_jobid_getServerParts(jobid,&srv,&port);
	glite_jobid_create(srv,port,&ftjobid);
	uniq = glite_jobid_getUnique(ftjobid);
	glite_jobid_free(ftjobid);

	asprintf(&ftjobid_s,"https://%s:%d/FT:%s",srv,port,uniq);
	assert(glite_jobid_parse(ftjobid_s,&ftjobid) == 0);

	edg_wll_SetLoggingJob(ctx,ftjobid,NULL,EDG_WLL_SEQ_NORMAL);

	check_log(edg_wll_RegisterJob,ftjobid_s,(
		ctx,
		ftjobid, (nsubjobs==0) ? EDG_WLL_REGJOB_FILE_TRANSFER : EDG_WLL_REGJOB_FILE_TRANSFER_COLLECTION,
		"n/a","n/a",
		nsubjobs,NULL,&subjobs));

	if (from && to) {	
		check_log(edg_wll_LogFileTransferRegister,ftjobid_s,(ctx,from,to));
	}
	else {
		check_log(edg_wll_LogFileTransferRegister,ftjobid_s,(ctx,"n/a","n/a"));
	}

	check_log(edg_wll_LogSandbox,ftjobid_s,(
				ctx,
				edg_wll_SandboxSandbox_typeToString(type),
				NULL,
				jobid_s));

	printf("GLITE_LB_%cSB_JOBID=\"%s\"\nGLITE_LB_%cSB_SEQUENCE=\"%s\"\n",
			type_c,ftjobid_s,
			type_c,edg_wll_GetSequenceCode(ctx));

	if (edg_wll_SetLoggingJob(ctx,jobid,sequence,EDG_WLL_SEQ_NORMAL)) {
		char	*et,*ed;
		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"edg_wll_SetLoggingJob(%s,%s): %s (%s)\n",
				jobid_s,sequence,et,ed);
		exit(1);
	}

	check_log(edg_wll_LogSandbox,jobid_s,(
				ctx,
				edg_wll_SandboxSandbox_typeToString(type),
				ftjobid_s,
				NULL));

	printf("GLITE_WMS_SEQUENCE_CODE=\"%s\"\n",edg_wll_GetSequenceCode(ctx));

	if (nsubjobs){
		for (i = 0; i < nsubjobs; i++){
			char    *ftsubjobid_s = edg_wlc_JobIdUnparse(subjobs[i]);
                	printf("EDG_WL_SUB_JOBID[%d]=\"%s\"\n", i, ftsubjobid_s);
	                free(ftsubjobid_s);
		}

		/*edg_wll_SetLoggingJob(ctx,ftjobid,NULL,EDG_WLL_SEQ_NORMAL);
		check_log(edg_wll_RegisterFTSubjobs, ftjobid_s, (
			ctx,
			ftjobid,
			"NS",
			nsubjobs, subjobs
		));*/

		for (i=0; subjobs[i]; i++) {
			char    *ftsubjobid_s = edg_wlc_JobIdUnparse(subjobs[i]);
			edg_wll_SetLoggingJob(ctx, subjobs[i], NULL, EDG_WLL_SEQ_NORMAL);
			check_log(edg_wll_LogSandbox, ftsubjobid_s, (
				ctx,
				edg_wll_SandboxSandbox_typeToString(type),
				NULL,
				jobid_s
			));
                        free(ftsubjobid_s);
		}
	}

	return 0;
}
