#ident "$Header: "

#include <string.h>
#include <errno.h>

#include "glite/jobid/cjobid.h"
#include "glite/lbu/trio.h"

#include "glite/lb/context-int.h"

#include "lbs_db.h"
#include "db_calls.h"

/** Returns bitmask of job membership in common server/proxy database 
 */
int edg_wll_jobMembership(edg_wll_Context ctx, edg_wlc_JobId job)
{
        char            *dbjob;
        char            *stmt = NULL;
        edg_wll_Stmt    q;
        int             ret, result = -1;
        char            *res[2] = { NULL, NULL};

        edg_wll_ResetError(ctx);

        dbjob = edg_wlc_JobIdGetUnique(job);

        trio_asprintf(&stmt,"select proxy,server from jobs where jobid = '%|Ss'",dbjob);
        ret = edg_wll_ExecStmt(ctx,stmt,&q);
        if (ret <= 0) {
                if (ret == 0) {
                        fprintf(stderr,"%s: no such job\n",dbjob);
                        edg_wll_SetError(ctx,ENOENT,dbjob);
                }
                goto clean;
        }
        free(stmt); stmt = NULL;

        if ((ret = edg_wll_FetchRow(q,res)) > 0) {
		result = 0;
                if (strcmp(res[0],"0")) result += DB_PROXY_JOB;
                if (strcmp(res[1],"0")) result += DB_SERVER_JOB;
        }
        else {
                if (ret == 0) result = 0;
                else {
                        fprintf(stderr,"Error retrieving proxy&server fields of jobs table. Missing column?\n");
                        edg_wll_SetError(ctx,ENOENT,dbjob);
                }
        }
        edg_wll_FreeStmt(&q);

clean:
	free(res[0]); free(res[1]);
        free(dbjob);
        free(stmt);
        return(result);
}
