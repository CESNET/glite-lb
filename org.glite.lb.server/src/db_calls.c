#ident "$Header: "

#include <string.h>
#include <errno.h>

#include "glite/jobid/cjobid.h"
#include "glite/lbu/trio.h"

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
        ret = edg_wll_ExecSQL(ctx,stmt,&q);
        if (ret <= 0) {
                if (ret == 0) {
                        fprintf(stderr,"%s: no such job\n",dbjob);
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
		fprintf(stderr,"Error retrieving proxy&server fields of jobs table. Missing column?\n");
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
int edg_wll_LockJobRow(edg_wll_Context ctx, glite_jobid_const_t job, int lock_mode) 
{
	char			*jobid_md5 = NULL;
	char			*stmt = NULL;
	glite_lbu_Statement 	sh;
	int			nr;


	edg_wll_ResetError(ctx);
	jobid_md5 = edg_wlc_JobIdGetUnique(job);

	if (lock_mode) 
		trio_asprintf(&stmt, "select count(*) from jobs where jobid='%|Ss' for update", jobid_md5);
	else
		trio_asprintf(&stmt, "select count(*) from jobs where jobid='%|Ss' lock in share mode", jobid_md5);

	if ((nr = edg_wll_ExecSQL(ctx,stmt,&sh)) < 0) goto cleanup;
	if (nr == 0) {
                edg_wll_SetError(ctx,ENOENT,"no state in DB");
                goto cleanup;
	}
	
cleanup:
	if (sh) glite_lbu_FreeStmt(&sh);
	free(stmt); stmt = NULL;
	free(jobid_md5);

	return edg_wll_Error(ctx, NULL, NULL);
}

