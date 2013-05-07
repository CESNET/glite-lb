%{
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
#include <string.h>
#include <errno.h>

#include "glite/lb/context-int.h"

#include "index.h"

#define yyerror(x) {}

#define YYDEBUG	1

#define ATTR_TYPE_SYSTEM	"system"
#define ATTR_TYPE_USER		"user"
#define ATTR_TYPE_TIME		"time"

static edg_wll_Context parse_ctx;
static const char	*parse_fname;

#define bailout(msg) \
{ \
	char	*buf; \
 \
	asprintf(&buf,"%s:%d: %s",parse_fname,lex_line,(msg)); \
	edg_wll_SetError(parse_ctx,EINVAL,buf); \
	free(buf); \
	YYABORT; \
}

extern FILE *yyin;

edg_wll_QueryRec	**indices_out;

extern int yylex(void);
%}

%term JOB_INDICES
%term STRING
%term INT
%term TYPE
%term NAME
%term PREFIX

%union
{
	char	*s;
	int	i;
	edg_wll_QueryRec	qr;
	edg_wll_QueryRec	*qrl;
	edg_wll_QueryRec	**qrll;
	struct elem_attr {
		int	attr;
		char	*val;
		int	intval;
	}			attr;
}

%type <s> string
%type <i> int
%type <qr> job_index_elem
%type <qrl> job_index job_index_elem_list
%type <qrll> job_index_list job_indices;
%type <attr> elem_attr opt_elem_attr

%%

config	: '[' job_indices soft_semicolon ']'	{ indices_out = $2; }
	;

job_indices	: JOB_INDICES '=' '{' job_index_list soft_comma '}' { $$ = $4; }
		;

job_index_list	: job_index	{ $$ = calloc(2,sizeof (*$$)); *$$ = $1; }
		| job_index_list ',' job_index
{
	int	i;
	for (i=0; $1[i]; i++);
	$$ = realloc($1,(i+2) * sizeof *$1);
	$$[i] = $3;
	$$[i+1] = NULL;
}
		;

job_index	: job_index_elem		{ $$ = calloc(2,sizeof (*$$)); memcpy($$,&$1,sizeof $1); }
		| '{' job_index_elem_list '}'	{ $$ = $2; }
		;

job_index_elem_list	: job_index_elem	{ $$ = calloc(2,sizeof (*$$)); memcpy($$,&$1,sizeof $1); }
		| job_index_elem_list ',' job_index_elem
{
	int	i;
	for (i=0; $1[i].attr; i++);
	$$ = realloc($1,(i+2) * sizeof *$1);
	memcpy($$+i,&$3,sizeof $3);
	memset($$+i+1,0,sizeof *$$);
}
		;

job_index_elem	: '[' elem_attr ';' elem_attr opt_elem_attr ']'
{
	char	*name = $2.attr == NAME ? $2.val :
				$4.attr == NAME ? $4.val : 
				$5.attr == NAME ? $5.val : NULL,
		*type = $2.attr == TYPE ? $2.val :
				$4.attr == TYPE ? $4.val :
				$5.attr == TYPE ? $5.val : NULL;
	int	prefix = $2.attr == PREFIX ? $2.intval :
				$4.attr == PREFIX ? $4.intval :
				$5.attr == PREFIX ? $5.intval : 0;


	if (!name) bailout("`name' required");
	if (!type) bailout("`type' required");

	if (strcasecmp(type,ATTR_TYPE_SYSTEM) == 0) {
		char	*name2;
		asprintf(&name2,STD_PREFIX "%s",name);
		if (edg_wll_ColumnToQueryRec(name2,&$$)) bailout("unknown attribute");
		free(name2);
		free(name);
	}
	else if (strcasecmp(type,ATTR_TYPE_USER) == 0) {
		$$.attr = EDG_WLL_QUERY_ATTR_USERTAG;
		$$.attr_id.tag = name;
	}
	else if (strcasecmp(type,ATTR_TYPE_TIME) == 0) {
		char	*name2;
		if (prefix) bailout("PREFIXLEN is not valid with time attributes");
		asprintf(&name2,TIME_PREFIX "%s",name);
		if (edg_wll_ColumnToQueryRec(name2,&$$)) bailout("unknown attribute");
		free(name2);
		free(name);
	}
	else bailout("unknown attr type");

	$$.value.i = prefix;
}
		;

