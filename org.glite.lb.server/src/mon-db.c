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
#include <unistd.h>
#include <getopt.h>
#include <sysexits.h>
#include <assert.h>

#include "glite/jobid/strmd5.h"
#include "glite/lb/context-int.h"
#include "glite/lb/jobstat.h"
#include "db_supp.h"
#include "openserver.h"

#ifdef LB_PERF
#include "glite/lb/lb_perftest.h"
#include "srv_perf.h"

enum lb_srv_perf_sink sink_mode;
#endif

/* XXX: referenced global variables, unsed in mon */
char	*server_key,*server_cert;
int	enable_lcas;
int	proxy_purge;
struct _edg_wll_authz_policy authz_policy;
char    *policy_file;

static struct option opts[] = {
	{ "mysql",1,NULL,'m' },
	{ "debug",0,NULL,'d' },
	{ "verbose",0,NULL,'v' },
	{ NULL, 0, NULL, 0 }
};

int debug  = 0;

static void usage();
static void do_exit(edg_wll_Context,int);
static const char *me;

int main(int argc,char **argv)
{
	int	opt, caps;
	char	*dbstring = getenv("LBDB");
	int	verbose = 0, rows = 0, fields = 0, njobs = 0, i;
	edg_wll_Context	ctx;
	char	*stmt = NULL, *status = NULL;
	char	*str[2];
	glite_lbu_Statement sh;
	int	jobs[EDG_WLL_NUMBER_OF_STATCODES];

	me = strdup(argv[0]);

	while ((opt = getopt_long(argc,argv,"m:dv",opts,NULL)) != EOF) switch (opt) {
		case 'm': dbstring = optarg; break;
		case 'd': debug++; verbose++; break;
		case 'v': verbose++; break;
		case '?': usage(); exit(EX_USAGE);
	}

	edg_wll_InitContext(&ctx);
	for (i = 1; i<EDG_WLL_NUMBER_OF_STATCODES; i++) jobs[i] = 0;
	if (edg_wll_Open(ctx,dbstring)) do_exit(ctx,EX_UNAVAILABLE);
	if ((caps = glite_lbu_DBQueryCaps(ctx->dbctx)) < 0 || !(caps & GLITE_LBU_DB_CAP_INDEX)) do_exit(ctx,EX_SOFTWARE);
	if (asprintf(&stmt,"SELECT status,count(status) FROM states GROUP BY status;") < 0) do_exit(ctx,EX_OSERR);
	if (verbose) fprintf(stderr,"mysql query: %s\n",stmt);
	if ((rows = edg_wll_ExecSQL(ctx,stmt,&sh)) < 0) do_exit(ctx,EX_SOFTWARE);
	if (verbose) fprintf(stderr,"number of states returned: %d\n",rows);
	if (rows > 0) fprintf(stdout,"Number of jobs in each state: \n");
	for (i = 0; i < rows; i++) {
		fields = edg_wll_FetchRow(ctx, sh, sizeof(str)/sizeof(str[0]),NULL,str);
		if (fields != 2) {
			glite_lbu_FreeStmt(&sh);
			do_exit(ctx,EX_SOFTWARE);
		}
		jobs[atoi(str[0])] = atoi(str[1]);
		if (str[0]) free(str[0]);
		if (str[1]) free(str[1]);
	}
	for (i = 1; i<EDG_WLL_NUMBER_OF_STATCODES; i++) {
		status = edg_wll_StatToString((edg_wll_JobStatCode) i);
		fprintf(stdout,"%d \t %s \t %d\n",i,status,jobs[i]);
		njobs += jobs[i];
		if (status) free(status);
	}
	fprintf(stdout,"Total number of jobs: %d\n",njobs);

	if (stmt) free(stmt);
	glite_lbu_FreeStmt(&sh);
	edg_wll_FreeContext(ctx);

	return 0;
}

static void do_exit(edg_wll_Context ctx,int code)
{
	char	*et,*ed;

	edg_wll_Error(ctx,&et,&ed);
	fprintf(stderr,"%s: %s (%s)\n",me,et,ed);
	exit(code);
}

static void usage()
{
	fprintf(stderr,"usage: %s <options>\n"
			"	-m,--mysql <dbstring>	use non-default database connection\n"
			"	-d,--debug		print debug info (if any)\n"
			"	-v,--verbose		be verbose\n",
			me);
}
