#include <getopt.h>
#include <stdsoap2.h>

#include "glite/security/glite_gsplugin.h"
#include "glite/lb/consumer.h"

#include "bk_ws_H.h"
#include "ws_fault.h"
#include "ws_typeref.h"

#include "soap_version.h"

#if GSOAP_VERSION <= 20602
#define soap_call___lb__QueryJobs soap_call___ns1__QueryJobs
#endif

#include "LoggingAndBookkeeping.nsmap"


static struct option opts[] = {
	{"help",	0,	NULL,	'h'},
	{"server",	1,	NULL,	'm'}
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
	edg_wll_Context				ctx;
	glite_gsplugin_Context			gsplugin_ctx;
	edg_wll_QueryRec			**conditions = NULL;
	struct soap				*soap = soap_new();
	struct _lbe__QueryJobs			*qjobs = NULL;
	struct _lbe__QueryJobsResponse		out;
	int					opt, err;
	char					*server = "http://localhost:9003/",
					 	*name = NULL;
	int					i, j;

	name = strrchr(argv[0],'/');
	if (name) name++; else name = argv[0];

	while ((opt = getopt_long(argc, argv, "hm:", opts, NULL)) != EOF) switch (opt)
	{
	case 'h': usage(name); return 0;
	case 'm': server = optarg; break;
	case '?': usage(name); return 1;
	}

	edg_wll_InitContext(&ctx);
	glite_gsplugin_init_context(&gsplugin_ctx);
	soap_set_namespaces(soap, namespaces);
	if ( soap_register_plugin_arg(soap, glite_gsplugin, (void *)gsplugin_ctx) )
	{
		soap_print_fault(soap, stderr);
		return 1;
	}

	glite_gsplugin_set_udata(soap, ctx);

	conditions = (edg_wll_QueryRec **)calloc(3,sizeof(edg_wll_QueryRec *));

	conditions[0] = (edg_wll_QueryRec *)calloc(2,sizeof(edg_wll_QueryRec));
	conditions[0][0].attr = EDG_WLL_QUERY_ATTR_STATUS;
	conditions[0][0].op = EDG_WLL_QUERY_OP_UNEQUAL;
	conditions[0][0].value.i = EDG_WLL_JOB_DONE;

	conditions[1] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	conditions[1][0].attr = EDG_WLL_QUERY_ATTR_OWNER;
	conditions[1][0].op = EDG_WLL_QUERY_OP_EQUAL;
	conditions[1][0].value.c = NULL;

	qjobs = soap_malloc(soap, sizeof(*qjobs));
	memset(qjobs, 0, sizeof(*qjobs));
	qjobs->flags = soap_malloc(soap, sizeof(*qjobs->flags));
	memset(qjobs->flags, 0, sizeof(*qjobs->flags));
	if (!qjobs->flags || edg_wll_QueryCondsExtToSoap(soap, (const edg_wll_QueryRec **)conditions, &qjobs->__sizeconditions, &qjobs->conditions)
		|| edg_wll_JobStatFlagsToSoap(soap, 0, qjobs->flags) ) {
		char	*et,*ed;

		fprintf(stderr, "%s: soap types conversion error...\n", argv[0]);
		edg_wll_FaultToErr(soap, ctx);
		edg_wll_Error(ctx, &et, &ed);
		fprintf(stderr, "%s: %s (%s)\n", argv[0], et, ed);
		exit(1);
	}

	for (i = 0; conditions[i]; i++) free(conditions[i]);
	free(conditions);

    err = soap_call___lb__QueryJobs(soap, server, "", qjobs, &out);
    switch ( err ) {
	case SOAP_OK: {
		int		i;

		printf("Query succesfull...\n");
		printf("%-65s%s\n\n", "jobid", "state");
		for ( i = 0; i < out.__sizejobs; i++ ) {
			edg_wll_JobStatCode statCode;


			edg_wll_SoapToJobStatCode(out.states[i]->state, &statCode);
			char *s = edg_wll_StatToString(statCode);
			printf("%-65s%s\n", out.jobs[i], s);
			free(s);
		}
		}
		break;
	case SOAP_FAULT: 
	case SOAP_SVR_FAULT: {
		char	*et,*ed;

		edg_wll_FaultToErr(soap, ctx);
		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"%s: %s (%s)\n",argv[0],et,ed);
		exit(1);
		}
	default:
		fprintf(stderr,"err = %d\n",err);
		soap_print_fault(soap,stderr);
    }

    soap_end(soap);
    soap_done(soap);
    free(soap);
    glite_gsplugin_free_context(gsplugin_ctx);
    edg_wll_FreeContext(ctx);

    return 0;
}

static void printstat(edg_wll_JobStat stat, int level)
{
    char        *s, *j, ind[10];
    int         i;


    for (i=0; i < level; i++)
        ind[i]='\t';
    ind[i]='\0';

    s = edg_wll_StatToString(stat.state);
/* print whole flat structure */
    printf("%sstate : %s\n", ind, s);
    printf("%sjobId : %s\n", ind, j = edg_wlc_JobIdUnparse(stat.jobId));
    printf("%sowner : %s\n", ind, stat.owner);
    printf("%sjobtype : %s\n", ind, (stat.jobtype ? "DAG" : "SIMPLE") );
    printf("%sparent_job : %s\n", ind,
            j = edg_wlc_JobIdUnparse(stat.parent_job));
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
	printf("%ssubjob_failed : %d\n", ind, stat.subjob_failed);
    printf("%sdone_code : %d\n", ind, stat.done_code);
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
                for (i=1; i<=stat.stateEnterTimes[0]; i++)
            printf("%s%14s  %s", ind, edg_wll_StatToString(i-1), (stat.stateEnterTimes[i] == 0) ?
            "    - not available -\n" : ctime((time_t *) &stat.stateEnterTimes[i]));
    printf("%slastUpdateTime : %ld.%06ld\n", ind, stat.lastUpdateTime.tv_sec,stat.lastUpdateTime.tv_usec);
	printf("%sexpectUpdate : %d\n", ind, stat.expectUpdate);
    printf("%sexpectFrom : %s\n", ind, stat.expectFrom);
    printf("%sacl : %s\n", ind, stat.acl);
    printf("\n");

    free(j);
    free(s);
}
