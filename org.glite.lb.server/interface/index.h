#ifndef GLITE_LB_INDEX_H
#define GLITE_LB_INDEX_H

#ident "$Header$"

#include "glite/lb/query_rec.h"

int edg_wll_QueryJobIndices(edg_wll_Context,edg_wll_QueryRec ***,char ***);
int edg_wll_QueryNotifIndices(edg_wll_Context,edg_wll_QueryRec ***,char ***);
int edg_wll_ColumnToQueryRec(const char *,edg_wll_QueryRec *);
char * edg_wll_QueryRecToColumn(const edg_wll_QueryRec *);
char * edg_wll_QueryRecToColumnExt(const edg_wll_QueryRec *);

int edg_wll_ParseIndexConfig(edg_wll_Context,const char *,edg_wll_QueryRec ***);
int edg_wll_DumpIndexConfig(edg_wll_Context,const char *,edg_wll_QueryRec * const *);

int edg_wll_CmpColumn(const edg_wll_QueryRec *,const edg_wll_QueryRec *);

char *edg_wll_StatToSQL(edg_wll_JobStat const *stat,edg_wll_QueryAttr attr);


typedef struct _edg_wll_IColumnRec {
	edg_wll_QueryRec	qrec;
	char *			colname;
} edg_wll_IColumnRec;

void edg_wll_FreeIColumnRec(edg_wll_IColumnRec *);
edg_wll_ErrorCode edg_wll_IColumnsSQLPart(edg_wll_Context, void *,  edg_wll_JobStat*, int , char **, char **);

int yylex();

extern int	lex_int;
extern char	*lex_out;
extern int	lex_line;

#define STD_PREFIX	"STD_"
#define USR_PREFIX	"USR_"
#define TIME_PREFIX	"TIME_"

#endif /* GLITE_LB_INDEX_H */