elem_attr	: TYPE '=' string	{ $$.attr = TYPE; $$.val = $3; }
		| NAME '=' string	{ $$.attr = NAME; $$.val = $3; }
		| PREFIX '=' int	{ $$.attr = PREFIX; $$.intval = $3; }
		;

opt_elem_attr	: 	{ $$.attr = 0; $$.val = NULL; }
		|	';' elem_attr	{ $$ = $2; }
		;

string		: STRING	{ $$ = lex_out; lex_out = NULL; }
		;

int		: INT		{ $$ = lex_int; }
		;

soft_semicolon	:
		| ';'
		;

soft_comma	:
		| ','
		;


%%


/* XXX: uses static variables -- non thread-safe */

int edg_wll_ParseIndexConfig(edg_wll_Context ctx,const char *fname,edg_wll_QueryRec ***out)
{
	yyin = strcmp(fname,"-") ? fopen(fname,"r") : stdin;
	lex_line = 1;

	if (!yyin) return edg_wll_SetError(ctx,errno,fname);

	parse_ctx = ctx;
	parse_fname = fname;
	edg_wll_ResetError(ctx);

	/* yydebug = 1; */
	if (yyparse() && !edg_wll_Error(ctx,NULL,NULL)) {
		char	buf[100];
		if (yyin != stdin) fclose(yyin);
		sprintf(buf,"%s:%d: parse error",fname,lex_line);
		return edg_wll_SetError(ctx,EINVAL,buf);
	}
	if (yyin != stdin) fclose(yyin);

	if (!edg_wll_Error(ctx,NULL,NULL)) *out = indices_out;
	indices_out = NULL;	/* XXX: memory leak on error but who cares? */
	return edg_wll_Error(ctx,NULL,NULL);
}

int edg_wll_DumpIndexConfig(edg_wll_Context ctx,const char *fname,edg_wll_QueryRec * const *idx)
{
	int	haveit = 0;

	FILE	*f = strcmp(fname,"-") ? fopen(fname,"w") : stdout;

	if (!f) return edg_wll_SetError(ctx,errno,fname);
	if (idx && *idx) { haveit = 1; fputs("[\n\tJobIndices = {\n",f); }

	while (idx && *idx) {
		const edg_wll_QueryRec *i;
		int	multi = (*idx)[1].attr;

		fputs(multi ? "\t\t{\n" : "\t\t",f);

		for (i=*idx; i->attr; i++) {
			char	*cn = edg_wll_QueryRecToColumnExt(i);
			char	prefix[100] = "";
			char	*type;

			switch (i->attr) {
				case EDG_WLL_QUERY_ATTR_USERTAG: type = ATTR_TYPE_USER; break;
				case EDG_WLL_QUERY_ATTR_TIME: type = ATTR_TYPE_TIME; break;
				default: type = ATTR_TYPE_SYSTEM; break;
			}
				
			if (i->value.i) sprintf(prefix,"; prefixlen = %d ",i->value.i);
			if (multi) fputs("\t\t\t",f);
			fprintf(f,"[ type = \"%s\"; name = \"%s\" %s]",type,cn,prefix);
			if (multi) fputs(i[1].attr ? ",\n" : "\n",f);
			free(cn);
		}

		if (multi) fputs("\t\t}",f);
		fputs(idx[1] ? ",\n" : "\n",f);

		idx++;
	}

	if (haveit) {
		fputs("\t}\n]\n",f);
		return edg_wll_ResetError(ctx);
	}
	else return edg_wll_SetError(ctx,ENOENT,"no indices");
}


