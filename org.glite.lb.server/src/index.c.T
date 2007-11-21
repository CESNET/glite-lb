#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "glite/lb/context-int.h"
#include "glite/lbu/trio.h"
#include "db_supp.h"
#include "index.h"

extern int debug;

#define const_len(c)	(sizeof((c))-1)

static int parse_indices(edg_wll_Context ctx,char **keys,char ***column_names,const char **built_in_indices,edg_wll_QueryRec *** index_out,char ***keys_out);

int edg_wll_QueryJobIndices(edg_wll_Context ctx,edg_wll_QueryRec *** index_out,char ***keys_out)
{
	static const char *built_in_indices[] = {
		"PRIMARY",
		"parent_job",
		NULL
	};
	char	**keys = NULL;
	char	***column_names = NULL;

	if (glite_lbu_QueryIndices(ctx->dbctx, "states", &keys, &column_names) != 0) {
		edg_wll_SetErrorDB(ctx);
		return EIO;
	}

	if (!keys) {
		*index_out = NULL;
		if (keys_out) *keys_out = NULL;
		return edg_wll_ResetError(ctx);
	}

/* XXX: keys are passed up or freed, column_names are freed there */
	if (parse_indices(ctx,keys,column_names,built_in_indices,index_out,keys_out)) 
		return edg_wll_Error(ctx,NULL,NULL);
	else return edg_wll_ResetError(ctx);

}

int edg_wll_QueryNotifIndices(edg_wll_Context ctx,edg_wll_QueryRec *** index_out,char ***keys_out)
{
	static const char *built_in_indices[] = {
		"PRIMARY",
		NULL
	};
	char	**keys = NULL;
	char	***column_names = NULL;

	if (glite_lbu_QueryIndices(ctx->dbctx, "notif_registrations", &keys, &column_names) != 0) {
		edg_wll_SetErrorDB(ctx);
		return EIO;
	}

	if (!keys) {
		*index_out = NULL;
		if (keys_out) *keys_out = NULL;
		return edg_wll_ResetError(ctx);
	}

/* XXX: keys are passed up or freed, column_names are freed there */
	if (parse_indices(ctx,keys,column_names,built_in_indices,index_out,keys_out)) 
		return edg_wll_Error(ctx,NULL,NULL);
	else return edg_wll_ResetError(ctx);

}

/* TODO: 
   - better error recovery (skip unrecognised indices etc.)
   - leaks memory on errors
*/

static int parse_indices(edg_wll_Context ctx,char **keys,char ***column_names,const char **built_in_indices,edg_wll_QueryRec *** index_out,char ***keys_out)
{

	edg_wll_QueryRec	**idx = NULL;

	int	*cols = NULL;

	int	i, j;
	int	nkeys = 0;



	for (i = 0; keys[i]; i++) {
		// skip builtin indices
		for (j = 0; built_in_indices[j]; j++) {
			if (strcasecmp(keys[i], built_in_indices[j]) == 0)
				goto next_index;
		}

		cols = realloc(cols,(nkeys+1) * sizeof cols[0]); 
		cols[nkeys] = 0;
		idx = realloc(idx,(nkeys+2) * sizeof idx[0]);
		idx[nkeys] = idx[nkeys+1] = NULL;

		for (j = 0; column_names[i][j]; j++) {
			if (cols[nkeys] <= j) {
				cols[nkeys] = j+1;
				idx[nkeys] = realloc(idx[nkeys],(j+2)*sizeof idx[nkeys][0]);
				memset(&idx[nkeys][j+1],0,sizeof idx[nkeys][0]);
			}
		
			if (edg_wll_ColumnToQueryRec(column_names[i][j],&idx[nkeys][j])) {
				int   code;
				char *ed;

				asprintf(&ed, "%s(%s): unsupported column", keys[i], column_names[i][j]);
				code = edg_wll_SetError(ctx, EINVAL, ed);
				free(ed);
				return code;
			}
#warning: TODO: needed Sub_part in value???
//      	               else idx[nkeys][j].value.i = atoi(showcol[Sub_part]);
		}
		nkeys++;
next_index:
		for (j = 0; column_names[i][j]; j++) free(column_names[i][j]);
		free(column_names[i]);
		column_names[i] = NULL;
		// just for escaping from nested cycles
		;	/* prevent compiler to complain */
	}

	free(cols);
	if (keys_out) *keys_out = keys;
	else { 
		for (i=0; i<nkeys; i++) free(keys[i]); 
		free(keys);
	}
	*index_out = idx;

	return edg_wll_ResetError(ctx);
}

int edg_wll_CmpColumn(const edg_wll_QueryRec *r1,const edg_wll_QueryRec *r2)
{
	if (r1->attr != r2->attr) return 1;
	switch (r1->attr) {
		case EDG_WLL_QUERY_ATTR_USERTAG:
			return strcasecmp(r1->attr_id.tag,r2->attr_id.tag);
		case EDG_WLL_QUERY_ATTR_TIME:
			return r1->attr_id.state != r2->attr_id.state;
		default:
			return 0;
	}
}

