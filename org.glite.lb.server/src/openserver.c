#ident "$Header$"

#include "glite/lb/context-int.h"
#include "openserver.h"
#include "db_supp.h"

edg_wll_ErrorCode edg_wll_Open(edg_wll_Context ctx, char *cs)
{
	if (glite_lbu_InitDBContext(&ctx->dbctx) != 0) {
		char *ed;

		glite_lbu_DBError(ctx->dbctx, NULL, &ed);
		edg_wll_SetError(ctx, EDG_WLL_ERROR_DB_INIT, ed);
		free(ed);
		return EDG_WLL_ERROR_DB_INIT;
	}
	return glite_lbu_DBConnect(ctx->dbctx,cs) ? edg_wll_SetErrorDB(ctx) : 0;
}

edg_wll_ErrorCode edg_wll_Close(edg_wll_Context ctx)
{
	glite_lbu_DBClose(ctx->dbctx);
	glite_lbu_FreeDBContext(ctx->dbctx);
	ctx->dbctx = NULL;
	return edg_wll_ResetError(ctx);
}
