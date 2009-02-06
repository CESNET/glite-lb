#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/*headers*/
#include "glite/security/glite_gss.h"
#include "glite/lb/context.h"
#include "glite/lb/notification.h"
/*end headers*/

static struct option opts[] = {
	{"help",		0,	NULL,	'h'},
	{"user",		1,	NULL,	'u'},
        {"timeout",		1,	NULL,	't'},
};

static void usage(char *me)
{
	fprintf(stderr, "usage: %s [option]\n"
			"\t-h, --help      Shows this screen.\n"
			"\t-u, --user      User DN.\n"
		        "\t-t, --timeout   Timeout for receiving.\n"
		        "GLITE_WMS_NOTIF_SERVER must be set.\n"
			, me);
}


int main(int argc, char *argv[])
{
        char			        *user;
	int				i, opt, err = 0;
	time_t                          valid;
	struct timeval                  timeout = {220, 0};

	user = NULL;
	while ( (opt = getopt_long(argc, argv, "h:u:t:", opts, NULL)) != EOF)
		switch (opt) {
		case 'h': usage(argv[0]); return 0;
		case 'u': user = strdup(optarg); break;
		case 't': timeout.tv_sec = atoi(optarg); break;
		case '?': usage(argv[0]); return 1;
		}

	/*variables*/
	edg_wll_Context		ctx;
	edg_wll_QueryRec	**conditions;
	edg_wll_NotifId         notif_id = NULL, recv_notif_id = NULL;
	edg_wll_JobStat         stat;
	/*end variables*/

	/*context*/
	edg_wll_InitContext(&ctx);
	/*end context*/

	/* set server:port if don't want to depend on GLITE_WMS_NOTIF_SERVER */
	/* edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER, server); */
	/* if (port) edg_wll_SetParam(ctx, EDG_WLL_PARAM_QUERY_SERVER_PORT, port); */
	
	conditions = (edg_wll_QueryRec **)calloc(2,sizeof(edg_wll_QueryRec *));
	conditions[0] = (edg_wll_QueryRec *)calloc(2,sizeof(edg_wll_QueryRec));


	/* set notification conditions to Owner=xxx */
	/*queryrec*/
	conditions[0][0].attr = EDG_WLL_QUERY_ATTR_OWNER;
	conditions[0][0].op = EDG_WLL_QUERY_OP_EQUAL;
	conditions[0][0].value.c = user;
	/*end queryrec*/

	/*register*/
	if (edg_wll_NotifNew(ctx, (edg_wll_QueryRec const* const*)conditions, 
           -1, NULL, &notif_id, &valid)) {
	        char    *et,*ed;

                edg_wll_Error(ctx,&et,&ed);
                fprintf(stderr,"%s: edg_wll_NotifNew(): %s (%s)\n",argv[0],et,ed);

                free(et); free(ed);
	        goto register_err;
	}
	fprintf(stdout,"Registration OK, notification ID: %s\nvalid: (%ld)\n",
		edg_wll_NotifIdUnparse(notif_id),
		valid);
	/*end register*/

	fprintf(stdout,"Waiting for a notification for %d seconds\n", timeout.tv_sec);

	/*receive*/
	if ( (err = edg_wll_NotifReceive(ctx, -1, &timeout, &stat, &recv_notif_id)) ) {
	        if (err != ETIMEDOUT) {
		     char    *et,*ed;

		     edg_wll_Error(ctx,&et,&ed);
		     fprintf(stderr,"%s: edg_wll_NotifReceive(): %s (%s)\n",argv[0],et,ed);

		     free(et); free(ed);
		     goto receive_err;
		}
		fprintf(stdout,"No job state change recived in given timeout\n");
	}
	else
	{
	        /* Check recv_notif_id if you have registered more notifications */
	        /* Print received state change */
	        printf("jobId : %s\n", edg_wlc_JobIdUnparse(stat.jobId));
		printf("state : %s\n\n", edg_wll_StatToString(stat.state));
		edg_wll_FreeStatus(&stat);
	}
	/*end receive*/

receive_err:

	/* Drop registration if not used anymore edg_wll_NotifDrop() */

	edg_wll_NotifIdFree(recv_notif_id);
        edg_wll_NotifCloseFd(ctx);
        /* edg_wll_NotifClosePool(ctx); */

register_err:
	
	edg_wll_FreeContext(ctx);

	return err;
}
