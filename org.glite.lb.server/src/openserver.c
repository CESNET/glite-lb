#ident "$Header$"

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "glite/lb/context-int.h"
#include "openserver.h"
#include "db_supp.h"

edg_wll_ErrorCode edg_wll_Open(edg_wll_Context ctx, char *cs)
{
	int ret, hit = 0, i;
	char *table[1];
	char *cols[20];
	glite_lbu_Statement stmt;

	if (glite_lbu_InitDBContext(&ctx->dbctx) != 0) {
		char *ed;

		glite_lbu_DBError(ctx->dbctx, NULL, &ed);
		edg_wll_SetError(ctx, EDG_WLL_ERROR_DB_INIT, ed);
		free(ed);
		return EDG_WLL_ERROR_DB_INIT;
	}
	if (glite_lbu_DBConnect(ctx->dbctx,cs) != 0) return edg_wll_SetErrorDB(ctx);

	// proxy and server columns added
	if (glite_lbu_ExecSQL(ctx->dbctx, "DESC jobs", &stmt) <= 0) goto err;
	hit = 0;
	while (hit < 2 && (ret = glite_lbu_FetchRow(stmt, 1, NULL, cols)) > 0) {
		assert(ret <= (int)(sizeof cols/sizeof cols[0]));
		if (strcasecmp(cols[0], "proxy") == 0 || 
		    strcasecmp(cols[0], "server") == 0) hit++;
		for (i = 0; i < ret; i++) free(cols[i]);
	}
	if (ret < 0) goto err;
	if (hit != 2) {
		ret = edg_wll_SetError(ctx, EINVAL, "old DB schema found, migration to new schema needed");
		goto close_db;
	}

	// events_flesh table added
	if (glite_lbu_ExecSQL(ctx->dbctx, "SHOW TABLES", &stmt) <= 0) goto err;
	hit = 0;
	while (hit < 1 && (ret = glite_lbu_FetchRow(stmt, 1, NULL, table)) > 0) {
		if (strcasecmp(table[0], "events_flesh") == 0) hit++;
		free(table[0]);
	}
	if (ret < 0) goto err;
	if (hit != 1) {
		ret = edg_wll_SetError(ctx, EINVAL, "events_flesh table not found, migration to new schema needed");
		goto close_db;
	}

	return 0;

err:
	edg_wll_SetErrorDB(ctx);
close_db:
	glite_lbu_DBClose(ctx->dbctx);
	return ret;
}

edg_wll_ErrorCode edg_wll_Close(edg_wll_Context ctx)
{
	glite_lbu_DBClose(ctx->dbctx);
	glite_lbu_FreeDBContext(ctx->dbctx);
	ctx->dbctx = NULL;
	return edg_wll_ResetError(ctx);
}
