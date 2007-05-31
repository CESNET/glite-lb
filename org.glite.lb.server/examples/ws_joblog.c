#include <getopt.h>
#include <stdsoap2.h>

#include "glite/security/glite_gsplugin.h"
#include "glite/lb/consumer.h"
#include "glite/lb/events_parse.h"

#include "bk_ws_H.h"
#include "ws_typeref.h"
#include "ws_fault.h"

#include "soap_version.h"

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

static void free_events(edg_wll_Event *events);

int main(int argc,char** argv)
{
	edg_wll_Context				ctx;
	glite_gsplugin_Context			gsplugin_ctx;
	struct soap				*mydlo = soap_new();
	struct _lbe__QueryEventsResponse	out;
	struct _lbe__QueryEvents		in;	
	edg_wll_QueryRec			**jconds = NULL,
						**econds = NULL;
	edg_wll_QueryRec			j[2], e[1];
	int					opt, err, i;
	edg_wlc_JobId				job;
	char					*server = "http://localhost:9003/",
						*jobid = NULL,
						*name = NULL;


	name = strrchr(argv[0],'/');
	if (name) name++; else name = argv[0];

	while ((opt = getopt_long(argc, argv, "hm:j:", opts, NULL)) != EOF) switch (opt)
	{
	case 'h': usage(name); return 0;
	case 'm': server = strdup(optarg); break;
	case 'j': jobid = strdup(optarg); break;
	case '?': usage(name); return 1;
	}

	if ( !jobid )
	{
		printf("jobid should be given\n");
		usage(name);
		return 1;
	}
	else if (edg_wlc_JobIdParse(jobid,&job)) {
                fprintf(stderr,"%s: can't parse job ID\n",argv[1]);
                return 1;
        }

		
	edg_wll_InitContext(&ctx);
    	glite_gsplugin_init_context(&gsplugin_ctx);

	soap_set_namespaces(mydlo, namespaces);
	if ( soap_register_plugin_arg(mydlo, glite_gsplugin, (void *)gsplugin_ctx) )
	{
		soap_print_fault(mydlo, stderr);
		return 1;
	}

	glite_gsplugin_set_udata(mydlo, ctx);


	/* prepare job log quary */
	memset(j,0,sizeof j);
	memset(e,0,sizeof e);

	j[0].attr = EDG_WLL_QUERY_ATTR_JOBID;
	j[0].op = EDG_WLL_QUERY_OP_EQUAL;
	j[0].value.j = job;

	
	jconds = (edg_wll_QueryRec **) calloc(2, sizeof(edg_wll_QueryRec *));
	for ( i = 0; i < 2; i++ )
	{
		jconds[i] = (edg_wll_QueryRec *) calloc(2, sizeof(edg_wll_QueryRec));
		jconds[i][0] = j[i];
	}

	econds = (edg_wll_QueryRec **) calloc(1, sizeof(edg_wll_QueryRec *));
	for ( i = 0; i < 1; i++ )
	{
		econds[i] = (edg_wll_QueryRec *) calloc(1, sizeof(edg_wll_QueryRec));
		econds[i][0] = e[i];
	}


	if (edg_wll_QueryCondsExtToSoap(mydlo, (const edg_wll_QueryRec **)jconds,
			&in.__sizejobConditions, &in.jobConditions) != SOAP_OK) {
		printf("Error converting QueryConds to Soap!\n");
		return(1);
	}

	//edg_wll_QueryCondsExtToSoap(mydlo, (const edg_wll_QueryRec **)econds,
	//	&in.__sizeeventConditions, &in.eventConditions);

	//in.jobConditions = NULL;
	//in.__sizejobConditions = 0;
	in.eventConditions = NULL;
	in.__sizeeventConditions = 0;

	switch (err = soap_call___lb__QueryEvents(mydlo, server, "",&in,&out))
	{
	case SOAP_OK:
		{
		edg_wll_Event 	*events = NULL;
		int		i;


		edg_wll_SoapToEventsQueryRes(mydlo,out,&events);

		for ( i = 0; events && events[i].type != EDG_WLL_EVENT_UNDEF; i++ )
		{
			char	*e = edg_wll_UnparseEvent(ctx,events+i);
			fputs(e,stdout);
			fputs("\n",stdout);
			free(e);
		}

		free_events(events);
		printf("\nFound %d events\n",i);
		}
		break;
	case SOAP_FAULT: 
	case SOAP_SVR_FAULT:
		{
		char	*et,*ed;

		edg_wll_FaultToErr(mydlo,ctx);
		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"%s: %s (%s)\n",argv[0],et,ed);
		exit(1);
		}
	default:
		fprintf(stderr,"err = %d\n",err);
		soap_print_fault(mydlo,stderr);
	}

	soap_dealloc(mydlo, out.events);
	return 0;
}


static void free_events(edg_wll_Event *events)
{
	int	i;

	if (events) {
		for (i=0; events[i].type != EDG_WLL_EVENT_UNDEF; i++) 
			edg_wll_FreeEvent(&(events[i]));
		edg_wll_FreeEvent(&(events[i])); /* free last line */
		free(events);	
		events = NULL;
	}
}
