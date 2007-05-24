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
#include "index.h"
#include "lbs_db.h"
#include "jobstat.h"

#ifdef LB_PERF
#include "glite/lb/lb_perftest.h"
#include "glite/lb/srv_perf.h"

enum lb_srv_perf_sink sink_mode;
#endif


static struct option opts[] = {
	{ "mysql",1,NULL,'m' },
	{ "remove",0,NULL,'R' },
	{ "really",0,NULL,'r' },
	{ "dump",0,NULL,'d' },
	{ "verbose",0,NULL,'v' },
	{ NULL, 0, NULL, 0 }
};

static void usage(const char *);
static void do_exit(edg_wll_Context,int);
static char *col_list(const edg_wll_QueryRec *);
static char *db_col_type(const edg_wll_QueryRec *);

/* XXX: don't bother with malloc() of arrays that are tiny in real life */
#define CI_MAX	200

int debug;

int main(int argc,char **argv)
{
	int	opt;
	char	*dbstring = getenv("LBDB");
	char	*fname = "-";
	int	dump = 0, really = 0, verbose = 0, remove_all = 0;
	edg_wll_Context	ctx;
	edg_wll_QueryRec	**old_indices,**new_indices;
	edg_wll_QueryRec	*new_columns[CI_MAX],*old_columns[CI_MAX];
	int	add_columns[CI_MAX],drop_columns[CI_MAX];
	int	add_indices[CI_MAX],drop_indices[CI_MAX];
	edg_wll_IColumnRec *added_icols;
	int	nadd_icols;
	char	**index_names;
	int	i,j,k;
	int	nnew,nold,nadd,ndrop;
	char	*stmt;

	for (i=0; i<CI_MAX; i++) add_indices[i] = drop_indices[i] = -1;
	memset(new_columns,0,sizeof new_columns);
	memset(old_columns,0,sizeof old_columns);
	memset(add_columns,0,sizeof add_columns);
	memset(drop_columns,0,sizeof drop_columns);

	while ((opt = getopt_long(argc,argv,"m:rRdv",opts,NULL)) != EOF) switch (opt) {
		case 'm': dbstring = optarg; break;
		case 'r': really = 1; break;
		case 'R': remove_all = 1; break;
		case 'd': dump = 1; break;
		case 'v': verbose++; break;
		case '?': usage(argv[0]); exit(EX_USAGE);
	}

	if (really && dump) { usage(argv[0]); exit(EX_USAGE); }
	if (really && remove_all) { usage(argv[0]); exit(EX_USAGE); }
	if (optind < argc) {
		if (dump || remove_all) { usage(argv[0]); exit(EX_USAGE); }
		else fname = argv[optind];
	}

	edg_wll_InitContext(&ctx);
	if (edg_wll_Open(ctx,dbstring)) do_exit(ctx,EX_UNAVAILABLE);
	if (edg_wll_DBCheckVersion(ctx,dbstring)) do_exit(ctx,EX_SOFTWARE);
	if (edg_wll_QueryJobIndices(ctx,&old_indices,&index_names)) do_exit(ctx,EX_SOFTWARE);

	if (dump) {
		if (edg_wll_DumpIndexConfig(ctx,fname,old_indices)) do_exit(ctx,EX_SOFTWARE);
		else if ( !remove_all ) return 0;
	}

	if (remove_all) {
		if (verbose) printf("Dropping all indices");
		for (i=0; index_names && index_names[i]; i++) {
			asprintf(&stmt,"alter table states drop index `%s`",index_names[i]);
			if (verbose) putchar('.');
			if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) do_exit(ctx,EX_SOFTWARE);
			free(stmt);
		}
		if (verbose) puts(" done");
		if (verbose) printf("Dropping index columns");
		for (i=0; old_indices && old_indices[i]; i++) {
			char *cname = edg_wll_QueryRecToColumn(old_indices[i]);
			asprintf(&stmt,"alter table states drop column `%s`",cname);
			if (verbose) putchar('.');
			if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) do_exit(ctx,EX_SOFTWARE);
			free(stmt);
			free(cname);
		}
		if (verbose) puts(" done");
		return 0;
	}

	if (edg_wll_ParseIndexConfig(ctx,fname,&new_indices))
		do_exit(ctx,edg_wll_Error(ctx,NULL,NULL) == EINVAL ? EX_DATAERR : EX_IOERR);

	/* FIXME: check duplicate columns in indices */

