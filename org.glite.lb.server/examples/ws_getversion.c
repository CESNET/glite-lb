#include <getopt.h>
#include <stdsoap2.h>

#include "glite/lb/consumer.h"

#include "ws_plugin.h"
#include "bk_ws_H.h"

static struct option opts[] = {
	{"help",	0,	NULL,	'h'},
	{"server",	1,	NULL,	'm'},
};

static void usage(char *me)
{
	fprintf(stderr,"usage: %s [option]\n"
		"\t-h, --help      Shows this screen.\n"
		"\t-m, --server    BK server address:port.\n"
		, me);
}

static void printstat(edg_wll_JobStat stat, int level);

int main(int argc,char** argv)
{
    edg_wll_Context						ctx;
    struct soap						   *mydlo = soap_new();
    struct edgwll2__GetVersionResponse	out;
    int									opt, err;
	char							   *server = "http://localhost:8999/",
									   *name = NULL;


	name = strrchr(argv[0],'/');
	if (name) name++; else name = argv[0];

	while ((opt = getopt_long(argc, argv, "hm:", opts, NULL)) != EOF) switch (opt)
	{
	case 'h': usage(name); return 0;
	case 'm': server = strdup(optarg); break;
	case '?': usage(name); return 1;
	}

    edg_wll_InitContext(&ctx);

	if ( soap_register_plugin_arg(mydlo, edg_wll_ws_plugin, (void *)ctx) )
	{
		soap_print_fault(mydlo, stderr);
		return 1;
	}

    switch (err = soap_call_edgwll2__GetVersion(mydlo, server, "", &out))
	{
	case SOAP_OK: printf("Server version: %s\n", out.version); break;
	case SOAP_FAULT: 
	default: printf("???\n");
    }

    return 0;
}
