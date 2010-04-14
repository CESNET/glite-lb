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

#include <getopt.h>
#include <stdsoap2.h>

#include "soap_version.h"
#include "glite/security/glite_gsplugin.h"
#include "glite/security/glite_gscompat.h"

#include "bk_ws_H.h"
#include "ws_fault.h"

#if GSOAP_VERSION <= 20602
#define soap_call___lb__QueryEvents soap_call___ns1__QueryEvents
#endif

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
	glite_gsplugin_Context			gsplugin_ctx;
	struct soap				*mydlo = soap_new();
	struct _lbe__QueryEventsResponse	out,*outp = &out;
	struct _lbe__QueryEvents		in;	
	int					opt, err;
	char					*server = "http://localhost:9003/",
						*jobid = NULL,
						*name = NULL;


	struct lbt__queryConditions	*qcp;
	struct lbt__queryRecord		*qrp;
	struct lbt__queryRecValue	qv;

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
		
    	glite_gsplugin_init_context(&gsplugin_ctx);

	soap_set_namespaces(mydlo, namespaces);
	if ( soap_register_plugin_arg(mydlo, glite_gsplugin, (void *)gsplugin_ctx) )
	{
		soap_print_fault(mydlo, stderr);
		return 1;
	}

	GLITE_SECURITY_GSOAP_LIST_CREATE(mydlo, &in, jobConditions, struct lbt__queryConditions, 1);
	qcp = GLITE_SECURITY_GSOAP_LIST_GET(in.jobConditions, 0);
	in.eventConditions = NULL;
	in.__sizeeventConditions = 0;

	memset(qcp,0,sizeof(*qcp));
	qcp->attr = lbt__queryAttr__JOBID;
	GLITE_SECURITY_GSOAP_LIST_CREATE(mydlo, qcp, record, struct lbt__queryRecord, 1);
	qrp = GLITE_SECURITY_GSOAP_LIST_GET(qcp->record, 0);

	memset(qrp,0,sizeof(*qrp));
	qrp->op = lbt__queryOp__EQUAL;
	qrp->value1 = &qv;

	GLITE_SECURITY_GSOAP_CHOICE_SET(&qv,c,lbt,queryRecValue,2,jobid);

//	qv.__union_2 = SOAP_UNION_lbt__union_2_c;
//	qv.union_2.c = jobid;

	switch (err = soap_call___lb__QueryEvents(mydlo, server, "",&in,&out))
	{
	case SOAP_OK:
	{
		struct soap	*outsoap = soap_new();
		outsoap->sendfd = 1;

		soap_serialize_PointerTo_lbe__QueryEventsResponse(outsoap,&outp);
		soap_begin_send(outsoap);
		soap_put_PointerTo_lbe__QueryEventsResponse(outsoap,&outp,"joblog","http://glite.org/wsdl/elements/lb:QueryJobsResponse");

		break;
	}
	case SOAP_FAULT: 
	case SOAP_SVR_FAULT:
		{
		char	*et;
		int	err;

		err = glite_lb_FaultToErr(mydlo,&et);
		fprintf(stderr,"%s: %s (%s)\n",argv[0],strerror(err),et);
		soap_done(mydlo);
		exit(1);
		}
	default:
		fprintf(stderr,"err = %d\n",err);
		soap_print_fault(mydlo,stderr);
	}

	soap_end(mydlo/*, out.events*/);
	soap_done(mydlo);
	free(mydlo);
	glite_gsplugin_free_context(gsplugin_ctx);
	return 0;
}
