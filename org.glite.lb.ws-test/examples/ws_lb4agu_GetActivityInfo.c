#include <getopt.h>
#include <stdsoap2.h>

#include "soap_version.h"
#include "glite/security/glite_gsplugin.h"
#include "glite/security/glite_gscompat.h"

#include "bk_ws_H.h"

#include "LoggingAndBookkeeping.nsmap"

static struct option opts[] = {
	{"help",	0,	NULL,	'h'},
	{"server",	1,	NULL,	'm'},
	{"jobid",	1,	NULL,	'j'}
};

static void usage(char *me)
{
	fprintf(stderr,"usage: %s [option]\n"
		"\t-h, --help      Shows this screen.\n"
		"\t-m, --server    BK server address:port.\n"
		"\t-j, --jobid     ID of requested job.\n"
		, me);
}

int main(int argc,char** argv)
{
	glite_gsplugin_Context				gsplugin_ctx;
	struct soap					*mydlo = soap_new();
	struct _lb4ague__GetActivityInfoRequest	*in;	
	struct _lb4ague__GetActivityInfoResponse	*out;
	int						opt, err;
	char						*server = "http://localhost:9003/",
							*jobid = NULL,
							*name = NULL;


	name = strrchr(argv[0],'/');
	if (name) name++; else name = argv[0];

	while ((opt = getopt_long(argc, argv, "hm:j:", opts, NULL)) != EOF) switch (opt)
	{
	case 'h': usage(name); return 0;
	case 'm': server = optarg; break;
	case 'j': jobid = optarg; break;
	case '?': usage(name); return 1;
	}

	if ( !jobid )
	{
		printf("jobid should be given\n");
		usage(name);
		return 1;
	}
		
	soap_set_namespaces(mydlo, namespaces);
	glite_gsplugin_init_context(&gsplugin_ctx);

	if ( soap_register_plugin_arg(mydlo, glite_gsplugin, (void *)gsplugin_ctx) )
	{
		soap_print_fault(mydlo, stderr);
		return 1;
	}


	in = soap_malloc(mydlo, sizeof(*in));
	out = soap_malloc(mydlo, sizeof(*out));
	in->id[0] = soap_strdup(mydlo, jobid);
	in->__sizeid = 1;

	switch (err = soap_call___lb4agu__GetActivityInfo(mydlo, server, "",in,out)) {
	case SOAP_OK:
		{
		struct soap	*outsoap = soap_new();;

		outsoap->sendfd = 1;
		soap_serialize_PointerTo_lb4ague__GetActivityInfoResponse(outsoap,&out);
		soap_begin_send(outsoap);
		soap_put_PointerTo_lb4ague__GetActivityInfoResponse(outsoap,&out,"status","http://glite.org/wsdl/services/lb4agu:GetActivityInfoResponse");
		soap_end_send(outsoap);
		}
		break;
	case SOAP_FAULT: 
	case SOAP_SVR_FAULT:
		{
		char	*et;
		int err;

		err = glite_lb_FaultToErr(mydlo,&et);
		fprintf(stderr,"%s: %s (%s)\n",argv[0],strerror(err),et);
		exit(1);
		}
	default:
		fprintf(stderr,"err = %d\n",err);
		soap_print_fault(mydlo,stderr);
    }

    soap_end(mydlo);
    soap_done(mydlo);
    free(mydlo);
    glite_gsplugin_free_context(gsplugin_ctx);

    return 0;
}
