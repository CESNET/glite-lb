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

/*
@@@AUTO
*/

@@@LANG: C

#include "args.h"
#include "glite/lb/events.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <fcntl.h>


typedef struct {
    int argc;
    char** argv;
    const char* help;
    int idx;
    edg_wll_Args list[1000];
    int last;
} opt_ctx_t;

/* lists of accepted tags */
@@@{
                gen "static const char * const eventJobCommon\[] = {";
                selectType $event '_common_';
                for (getFieldsOrdered $event) {
                        my $f = selectField $event $_;
                        my $fn = $f->{name};
                        gen "\"$fn\",  ";
                }
                gen "NULL };\n";
@@@}
@@@{
        for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
                        $event->getTypes) {
                gen "static const char * const event$t\[] = {";
                selectType $event $t;
                for (getFieldsOrdered $event) {
                        my $f = selectField $event $_;
                        my $fn = $f->{name};
                        gen "\"$fn\",  "
                }
                gen "NULL };\n";
        }
@@@}

static const char * const * const eventTags[] = {
        eventJobCommon,
@@@{
        for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
                        $event->getTypes) {
                gen "\tevent$t,\n";
        }
        gen "\tNULL\n};\n";
@@@}

static int read_bool(const edg_wll_Args* o, char* arg, char* par)
{
    int b = 1;
    int rs = 0;

    if (par)
    {
	if (!strcasecmp(par, "off") || !strcmp(par, "0")
	    || !strcasecmp(par, "false"))
	{
	    b = 0;
	    rs = 1;
	}
	else if (!strcasecmp(par, "on") || !strcmp(par, "1")
		 || !strcasecmp(par, "true"))
	{
	    rs = 1;
	}
    }

    if (o->value)
	*(int*)o->value = b;
    return rs;
}

static void read_float(const edg_wll_Args* o, char* arg, char* par)
{
    float d;
    if (!par)
    {
	printf("Option: %s  - missing float value\n", arg);
	exit(1);
    }
    d = strtof(par,NULL);
    if (o->min != o->max)
    {
	if (d < o->min && d > o->max)
	{
	    printf("Option: %s  - value: %f  out of range <%d, %d>",
		   arg, d, o->min, o->max);
	    exit(1);
	}
    }
    if (o->value)
	*(float*)o->value = d;
}

static void read_double(const edg_wll_Args* o, char* arg, char* par)
{
    double d;
    if (!par)
    {
	printf("Option: %s  - missing double value\n", arg);
	exit(1);
    }
    d = strtod(par,NULL);
    if (o->min != o->max)
    {
	if (d < o->min && d > o->max)
	{
	    printf("Option: %s  - value: %f  out of range <%d, %d>",
		   arg, d, o->min, o->max);
	    exit(1);
	}
    }
    if (o->value)
	*(double*)o->value = d;
}

static void read_string(const edg_wll_Args* o, char* arg, char* par)
{
    if (!par)
    {
	printf("Option: %s  - missing string value\n", arg);
	exit(1);
    }
    if (o->value)
    {
	if (strstr(arg, "-file"))
	{
            struct stat buf;
	    int fd = open(par, O_RDONLY);
	    if (fd >= 0)
	    {
                char* b;
		fstat(fd, &buf);
		b = (char*) malloc(buf.st_size);
		if (b)
		{
                    int p = 0;
		    printf("Opened & reading %s  %lld\n",
				par, (long long)buf.st_size);
		    while (p < buf.st_size)
		    {
			int r = read(fd, b + p, buf.st_size);
			if (r < 0)
			    break;
			p += r;
		    }

		    *(char**)o->value = b;
		}
                close(fd);
	    }
            else
		fprintf(stderr, "can't open file: \"%s\"  (%s)\n", par, strerror(errno));

	}
	else
	    *(char**)o->value = strdup(par);
    }
}

static void read_int(const edg_wll_Args* o, char* arg, char* par)
{
    int v = 0;
    if (!par)
    {
	printf("Option: %s  - missing integer value\n", arg);
	exit(1);
    }
    sscanf(par, "%i", &v);
    if (o->min != o->max)
    {
	if (v < o->min && v > o->max)
	{
	    printf("Option: %s  - value: %d  out of range <%d, %d>",
		   arg, v, o->min, o->max);
	    exit(1);
	}
    }
    if (o->value)
	*(int*)o->value = v;
}

