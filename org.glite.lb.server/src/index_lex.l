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

#include <string.h>

#include "glite/lb/context-int.h"
#include "index_parse.h"
#include "index.h"

char	*lex_out;
int	lex_int;
int	lex_line;

int yywrap(void) { return 1; }

#define YY_NO_UNPUT

%}


delim	[ \t]
ws	{delim}+
string	\"[^\"\n]*\"
digit	[0-9]
int	{digit}+

%%
{ws}	{}

JobIndices	return JOB_INDICES;
type		return TYPE;
name		return NAME;
prefixlen	return PREFIX;
{int}		{
			lex_int = atoi(yytext);
			return INT;
		}
{string}	{
			int	len;
			lex_out = malloc(len = strlen(yytext)-1);
			strncpy(lex_out,yytext+1,len-1);
			lex_out[len-1] = 0;
			return STRING;
		}
\n	lex_line++;
.		return *yytext;

