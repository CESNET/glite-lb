#include <getopt.h>
#include <stdsoap2.h>
#include <expat.h>

#include "soap_version.h"
#include "glite/security/glite_gsplugin.h"
#include "glite/security/glite_gscompat.h"
#include "glite/lb/consumer.h"
#include "glite/lb/xml_conversions.h"

#include "bk_ws_H.h"
#include "ws_typeref.h"
#include "ws_fault.h"

#if GSOAP_VERSION <= 20602
#define soap_call___lb__JobStatus soap_call___ns1__JobStatus
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

static void printstat(edg_wll_JobStat stat, int level);

int main(int argc,char** argv)
{
    edg_wll_Context						ctx;
    glite_gsplugin_Context				gsplugin_ctx;
    struct soap						   *mydlo = soap_new();
    struct _lbe__JobStatusResponse	out;
    struct _lbe__JobStatus		in;	
    int									opt, err;
	char							   *server = "http://localhost:9003/",
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
		
    edg_wll_InitContext(&ctx);
    soap_set_namespaces(mydlo, namespaces);
    glite_gsplugin_init_context(&gsplugin_ctx);

	if ( soap_register_plugin_arg(mydlo, glite_gsplugin, (void *)gsplugin_ctx) )
	{
		soap_print_fault(mydlo, stderr);
		return 1;
	}

    glite_gsplugin_set_udata(mydlo, ctx);

    in.jobid = soap_strdup(mydlo, jobid);
    in.flags = soap_malloc(mydlo, sizeof(*in.flags));
    edg_wll_JobStatFlagsToSoap(mydlo, 0, in.flags);

    switch (err = soap_call___lb__JobStatus(mydlo, server, "",&in,&out))
	{
	case SOAP_OK:
		{
		edg_wll_JobStat s;

		edg_wll_SoapToStatus(mydlo,out.stat,&s);
		printstat(s, 0);
		edg_wll_FreeStatus(&s);
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

    soap_end(mydlo);
    soap_done(mydlo);
    free(mydlo);
    glite_gsplugin_free_context(gsplugin_ctx);
    edg_wll_FreeContext(ctx);

    return 0;
}

static void printstat(edg_wll_JobStat stat, int level)
{
	char		*s, *j1,*j2, ind[10];
	int 		i;


	for (i=0; i < level; i++)
		ind[i]='\t';
	ind[i]='\0';
	
	s = edg_wll_StatToString(stat.state); 
/* print whole flat structure */
	printf("%sstate : %s\n", ind, s);
	printf("%sjobId : %s\n", ind, j1 = edg_wlc_JobIdUnparse(stat.jobId));
	printf("%sowner : %s\n", ind, stat.owner);
	switch (stat.jobtype) {
		case EDG_WLL_STAT_SIMPLE:
			printf("%sjobtype : SIMPLE\n", ind);
			break;
		case EDG_WLL_STAT_DAG:
			printf("%sjobtype : DAG\n", ind);
                        break;
		case EDG_WLL_STAT_COLLECTION:
			printf("%sjobtype : COLLECTION\n", ind);
                        break;
		case EDG_WLL_STAT_PBS:
			printf("%sjobtype : PBS\n", ind);
                        break;
		default:
			break;
	}
	printf("%sparent_job : %s\n", ind,
			j2 = edg_wlc_JobIdUnparse(stat.parent_job));
	if (stat.jobtype) {;
		printf("%sseed : %s\n", ind, stat.seed);
		printf("%schildren_num : %d\n", ind, stat.children_num);
		printf("%schildren :\n", ind);
		if (stat.children) 
			for  (i=0; stat.children[i]; i++) 
				printf("%s\tchildren : %s\n", ind, stat.children[i]);
		printf("%schildren_states :\n", ind);
		if (stat.children_states)
		 	for  (i=0; stat.children_states[i].state; i++)
		 		printstat(stat.children_states[i], level+1);
		printf("%schildren_hist :\n",ind);
		if (stat.children_hist) 
			for (i=1; i<=stat.children_hist[0]; i++) 
				printf("%s%14s  %d\n", ind, edg_wll_StatToString(i-1),stat.children_hist[i]);
	}
	printf("%scondorId : %s\n", ind, stat.condorId);
	printf("%sglobusId : %s\n", ind, stat.globusId);
	printf("%slocalId : %s\n", ind, stat.localId);
	printf("%sjdl : %s\n", ind, stat.jdl);
	printf("%smatched_jdl : %s\n", ind, stat.matched_jdl);
	printf("%sdestination : %s\n", ind, stat.destination);
	printf("%snetwork server : %s\n", ind, stat.network_server);
	printf("%scondor_jdl : %s\n", ind, stat.condor_jdl);
	printf("%srsl : %s\n", ind, stat.rsl);
	printf("%sreason : %s\n", ind, stat.reason);
	printf("%slocation : %s\n", ind, stat.location);
	printf("%sce_node : %s\n", ind, stat.ce_node);
	printf("%ssubjob_failed : %d\n", ind, stat.subjob_failed);
	printf("%sdone_code : %s\n", ind, edg_wll_done_codeToString(stat.done_code));
	printf("%sexit_code : %d\n", ind, stat.exit_code);
	printf("%sresubmitted : %d\n", ind, stat.resubmitted);
	printf("%scancelling : %d\n", ind, stat.cancelling);
	printf("%scancelReason : %s\n", ind, stat.cancelReason);
	printf("%scpuTime : %d\n", ind, stat.cpuTime);
	printf("%suser_tags :\n",ind);
	if (stat.user_tags) 
		for (i=0; stat.user_tags[i].tag; i++) printf("%s%14s = \"%s\"\n", ind, 
						      stat.user_tags[i].tag,stat.user_tags[i].value);
	printf("%sstateEnterTime : %ld.%06ld\n", ind, stat.stateEnterTime.tv_sec,stat.stateEnterTime.tv_usec);
	printf("%sstateEnterTimes : \n",ind);
	if (stat.stateEnterTimes)  
                for (i=1; i<=stat.stateEnterTimes[0]; i++) {
			time_t	st = stat.stateEnterTimes[i];

			printf("%s%14s  %s", ind, edg_wll_StatToString(i-1), st == 0 ? 
			"    - not available -\n" : ctime(&st));
		}
	printf("%slastUpdateTime : %ld.%06ld\n", ind, stat.lastUpdateTime.tv_sec,stat.lastUpdateTime.tv_usec);
	printf("%sexpectUpdate : %d\n", ind, stat.expectUpdate);
	printf("%sexpectFrom : %s\n", ind, stat.expectFrom);
	printf("%sacl : %s\n", ind, stat.acl);
	printf("%spayload_running: %d\n", ind, stat.payload_running);
	if (stat.possible_destinations) {
		printf("%spossible_destinations : \n", ind);
		for (i=0; stat.possible_destinations[i]; i++) 
			printf("%s\t%s \n", ind, stat.possible_destinations[i]);
	}
	if (stat.possible_ce_nodes) {
		printf("%spossible_ce_nodes : \n", ind);
		for (i=0; stat.possible_ce_nodes[i]; i++) 
			printf("%s\t%s \n", ind, stat.possible_ce_nodes[i]);
	}
	/* PBS state section */
	if (stat.jobtype == EDG_WLL_STAT_PBS) {
		printf("%spbs_state : %s\n", ind, stat.pbs_state);
		printf("%spbs_queue : %s\n", ind, stat.pbs_queue);
		printf("%spbs_owner : %s\n", ind, stat.pbs_owner);
		printf("%spbs_name : %s\n", ind, stat.pbs_name);
		printf("%spbs_reason : %s\n", ind, stat.pbs_reason);
		printf("%spbs_scheduler : %s\n", ind, stat.pbs_scheduler);
		printf("%spbs_dest_host : %s\n", ind, stat.pbs_dest_host);
		printf("%spbs_pid : %d\n", ind, stat.pbs_pid);
		printf("%spbs_resource_usage : %s%s\n", ind,
			(stat.pbs_resource_usage) ? "\n" : "", stat.pbs_resource_usage);
		printf("%spbs_exit_status : %d\n", ind, stat.pbs_exit_status);
		printf("%spbs_error_desc : %s%s\n", ind, 
			(stat.pbs_error_desc) ? "\n" : "", stat.pbs_error_desc);
	}

	printf("\n");	
	
	free(j1);
	free(j2);
	free(s);
}