static void read_uint16(const edg_wll_Args* o, char* arg, char* par)
{
    int v = 0;
    if (!par)
    {
	printf("Option: %s  - missing integer value\n", arg);
	exit(1);
    }
    sscanf(par, "%i", &v);
    if (o->min != o->max)
    {
	if (v < o->min && v > o->max)
	{
	    printf("Option: %s  - value: %d  out of range <%d, %d>",
		   arg, v, o->min, o->max);
	    exit(1);
	}
    }
    if (o->value)
	*(u_int16_t*)o->value = v;
}

static void read_event(const edg_wll_Args* o, char* arg, char* par)
{
    edg_wll_EventCode ec = edg_wll_StringToEvent(par);
    if (ec == EDG_WLL_EVENT_UNDEF)
    {
	if (strcmp(par, "help") == 0)
	{
	    // list type
	    int i, k = EDG_WLL_EVENT_UNDEF + 1;
	    printf("Available events:   extra options\n");
/* XXX: holes in edg_wll_EventCode */
	    for (i = EDG_WLL_EVENT_UNDEF + 1; i < EDG_WLL_EVENT__LAST; i++)
	    {
		char* e = edg_wll_EventToString(i);
		if (e)
		{
                    int j = 0;
		    printf("  %s:  ", e);
		    while (eventTags[k][j])
			printf("%s ", eventTags[k][j++]);
                    fputc('\n', stdout);
		    k++;
		    free(e);
		}
	    }
	}
        else
	    fprintf(stderr,"ERROR %s  unknown event: %s\n", arg, par);
	exit(1);
    }
    if (o->value)
	*(edg_wll_EventCode*)o->value = ec;

}

static void read_source(const edg_wll_Args* o, char* arg, char* par)
{
    edg_wll_Source s = edg_wll_StringToSource(par);
    if (s == EDG_WLL_SOURCE_NONE)
    {
	if (strcmp(par, "help") == 0)
	{
	    // list type
	    int i;
	    printf("Valid sources:\n");
	    for (i = EDG_WLL_SOURCE_NONE + 1; i < EDG_WLL_SOURCE__LAST; i++)
	    {
		char* e = edg_wll_SourceToString(i);
		if (e)
		{
		    printf("  %s\n", e);
       	    	    free(e);
		}
	    }
	}
	else
	    fprintf(stderr,"ERROR %s  unknown source: %s\n", arg, par);
	exit(1);
    }
    if (o->value)
	*(edg_wll_Source*)o->value = s;
}

/* FIXME: parse timeval from string correctly */
static void read_timeval(const edg_wll_Args* o, char* arg, char* par)
{
    int v = 0;
    if (!par)
    {
        printf("Option: %s  - missing timeval value\n", arg);
        exit(1);
    }
    sscanf(par, "%i", &v);
    if (o->min != o->max)
    {
        if (v < o->min && v > o->max)
        {
            printf("Option: %s  - value: %d  out of range <%d, %d>",
                   arg, v, o->min, o->max);
            exit(1);
        }
    }
    if (o->value)
        *(int*)o->value = v;
}

static void read_jobid(const edg_wll_Args* o, char* arg, char* par)
{
	glite_jobid_t jobid = NULL;

	if (glite_jobid_parse(par, &jobid) != 0) {
		fprintf(stderr, "ERROR %s  can't parse %s: %s\n", arg, par, strerror(errno));
		exit(1);
	}
	if (o->value) *(glite_jobid_t *)o->value = jobid;
	else glite_jobid_free(jobid);
}

static void read_taglist(const edg_wll_Args* o, char* arg, char* par)
{
	edg_wll_TagValue *list = NULL;

	if(edg_wll_TagListParse(par, &list) != 0) {
		fprintf(stderr, "ERROR %s  can't parse %s: %s\n", arg, par, strerror(errno));
		exit(1);
	}
	if(o->value) *(edg_wll_TagValue **)o->value = list;
	else /* FIXME: free it properly */ free(list);
}

