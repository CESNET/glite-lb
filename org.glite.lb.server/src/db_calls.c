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

clean:
        glite_lbu_FreeStmt(&q);

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

