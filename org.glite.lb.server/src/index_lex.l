%{
#ident "$Header$"

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

