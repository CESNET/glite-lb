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


int edg_wll_QueryJobIndices(edg_wll_Context,edg_wll_QueryRec ***,char ***);
int edg_wll_ColumnToQueryRec(const char *,edg_wll_QueryRec *);
char * edg_wll_QueryRecToColumn(const edg_wll_QueryRec *);
char * edg_wll_QueryRecToColumnExt(const edg_wll_QueryRec *);

int edg_wll_ParseIndexConfig(edg_wll_Context,const char *,edg_wll_QueryRec ***);
int edg_wll_DumpIndexConfig(edg_wll_Context,const char *,edg_wll_QueryRec * const *);

int edg_wll_CmpColumn(const edg_wll_QueryRec *,const edg_wll_QueryRec *);

typedef struct _edg_wll_IColumnRec {
	edg_wll_QueryRec	qrec;
	char *			colname;
} edg_wll_IColumnRec;

void edg_wll_FreeIColumnRec(edg_wll_IColumnRec *);

int yylex();

extern int	lex_int;
extern char	*lex_out;
extern int	lex_line;

#define STD_PREFIX	"STD_"
#define USR_PREFIX	"USR_"
#define TIME_PREFIX	"TIME_"
