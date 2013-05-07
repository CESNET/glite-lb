%{

#ident "$Header$"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "glite/lb/context-int.h"
#include "authz_policy.h"

void yyerror (const char *);
void set_error(const char *format, ...);

static edg_wll_Context parse_ctx;
static const char *parse_fname;
static edg_wll_authz_policy parse_policy;

extern unsigned lineno;

extern FILE *yyin;

extern int yylex(void);

struct _rules {
    struct _edg_wll_authz_rule *rule;
    struct _rules *next;
} _rules;

%}

%union {
    char *string;
    struct _rules *rules;
    struct _edg_wll_authz_rule *rule;
    struct _edg_wll_authz_attr *attr;
}

%token RESOURCE ACTION RULE PERMIT
%token <string> STRING
%token <string> LITERAL
%type <attr> assignment
%type <rule> assignments
%type <rules> rule rules

%start policy

%%

policy		: RESOURCE STRING '{' actions '}'
		{
			if (strcmp($2, "LB") != 0)
				set_error("Undefined resource '%s'", $2);
		}
		;

actions		: 
		| action actions
		;

action		: ACTION STRING '{' rules '}'
		{
			struct _rules *r;

			authz_action ac = find_authz_action($2);

			if (ac == ACTION_UNDEF)
				set_error("undefined action '%s'", $2);

			for (r = $4; r; r = r->next) {
				edg_wll_add_authz_rule(parse_ctx, parse_policy,
					ac, r->rule);
			}
		}
		;

rules		:
		{
			$$ = NULL;
		}
		| rule rules
		{
			$1->next = $2;
			$$ = $1;
		}
		;

rule		: RULE PERMIT '{' assignments '}'
		{
			$$ = malloc(sizeof(*$$));
			$$->rule = $4;
			$$->next = NULL;
		}
		;

assignments	:
		{
			$$ = calloc(1, sizeof(*$$));
		}
		| assignment assignments
		{
			edg_wll_add_authz_attr(parse_ctx, $2, $1->id, $1->value);
			$$ = $2;
		}
		;

assignment	: LITERAL '=' STRING
		{
			$$ = malloc(sizeof(*$$));
			$$->id = find_authz_attr($1);
			if ($$->id == ATTR_UNDEF)
				set_error("undefined attribute '%s'", $1);
			$$->value = $3;
		}
		;

%%

void set_error(const char *format, ...)
{
    va_list args;
    char *buf, *msg;

    va_start (args, format);
    vasprintf (&buf, format, args);
    va_end(args);

    asprintf(&msg, "%s:%d: %s", parse_fname, lineno, buf);
    edg_wll_SetError(parse_ctx, EINVAL, msg);
    free(buf);
    free(msg);
}

void yyerror(const char *str)
{
    set_error("%s", str);
}

int
parse_server_policy(edg_wll_Context ctx, const char *filename, edg_wll_authz_policy policy)
{
    lineno = 1;
    yyin = fopen(filename, "r");
    if (yyin == NULL)
	return edg_wll_SetError(ctx,errno,filename);

    parse_ctx = ctx;
    parse_fname = filename;
    parse_policy = policy;
    edg_wll_ResetError(ctx);
    yyparse();
    fclose(yyin);

    return edg_wll_Error(ctx,NULL,NULL);
}
