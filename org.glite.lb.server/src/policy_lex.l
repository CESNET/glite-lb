%{

#ident "$Header$"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "policy_gram.h"

unsigned lineno = 1;

#define YY_NO_UNPUT

%}
%%

resource	{ return RESOURCE; }
action		{ return ACTION; }
rule		{ return RULE; }
permit		{ return PERMIT; }
[A-Za-z0-9_]+	{ 
		  yylval.string = strdup ((const char *)yytext);
		  return LITERAL;
		}
\"[^\"\n]*\"	{
		  int len;
		  yylval.string = malloc(len = strlen(yytext)-1);
		  strncpy(yylval.string,yytext+1,len-1);
		  yylval.string[len-1] = '\0';
		  return STRING;
		} 
[={}]		{ return *yytext; }
"\n"		{ lineno++; }
[ \t]+		;
.		{ return *yytext; }
%%

int yywrap() { return 1; }
