#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

extern int edg_wll_DoLogEvent(edg_wll_Context context, edg_wll_LogLine logline);
extern int edg_wll_DoLogEventProxy(edg_wll_Context context, edg_wll_LogLine logline);

extern char *optarg;
extern int opterr,optind;

static struct option const long_options[] {
		{"dst", required_arg, NULL, 0},
		{
		{NULL, 0, NULL, 0}
};


void
print_usage(char *name)
{
	fprintf(stderr, "Usage: %s \n", name);
}


int
main(int argc, char *argv[])
{
}
