#include <getopt.h>
#include <stdsoap2.h>

#include "glite/security/glite_gsplugin.h"
#include "glite/lb/consumer.h"

#include "bk_ws_H.h"

#include "LoggingAndBookkeeping.nsmap"

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

//static void printstat(edg_wll_JobStat stat, int level);

int main(int argc,char** argv)
{
    edg_wll_Context						ctx;
    glite_gsplugin_Context				gsplugin_ctx;
    struct soap							soap;
    struct _lbe__GetVersionResponse	out;
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
    glite_gsplugin_init_context(&gsplugin_ctx);

	soap_init(&soap);

	if ( soap_register_plugin_arg(&soap, glite_gsplugin, (void *)gsplugin_ctx) )
	{
		soap_print_fault(&soap, stderr);
		return 1;
	}

	glite_gsplugin_set_udata(&soap, gsplugin_ctx);

    switch (err = soap_call___lb__GetVersion(&soap, server, "", NULL, &out))
	{
	case SOAP_OK: printf("Server version: %s\n", out.version); break;
	case SOAP_FAULT: 
	default: printf("???, err=%d\n", err);
    }

    soap_done(&soap);

    return 0;
}
