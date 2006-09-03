#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sysexits.h>
#include <assert.h>

#include "glite/wmsutils/jobid/strmd5.h"
#include "glite/lb/consumer.h"
#include "glite/lb/context-int.h"
#include "glite/lb/lbs_db.h"
#include "glite/lb/jobstat.h"

static struct option opts[] = {
	{ "mysql",1,NULL,'m' },
	{ "verbose",0,NULL,'v' },
	{ NULL, 0, NULL, 0 }
};

static void usage();
static void do_exit(edg_wll_Context,int);
static const char *me;

int main(int argc,char **argv)
{
	int	opt;
	char	*dbstring = getenv("LBDB");
	int	verbose = 0, rows = 0, fields = 0, njobs = 0, i;
	edg_wll_Context	ctx;
	char	*stmt = NULL, *status = NULL;
	char	*str[2];
	edg_wll_Stmt sh;
	int	jobs[EDG_WLL_NUMBER_OF_STATCODES];

	me = strdup(argv[0]);

	while ((opt = getopt_long(argc,argv,"m:v",opts,NULL)) != EOF) switch (opt) {
		case 'm': dbstring = optarg; break;
		case 'v': verbose++; break;
		case '?': usage(); exit(EX_USAGE);
	}

	edg_wll_InitContext(&ctx);
	for (i = 1; i<EDG_WLL_NUMBER_OF_STATCODES; i++) jobs[i] = 0;
	if (edg_wll_Open(ctx,dbstring)) do_exit(ctx,EX_UNAVAILABLE);
	if (edg_wll_DBCheckVersion(ctx)) do_exit(ctx,EX_SOFTWARE);
	if (asprintf(&stmt,"SELECT status,count(status) FROM states GROUP BY status;") < 0) do_exit(ctx,EX_OSERR);
	if (verbose) fprintf(stderr,"mysql query: %s\n",stmt);
	if ((rows = edg_wll_ExecStmt(ctx,stmt,&sh)) < 0) do_exit(ctx,EX_SOFTWARE);
	if (verbose) fprintf(stderr,"number of states returned: %d\n",rows);
	if (rows > 0) fprintf(stdout,"Number of jobs in each state: \n");
	for (i = 0; i < rows; i++) {
		fields = edg_wll_FetchRow(sh, str);
		if (fields != 2) {
			edg_wll_FreeStmt(&sh);
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
	edg_wll_FreeStmt(&sh);
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
			"	-v,--verbose		be verbose\n",
			me);
}