static char *to_sql_string(edg_wll_JobStat const *stat,int offset)
{
	char	*out = NULL,
	    	*in = *((char **) (((char *) stat) + offset));

       	if (in) trio_asprintf(&out,"'%|Ss'",in);
	return out;
}

static char *to_sql_timeval(edg_wll_JobStat const *stat,int offset)
{
	char	*out;
	glite_lbu_TimeToDB( ((struct timeval *) (((char *) stat) + offset))->tv_sec, &out );
	return out;
}

static edg_wll_JobStat	fakestat;

static struct {
	const char	*name;
	edg_wll_QueryAttr	attr;
	int		offset;
	char	* (*to_sql)(edg_wll_JobStat const *,int);
} std_attrs[] =
{
@@@{
	for my $n ($status->getAllFieldsOrdered) {
		my $f = selectField $status $n;
		next unless $f->{index};
		my $u = uc $n;
		gen "\t{ \"$n\", EDG_WLL_QUERY_ATTR_$u, ((char *) \&fakestat.$n) - ((char *) \&fakestat),\&to_sql_$f->{type} },\n";
	}
@@@}
	{ NULL, },
};

/* TODO: use in queries */
char *edg_wll_StatToSQL(edg_wll_JobStat const *stat,edg_wll_QueryAttr attr)
{
	int	i;

	for (i=0; std_attrs[i].attr && std_attrs[i].attr != attr; i++);

	return std_attrs[i].attr ? std_attrs[i].to_sql(stat,std_attrs[i].offset) : (char *) -1;
}

int edg_wll_ColumnToQueryRec(const char *col_name,edg_wll_QueryRec *rec)
{
	int	i;

	memset(rec,0,sizeof *rec);
	if (strncasecmp(col_name,STD_PREFIX,const_len(STD_PREFIX)) == 0) {
		for (i=0; std_attrs[i].name
			&& strcasecmp(std_attrs[i].name,col_name+const_len(STD_PREFIX)); i++);

		if (std_attrs[i].name) rec->attr = std_attrs[i].attr;
	}
	else if (strncasecmp(col_name,TIME_PREFIX,const_len(TIME_PREFIX)) == 0) {
		rec->attr_id.state = edg_wll_StringToStat(col_name+const_len(TIME_PREFIX));
		if (rec->attr_id.state != (edg_wll_JobStatCode) -1) rec->attr = EDG_WLL_QUERY_ATTR_TIME;
	}
	else if (strncasecmp(col_name,USR_PREFIX,const_len(USR_PREFIX)) == 0) {
		rec->attr = EDG_WLL_QUERY_ATTR_USERTAG;
		rec->attr_id.tag = strdup(col_name+const_len(USR_PREFIX));
		rec->value.c = NULL;
	}

	return !rec->attr;
}

char * edg_wll_QueryRecToColumn(const edg_wll_QueryRec *rec)
{
	char	col[100] = "";

	if (rec->attr == EDG_WLL_QUERY_ATTR_USERTAG) {
		strcpy(col,USR_PREFIX);
		strcat(col,rec->attr_id.tag);
	}
	else if (rec->attr == EDG_WLL_QUERY_ATTR_TIME) {
		char	*s = edg_wll_StatToString(rec->attr_id.state);

		if (s) {
			strcpy(col,TIME_PREFIX);
			strcat(col,s);
			free(s);
		}
	}
	else {
		int	i;
		for (i=0; std_attrs[i].name && std_attrs[i].attr != rec->attr; i++);
		if (std_attrs[i].name) {
			strcpy(col,STD_PREFIX);
			strcat(col,std_attrs[i].name);
		}
	}

	return col[0] ? strdup(col) : NULL;
}

char * edg_wll_QueryRecToColumnExt(const edg_wll_QueryRec *rec)
{
	char	*intern = edg_wll_QueryRecToColumn(rec),
		*out;

	if (!intern) return NULL;

	switch (rec->attr) {
		case EDG_WLL_QUERY_ATTR_USERTAG:
			out = strdup(intern+const_len(USR_PREFIX));
			break;
		case EDG_WLL_QUERY_ATTR_TIME:
			out = strdup(intern+const_len(TIME_PREFIX));
			break;
		default:
			out = strdup(intern+const_len(STD_PREFIX));
			break;
	}
	free(intern);
	return out;
}

void edg_wll_FreeIColumnRec(edg_wll_IColumnRec *icrp)
{
	if (icrp->qrec.attr == EDG_WLL_QUERY_ATTR_USERTAG)
		free(icrp->qrec.attr_id.tag);
	free(icrp->colname);
}