/* indices to add & drop */
	nadd = ndrop = 0;
	if (old_indices && new_indices) {
		for (i=0; old_indices[i]; i++) {
			for (j=0; new_indices[j]; j++) {
				for (k=0; old_indices[i][k].attr &&
					!edg_wll_CmpColumn(&old_indices[i][k],&new_indices[j][k]);
					k++);
				if (!old_indices[i][k].attr && !new_indices[j][k].attr) break;
			}
			if (!new_indices[j]) drop_indices[ndrop++] = i;
		}
					
		for (i=0; new_indices[i]; i++) {
			for (j=0; old_indices[j]; j++) {
				for (k=0; new_indices[i][k].attr &&
					!edg_wll_CmpColumn(&new_indices[i][k],&old_indices[j][k]);
					k++);
				if (!new_indices[i][k].attr && !old_indices[j][k].attr) break;
			}
			if (!old_indices[j]) add_indices[nadd++] = i;
		}
	}
	else if (new_indices) for (i=0; new_indices[i]; i++) add_indices[i] = i;
	else if (old_indices) for (i=0; old_indices[i]; i++) drop_indices[i] = i;

/* old and new column sets */
	nold = nnew = 0;
	if (old_indices) for (i=0; old_indices[i]; i++) 
		for (j=0; old_indices[i][j].attr; j++) {
			for (k=0; k<nold && edg_wll_CmpColumn(old_columns[k],&old_indices[i][j]); k++);
			if (k == nold) {
				assert(nold < CI_MAX);
				old_columns[nold++] = &old_indices[i][j];
			}
		}

	if (new_indices) for (i=0; new_indices[i]; i++) 
		for (j=0; new_indices[i][j].attr; j++) {
			for (k=0; k<nnew && edg_wll_CmpColumn(new_columns[k],&new_indices[i][j]); k++);
			if (k == nnew) {
				assert(nnew < CI_MAX);
				new_columns[nnew++] = &new_indices[i][j];
			}
		}

/* go! */
	if (!really) puts("\n** Dry run, no actual actions performed **\n");

	if (verbose) puts("Dropping indices ...");
	for (i=0; drop_indices[i] >= 0; i++) {
		if (verbose) { 
			char	*n = col_list(old_indices[drop_indices[i]]);
			printf("\t%s(%s) ... ",index_names[drop_indices[i]],n); fflush(stdout);
			free(n);
		}
		if (really) {
			asprintf(&stmt,"alter table states drop index `%s`",index_names[drop_indices[i]]);
			if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) do_exit(ctx,EX_SOFTWARE);
			free(stmt);
		}
		if (verbose) puts(really ? "done" : "");
	}

	if (verbose) puts("Dropping columns ...");
	for (i=0; i<nold; i++) {
		for (j=0; j<nnew && edg_wll_CmpColumn(old_columns[i],new_columns[j]); j++);
		if (j == nnew) {
			char	*cname = edg_wll_QueryRecToColumn(old_columns[i]);
			if (verbose) printf("\t%s\n",cname); 
			if (really) {
				asprintf(&stmt,"alter table states drop column `%s`",cname);
				if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) do_exit(ctx,EX_SOFTWARE);
				free(stmt);
			}
			free(cname);
		}
	}

	added_icols = calloc(nnew+1, sizeof(edg_wll_IColumnRec));
	nadd_icols = 0;
	if (verbose) puts("Adding columns ...");
	for (i=0; i<nnew; i++) {
		for (j=0; j<nold && edg_wll_CmpColumn(new_columns[i],old_columns[j]); j++);
		if (j == nold) {
			char	*cname = edg_wll_QueryRecToColumn(new_columns[i]);
			if (verbose) printf("\t%s\n",cname); 
			if (really) {
				char	*ctype = db_col_type(new_columns[i]);
				asprintf(&stmt,"alter table states add `%s` %s",cname,ctype);
				if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) do_exit(ctx,EX_SOFTWARE);
				free(stmt);
			}
			memcpy(&added_icols[nadd_icols].qrec, new_columns[i], sizeof(edg_wll_QueryRec));
			added_icols[nadd_icols].colname = strdup(cname);
			if (new_columns[i]->attr == EDG_WLL_QUERY_ATTR_USERTAG)
				added_icols[nadd_icols].qrec.attr_id.tag =
					strdup(new_columns[i]->attr_id.tag);
			nadd_icols++;
			free(cname);
		}
	}

	if (nadd_icols) {
		added_icols[nadd_icols].qrec.attr = EDG_WLL_QUERY_ATTR_UNDEF;
		added_icols[nadd_icols].colname = NULL;
		if (verbose) puts("Refreshing data ...");
		if (really && edg_wll_RefreshIColumns(ctx, (void*) added_icols)) do_exit(ctx,EX_SOFTWARE);
	}
	else if (verbose) puts("No data refresh required");

	for (nadd_icols--; nadd_icols >= 0; nadd_icols--)
		edg_wll_FreeIColumnRec(&added_icols[nadd_icols]);
	free(added_icols);

	if (verbose) puts("Creating indices ...");
	for (i=0; add_indices[i] >= 0; i++) {
		char	*l = col_list(new_indices[add_indices[i]]);
		char	*n = str2md5base64(l);
		if (verbose) { printf("\t%s(%s) ... ",n,l); fflush(stdout); }
		if (really) {
			asprintf(&stmt,"create index `%s` on states(%s)",n,l);
			free(n); free(l);
			if (edg_wll_ExecStmt(ctx,stmt,NULL) < 0) do_exit(ctx,EX_SOFTWARE);
			free(stmt);
		}
		if (verbose) puts(really ? "done" : "");
	}

	return 0;
}