static void show_help(const edg_wll_Args* o, int prefix)
{
    unsigned max = 0;
    char** l = malloc(sizeof(char*) * 1000);
    int li = 0;
    unsigned i = 0;

    for (i = 0; o[i].type != EDG_WLL_ARGS_NONE; i++)
    {
	char b[80];
        unsigned len;
	if (o[i].type == EDG_WLL_ARGS_HELP)
	    sprintf(b, "  -h  --help");
	else
	    sprintf(b, "  %c%s  %s%s",
		    (o[i].oshort && prefix) ? '-' : ' ',
		    o[i].oshort ? o[i].oshort : " ",
		    (o[i].olong && prefix) ? "--" : "",
		    o[i].olong ? o[i].olong : ""
		    //opt[i].options ? opt[i].options : "",
		    //o[i].help ? o[i].help : ""
		   );
	l[li] = strdup(b);
        li++;
	len = strlen(b);
	if (max < len)
	    max = len;
    }
    for (i = 0; o[i].type != EDG_WLL_ARGS_NONE; i++)
    {
	if (!o[i].oshort && !o[i].olong
	    && o[i].type != EDG_WLL_ARGS_HELP
	    && o[i].type != EDG_WLL_ARGS_OPTIONS)
            continue;

	if (o[i].type != EDG_WLL_ARGS_OPTIONS)
	{
            unsigned s;
	    fputs(l[i], stdout);
	    for (s = strlen(l[i]); s <= max; s++)
		fputc(' ', stdout);
	    if (o[i].type == EDG_WLL_ARGS_HELP)
		fputs("this help message", stdout);
	}

	if (o[i].value)
	{
	    switch (o[i].type)
	    {
	    case EDG_WLL_ARGS_INT:
		if (o[i].help)
		    printf(o[i].help, *(int*)o[i].value,
			   o[i].min, o[i].max);
		break;
	    case EDG_WLL_ARGS_STRING:
	    case EDG_WLL_ARGS_SELECTSTRING:
                if (o[i].help)
		    printf(o[i].help, *(const char**)o[i].value);
		break;
	    case EDG_WLL_ARGS_OPTIONS:
		show_help((const edg_wll_Args*)o[i].value, prefix);
		continue;
	    default:
                if (o[i].help)
		    fputs(o[i].help, stdout);
		break;
	    }
	}
	else if (o[i].help)
	    fputs(o[i].help, stdout);

	fputs("\n", stdout);
    }
    while (--li>=0) free(l[li]);
    free(l);
}
#if 0
static void parse_suboptions(const Option* o, const char* oname, char* pars, const char* r)
{
    avm::vector<char*> arr;
    split(arr, pars);

    if (!arr.size() || strcmp(arr[0], "help") == 0)
    {
	printf("Available options for '%s' (optA=x:optB=...)\n", oname);
	show_help(o, false);
	exit(0);
    }
    for (unsigned i = 0; i < arr.size(); i++)
    {
	char* par = strchr(arr[i], '=');
	if (par)
	{
	    *par = 0;
	    par++;
	}

	for (unsigned j = 0; o[j].type != Args::Option::NONE; j++)
	{
	    if ((o[j].oshort && strcmp(o[j].oshort, arr[i]) == 0)
		|| (o[j].olong && strcmp(o[j].olong, arr[i]) == 0))
	    {
		switch(o[j].type)
		{
		case Args::Option::BOOL:
		    read_bool(&o[j], arr[i], par, r);
		    break;
		case Args::Option::FLOAT:
		    read_float(&o[j], arr[i], par, r);
		    break;
		case Args::Option::DOUBLE:
		    read_double(&o[j], arr[i], par, r);
		    break;
		case Args::Option::INT:
		    read_int(&o[j], arr[i], par, r);
		    break;
		default:
		    ;
		}
	    }
	}
    }
}
#endif

