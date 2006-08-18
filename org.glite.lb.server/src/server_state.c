#ident "$Header$"

#include "glite/lb-utils/db.h"
#include "glite/lb/trio.h"
#include "glite/lb/context-int.h"

#include "db_supp.h"
#include "server_state.h"

int edg_wll_GetServerState(edg_wll_Context ctx,const char *name,char **val)
{
	char	*stmt = NULL;
	glite_lbu_Statement q = NULL;

	*val = NULL;
	trio_asprintf(&stmt,"select value from server_state "
			"where prefix = 'https://%|Ss:%d' and name = '%|Ss'",
			ctx->srvName,ctx->srvPort,name);

	switch (glite_lbu_ExecSQL(ctx->dbctx,stmt,&q)) {
		case 0: edg_wll_SetError(ctx,ENOENT,name); break;
		case -1: edg_wll_SetErrorDB(ctx); break;
		default: glite_lbu_FetchRow(q,1,NULL,val); break;
	}

	glite_lbu_FreeStmt(&q);
	free(stmt);
	return edg_wll_Error(ctx, NULL, NULL);
}

int edg_wll_SetServerState(edg_wll_Context ctx,const char *name,const char *val)
{
	char	*stmt = NULL;

	trio_asprintf(&stmt,"insert into server_state (prefix,name,value) "
			"values ('https://%|Ss:%d','%|Ss','%|Ss')",
			ctx->srvName,ctx->srvPort,name,val);

	switch(glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL)) {
		case 1: break;
		case -1: if (edg_wll_Error(ctx,NULL,NULL) == EEXIST) {
				 free(stmt);
				 trio_asprintf(&stmt,"update server_state set value = '%|Ss' "
						 "where prefix = 'https://%|Ss:%d' "
						 "and name = '%|Ss'",
						 val,ctx->srvName,ctx->srvPort,name);
				 glite_lbu_ExecSQL(ctx->dbctx,stmt,NULL);
			 }
			 break;

		default: abort();
	}
	free(stmt);
	return edg_wll_SetErrorDB(ctx);
}