static char *col_list(const edg_wll_QueryRec *ind)
{
	char	*ret,*aux,size[50] = "";
	int	j;

	aux = edg_wll_QueryRecToColumn(ind);
	if (ind->value.i) sprintf(size,"(%d)",ind->value.i);
	asprintf(&ret,"`%s`%s",aux,size);
	free(aux);

	for (j=1; ind[j].attr; j++) {
		char	*n = edg_wll_QueryRecToColumn(ind+j),size[50] = "";
		if (ind[j].value.i) sprintf(size,"(%d)",ind[j].value.i);
		aux = ret;
		asprintf(&ret,"%s,`%s`%s",aux,n,size);
		free(n); free(aux);
	}
	return ret;
}

static char *db_col_type(const edg_wll_QueryRec *r)
{
	switch (r->attr) {
		case EDG_WLL_QUERY_ATTR_OWNER:
		case EDG_WLL_QUERY_ATTR_LOCATION:
		case EDG_WLL_QUERY_ATTR_DESTINATION:
		case EDG_WLL_QUERY_ATTR_USERTAG:
		case EDG_WLL_QUERY_ATTR_HOST:
		case EDG_WLL_QUERY_ATTR_CHKPT_TAG:
			return "char(250) binary null";

		case EDG_WLL_QUERY_ATTR_TIME:
			return "datetime null";
		default:
			return NULL;
	}
}

static void do_exit(edg_wll_Context ctx,int code)
{
	char	*et,*ed;

	edg_wll_Error(ctx,&et,&ed);
	fprintf(stderr,"edg-bkindex: %s (%s)\n",et,ed);
	exit(code);
}

static void usage(const char *me)
{
	fprintf(stderr,"usage: %s <options> [file]\n"
			"	-m,--mysql <dbstring>	use non-default database connection\n"
			"	-r,--really		really perform reindexing\n"
			"	-R,--remove		remove all indexes from server\n"
			"	-d,--dump		dump current setup\n"
			"	-v,--verbose		increase verbosity (2 levels)\n"
			"\n	-r and -d are mutually exlusive\n"
			"\n	-r and -R are mutually exlusive\n"
			"	[file] is applicable only without -d and -R\n",
			me);
}

/*
 * Set values of index columns in state table (after index reconfiguration)
 */

edg_wll_ErrorCode edg_wll_RefreshIColumns(edg_wll_Context ctx, void *job_index_cols) {

	edg_wll_Stmt sh, sh2;
	int njobs, ret = -1;
	intJobStat *stat;
	edg_wlc_JobId jobid;
	char *res[5];
	char *rest;
	char *icvalues, *stmt;
	int i;

	edg_wll_ResetError(ctx);
	if (!job_index_cols) return 0;

	if ((njobs = edg_wll_ExecStmt(ctx, "select s.jobid,s.int_status,s.seq,s.version,j.dg_jobid"
				       " from states s, jobs j where s.jobid=j.jobid",&sh)) < 0) {
		edg_wll_FreeStmt(&sh);
		return edg_wll_Error(ctx, NULL, NULL);
	}
	while ((ret=edg_wll_FetchRow(sh,res)) >0) {
		if (strcmp(res[3], INTSTAT_VERSION)) {
			stat = NULL;
			if (!edg_wlc_JobIdParse(res[4], &jobid)) {
				if ((stat = malloc(sizeof(intJobStat))) != NULL) {
					if (edg_wll_intJobStatus(ctx, jobid, 0, stat, 1)) {
						free(stat);
						stat = NULL;
					}
				}
				edg_wlc_JobIdFree(jobid);
			}
		} else {
			stat = dec_intJobStat(res[1], &rest);
			if (rest == NULL) stat = NULL;
		}
		if (stat == NULL) {
			edg_wll_FreeStmt(&sh);
			return edg_wll_SetError(ctx, EDG_WLL_ERROR_SERVER_RESPONSE,
				"cannot decode int_status from states DB table");
		}

		edg_wll_IColumnsSQLPart(ctx, job_index_cols, stat, 0, NULL, &icvalues);
		trio_asprintf(&stmt, "update states set seq=%s%s where jobid='%|Ss'", res[2], icvalues, res[0]);
		ret = edg_wll_ExecStmt(ctx, stmt, &sh2);
		edg_wll_FreeStmt(&sh2);

		for (i = 0; i < 5; i++) free(res[i]);
		destroy_intJobStat(stat); free(stat);
		free(stmt); free(icvalues);

		if (ret < 0) return edg_wll_Error(ctx, NULL, NULL);
		
	}
	edg_wll_FreeStmt(&sh);
	return edg_wll_Error(ctx, NULL, NULL);
}