static int findOpt(opt_ctx_t* ctx, int olong)
{
    char* arg = ctx->argv[ctx->idx] + olong + 1;
    char* par = strchr(arg, '=');
    const edg_wll_Args* o = NULL;
    const edg_wll_Args* ol[200];
    int olb = 0;
    int ole = 0;

    char* argnofile = strdup(arg);
    char* nofile = strstr(argnofile, "-file"); // should be the ending
    if (nofile && (nofile - argnofile) > 2)
	nofile[0] = 0;

    olong++;
    if (par)
    {
        *par = 0;
	par++;
    }
    else if ((ctx->idx + 1) < ctx->argc)
    {
	par = ctx->argv[++ctx->idx];
    }

    ol[ole++] = ctx->list;
    while (ole > olb)
    {
	o = ol[olb++];
	for (; o->type != EDG_WLL_ARGS_NONE; o++)
	{
	    //printf("OPTION %d   '%s'  %d   '%s' '%s'\n", o->type, arg, olong, o->oshort, o->olong);
	    if (o->type == EDG_WLL_ARGS_HELP
		&& (strcasecmp(arg, "h") == 0
		    || strcmp(arg, "?") == 0
		    || strcasecmp(arg, "help") == 0))
		break;
	    if (o->type == EDG_WLL_ARGS_OPTIONS)
	    {
                ol[ole++] = o->value;
                continue;
	    }

	    if (olong < 2)
	    {
		if (o->oshort &&
		    (strcmp(arg, o->oshort) == 0
		     || strcmp(argnofile, o->oshort) == 0))
		    break;
	    }
	    else if (o->olong && (strcmp(arg, o->olong) == 0
				  || strcmp(argnofile, o->olong) == 0))
		break;
	}
	if (o->type != EDG_WLL_ARGS_NONE)
	    break;
    }

    switch (o->type)
    {
    case EDG_WLL_ARGS_NONE:
	return -1;
    case EDG_WLL_ARGS_BOOL:
	if (!read_bool(o, arg, par))
	    ctx->idx--; // no argument given
	break;
    case EDG_WLL_ARGS_FLOAT:
	read_float(o, arg, par);
	break;
    case EDG_WLL_ARGS_DOUBLE:
	read_double(o, arg, par);
	break;
    case EDG_WLL_ARGS_STRING:
        read_string(o, arg, par);
        break;
    case EDG_WLL_ARGS_INT:
	read_int(o, arg, par);
	break;
    case EDG_WLL_ARGS_UINT16:
	read_uint16(o, arg, par);
	break;
    case EDG_WLL_ARGS_EVENT:
	read_event(o, arg, par);
	break;
    case EDG_WLL_ARGS_SOURCE:
	read_source(o, arg, par);
	break;
    case EDG_WLL_ARGS_HELP:
	printf("\nUsage: %s %s\n\n", ctx->argv[0], ctx->help);
	show_help(ctx->list, 1);
	exit(0);
    //case EDG_WLL_ARGS_SUBOPTIONS:
	//parse_suboptions((const edg_wll_Args*)o->value, arg, par, regname);
    case EDG_WLL_ARGS_TIMEVAL:
	read_timeval(o, arg, par);
	break;
    case EDG_WLL_ARGS_JOBID:
	read_jobid(o, arg, par);
	break;
    case EDG_WLL_ARGS_TAGLIST:
	read_taglist(o, arg, par);
	break;
    default:
	printf("FIXME: unhandle option type %d\n", o->type);
        break;
    }

    if (argnofile)
        free(argnofile);

    return 0;
}

static void addOptions(opt_ctx_t* ctx, const edg_wll_Args* options)
{
    const edg_wll_Args* o = options;

    while (o->type != EDG_WLL_ARGS_NONE)
    {
	ctx->list[ctx->last++] = *o;
        o++;
    }
}

void edg_wll_ParseArgs(int* argc, char** argv, const edg_wll_Args* options,
		       const char* help)
{
    int sidx = 1;
    opt_ctx_t ctx;

    ctx.argc = *argc;
    ctx.argv = argv;
    ctx.help = help;
    ctx.last = 0;

    addOptions(&ctx, options);

    for (ctx.idx = 1; ctx.idx < ctx.argc; ctx.idx++)
    {
	if (argv[ctx.idx][0] == '-')
	{
	    int olong = (argv[ctx.idx][1] == '-');
	    if (olong && argv[ctx.idx][2] == 0)
		break; // end of options
	    //printf("ARG %d  %s\n", ctx.idx, argv[ctx.idx]);
	    if (findOpt(&ctx, olong) == 0)
		continue;
	}
	else if (sidx != ctx.idx)
	{
	    fprintf(stderr,"SIDX  %d  %d\n", sidx, ctx.idx);
	    argv[sidx] = argv[ctx.idx];
	}
	sidx++;
    }

    while (ctx.idx < *argc && sidx != ctx.idx)
    {
	argv[sidx++] = argv[ctx.idx++];
    }

    *argc = sidx;
}
