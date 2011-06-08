#ident "$Header$"
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


#include <errno.h>

#include "glite/lbu/trio.h"
#include "glite/lb/context-int.h"
#include "glite/lbu/log.h"

#include "server_state.h"
#include "db_supp.h"

int edg_wll_GetServerState(edg_wll_Context ctx,const char *name,char **val)
{
	char	*stmt = NULL;
	glite_lbu_Statement	q = NULL;


	trio_asprintf(&stmt,"select value from server_state "
			"where prefix = 'https://%|Ss:%d' and name = '%|Ss'",
			ctx->srvName,ctx->srvPort,name);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	switch (edg_wll_ExecSQL(ctx,stmt,&q)) {
		case 0: edg_wll_SetError(ctx,ENOENT,name); break;
		case -1: break;
		default: edg_wll_FetchRow(ctx,q,sizeof(val)/sizeof(val[0]),NULL,val); break;
	}

	glite_lbu_FreeStmt(&q);
	free(stmt);
	return edg_wll_Error(ctx,NULL,NULL);
}

int edg_wll_SetServerState(edg_wll_Context ctx,const char *name,const char *val)
{
	char	*stmt = NULL;
	int sql_retval;

	// Check if record exists
	trio_asprintf(&stmt,"select value from server_state "
			"where prefix = 'https://%|Ss:%d' and name = '%|Ss'",
			ctx->srvName,ctx->srvPort,name);
	glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);

	sql_retval = edg_wll_ExecSQL(ctx,stmt,NULL);

	free(stmt);

	if (!sql_retval) {
		trio_asprintf(&stmt,"insert into server_state (prefix,name,value) "
				"values ('https://%|Ss:%d','%|Ss','%|Ss')",
				ctx->srvName,ctx->srvPort,name,val);
		glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, LOG_PRIORITY_DEBUG, stmt);
		edg_wll_ExecSQL(ctx,stmt,NULL);
		free(stmt);
	}
	else {
		if (sql_retval > 0) {
			trio_asprintf(&stmt,"update server_state set value = '%|Ss' "
					 "where prefix = 'https://%|Ss:%d' "
					 "and name = '%|Ss'",
					 val,ctx->srvName,ctx->srvPort,name);
			glite_common_log_msg(LOG_CATEGORY_LB_SERVER_DB, 
					LOG_PRIORITY_DEBUG, stmt);
			edg_wll_ExecSQL(ctx,stmt,NULL);
			free(stmt);
		}
		else abort();
	 }

	return edg_wll_Error(ctx,NULL,NULL);
}
