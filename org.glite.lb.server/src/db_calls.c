#ident "$Header: "

#include <string.h>
#include <errno.h>

#include "glite/jobid/cjobid.h"
#include "glite/lbu/trio.h"
#include "glite/lbu/log.h"

#include "glite/lb/context-int.h"

#include "db_calls.h"
#include "db_supp.h"

/** Returns bitmask of job membership in common server/proxy database 
 */
int edg_wll_jobMembership(edg_wll_Context ctx, glite_jobid_const_t job)
{
        char            *dbjob;
        char            *stmt = NULL;
        glite_lbu_Statement q;
        int             ret, result = -1;
        char            *res[2] = { NULL, NULL};

        edg_wll_ResetError(ctx);

        dbjob = edg_wlc_JobIdGetUnique(job);

        trio_asprintf(&stmt,"select proxy,server from jobs where jobid = '%|Ss' for update",dbjob);
	glite_common_log(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
        ret = edg_wll_ExecSQL(ctx,stmt,&q);
        if (ret <= 0) {
                if (ret == 0) {
			glite_common_log(LOG_CATEGORY_CONTROL, 
				LOG_PRIORITY_WARN, "%s: no such job",dbjob);
                        edg_wll_SetError(ctx,ENOENT,dbjob);
                }
                goto clean;
        }
        free(stmt); stmt = NULL;

        if ((ret = edg_wll_FetchRow(ctx,q,sizeof(res)/sizeof(res[0]),NULL,res)) > 0) {
		result = 0;
                if (strcmp(res[0],"0")) result += DB_PROXY_JOB;
                if (strcmp(res[1],"0")) result += DB_SERVER_JOB;
        }
        else {
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, "Error retrieving proxy&server fields of jobs table. Missing column?");
                edg_wll_SetError(ctx,ENOENT,dbjob);
        }
        glite_lbu_FreeStmt(&q);

clean:
	free(res[0]); free(res[1]);
        free(dbjob);
        free(stmt);
        return(result);
}


/* just lock one row corresponding to job in table jobs
 * lock_mode: 0 = lock in share mode / 1 = for update
 */
int edg_wll_LockJobRow(edg_wll_Context ctx, const char *job, int lock_mode) 
{
	char			*stmt = NULL;
	glite_lbu_Statement 	sh;
	int			nr;


	edg_wll_ResetError(ctx);

	if (lock_mode) 
		trio_asprintf(&stmt, "select * from jobs where jobid='%|Ss' for update", job);
	else
		trio_asprintf(&stmt, "select * from jobs where jobid='%|Ss' lock in share mode", job);

	glite_common_log(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
	if ((nr = edg_wll_ExecSQL(ctx,stmt,&sh)) < 0) goto cleanup;
	if (nr == 0) {
		char *err;

		asprintf(&err,"jobid='%s' not registered in DB", job);
                edg_wll_SetError(ctx,ENOENT, err);
		free(err);
                goto cleanup;
	}
	
cleanup:
	if (sh) glite_lbu_FreeStmt(&sh);
	free(stmt); stmt = NULL;

	return edg_wll_Error(ctx, NULL, NULL);
}

