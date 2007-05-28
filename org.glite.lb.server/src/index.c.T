#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "glite/lb/context-int.h"
#include "lbs_db.h"
#include "index.h"

extern int debug;

#define const_len(c)	(sizeof((c))-1)

/* TODO: 
   - better error recovery (skip unrecognised indices etc.)
   - leaks memory on errors
*/

int edg_wll_QueryJobIndices(edg_wll_Context ctx,edg_wll_QueryRec *** index_out,char ***keys_out)
{
	edg_wll_QueryRec	**idx = NULL;
	edg_wll_Stmt		stmt = NULL;

	int	i,j,ret;

	static const char *built_in_indices[] = {
		"PRIMARY",
		"parent_job",
		NULL
	};

/* XXX: "show index from" columns. Matches at least MySQL 4.0.11 */

	char	*showcol[12];
	int	Key_name,Seq_in_index,Column_name,Sub_part;

	char	**keys = NULL;
	int	*cols = NULL;
	char	**col_names = NULL;

	int	nkeys = 0;

	Key_name = Seq_in_index = Column_name = Sub_part = -1;

	if (edg_wll_ExecStmt(ctx,"show index from states",&stmt)<0) 
		return edg_wll_Error(ctx,NULL,NULL);

	while ((ret = edg_wll_FetchRow(stmt,showcol))) {
		if (ret < 0) return edg_wll_Error(ctx,NULL,NULL);
		assert(ret <= sizeof showcol/sizeof showcol[0]);

		if (!col_names) {
			col_names = malloc(ret * sizeof col_names[0]);
			edg_wll_QueryColumns(stmt,col_names);
			for (i=0; i<ret; i++) 
				if (!strcasecmp(col_names[i],"Key_name")) Key_name = i;
				else if (!strcasecmp(col_names[i],"Seq_in_index")) Seq_in_index = i;
				else if (!strcasecmp(col_names[i],"Column_name")) Column_name = i;
				else if (!strcasecmp(col_names[i],"Sub_part")) Sub_part = i;

			assert(Key_name >= 0 && Seq_in_index >= 0 && 
					Column_name >= 0 && Sub_part >= 0);

		}


		for (j=0; built_in_indices[j]; j++) {
			if (strcasecmp(showcol[Key_name],built_in_indices[j]) == 0) {
				for (i=0; i<ret; i++) free(showcol[i]);
				goto continue_fetch_index;
			}
		}
		
		for (i=0; i<nkeys && strcasecmp(showcol[Key_name],keys[i]); i++);

		if (i == nkeys) {
			keys = realloc(keys,(i+2) * sizeof keys[0]);
			keys[i] = showcol[Key_name];
			keys[i+1] = NULL;
			cols = realloc(cols,(i+1) * sizeof cols[0]); 
			cols[i] = 0;
			idx = realloc(idx,(i+2) * sizeof idx[0]);
			idx[i] = idx[i+1] = NULL;
			showcol[Key_name] = NULL;
			nkeys++;
		}

		j = atoi(showcol[Seq_in_index])-1;
		if (cols[i] <= j) {
			cols[i] = j+1;
			idx[i] = realloc(idx[i],(j+2)*sizeof idx[i][0]);
			memset(&idx[i][j+1],0,sizeof idx[i][0]);
		}

		if (edg_wll_ColumnToQueryRec(showcol[Column_name],&idx[i][j])) {
			char	ed[300];
			sprintf(ed,"%s(%s): unsupported column",keys[i],showcol[Column_name]);
			return edg_wll_SetError(ctx,EINVAL,ed);
		}
		else idx[i][j].value.i = atoi(showcol[Sub_part]);
			
		for (i=0; i<ret; i++) free(showcol[i]);
continue_fetch_index:
		// just for escaping from nested cycles		
		;	/* prevent compiler to complain	*/
	}

	edg_wll_FreeStmt(&stmt);
	free(cols);
	free(col_names);
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

static struct {
	const char	*name;
	edg_wll_QueryAttr	attr;
} std_attrs[] =
{
@@@{
	for my $n ($status->getAllFieldsOrdered) {
		my $f = selectField $status $n;
		next unless $f->{index};
		my $u = uc getName $f;
		gen "\t{ \"$n\", EDG_WLL_QUERY_ATTR_$u },\n";
	}
@@@}
	{ NULL, },
};

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

