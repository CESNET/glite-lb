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


#include <assert.h>
#include <errno.h>
#include <string.h>

#include "glite/lb/context-int.h"
#include "glite/lbu/log.h"
#include "openserver.h"
#include "db_supp.h"

edg_wll_ErrorCode edg_wll_Open(edg_wll_Context ctx, char *cs)
{
	int ret, hit = 0, i;
	char *table[1];
	char *cols[20];
	glite_lbu_Statement stmt;

	if (!ctx->dbctx && (ret = glite_lbu_InitDBContext((glite_lbu_DBContext*) &ctx->dbctx, GLITE_LBU_DB_BACKEND_MYSQL, LOG_CATEGORY_LB_SERVER_DB)) != 0) {
		char *ed;

		if (ctx->dbctx) {
			glite_lbu_DBError(ctx->dbctx, NULL, &ed);
			edg_wll_SetError(ctx, EDG_WLL_ERROR_DB_INIT, ed);
			free(ed);
		} else
			edg_wll_SetError(ctx, EDG_WLL_ERROR_DB_INIT, strerror(ret));

		return EDG_WLL_ERROR_DB_INIT;
	}
	if (glite_lbu_DBConnect(ctx->dbctx,cs) != 0) return edg_wll_SetErrorDB(ctx);

	hit = 0;
	// new columns added to jobs
	if ((ret = glite_lbu_ExecSQL(ctx->dbctx, "DESC jobs", &stmt)) <= 0) goto err;
	while (hit < 4 && (ret = glite_lbu_FetchRow(stmt, 1, NULL, cols)) > 0) {
		assert(ret <= (int)(sizeof cols/sizeof cols[0]));
		if (strcasecmp(cols[0], "proxy") == 0 ||
		    strcasecmp(cols[0], "server") == 0 ||
		    strcasecmp(cols[0], "grey") == 0 ||
		    strcasecmp(cols[0], "nevents") == 0) hit++;
		for (i = 0; i < ret; i++) free(cols[i]);
	}
	if (ret < 0) goto err;
	glite_lbu_FreeStmt(&stmt);
	// new columns added to events
	if (glite_lbu_ExecSQL(ctx->dbctx, "DESC events", &stmt) <= 0) goto err;
	while (hit < 5 && (ret = glite_lbu_FetchRow(stmt, 1, NULL, cols)) > 0) {
		assert(ret <= (int)(sizeof cols/sizeof cols[0]));
		if (strcasecmp(cols[0], "seqcode") == 0) hit++;
		for (i = 0; i < ret; i++) free(cols[i]);
	}
	if (ret < 0) goto err;
	glite_lbu_FreeStmt(&stmt);
	// new columns added to notif_registrations
	if (glite_lbu_ExecSQL(ctx->dbctx, "DESC notif_registrations", &stmt) <= 0) goto err;
	while (hit < 6 && (ret = glite_lbu_FetchRow(stmt, 1, NULL, cols)) > 0) {
		assert(ret <= (int)(sizeof cols/sizeof cols[0]));
		if (strcasecmp(cols[0], "flags") == 0) hit++;
		for (i = 0; i < ret; i++) free(cols[i]);
	}
	if (ret < 0) goto err;
	glite_lbu_FreeStmt(&stmt);

	if (hit != 6) {
		ret = edg_wll_SetError(ctx, EDG_WLL_ERROR_DB_INIT, "old DB schema found, migration to new schema needed");
		goto close_db;
	}

	// events_flesh table added
	if (glite_lbu_ExecSQL(ctx->dbctx, "SHOW TABLES", &stmt) <= 0) goto err;
	hit = 0;
	while (hit < 3 && (ret = glite_lbu_FetchRow(stmt, 1, NULL, table)) > 0) {
		if (strcasecmp(table[0], "events_flesh") == 0 ||
		strcasecmp(table[0], "zombie_jobs") == 0 ||
		strcasecmp(table[0], "zombie_suffixes") == 0 ||
		strcasecmp(table[0], "zombie_prefixes") == 0) hit++;
		free(table[0]);
	}
	if (ret < 0) goto err;
	glite_lbu_FreeStmt(&stmt);
	if (hit != 3) {
		ret = edg_wll_SetError(ctx, EDG_WLL_ERROR_DB_INIT, "events_flesh or zombie_jobs or zombie_prefixes or zombie_suffixes table not found, migration to new schema needed");
		goto close_db;
	}

	return 0;

err:
	edg_wll_SetErrorDB(ctx);
	glite_lbu_FreeStmt(&stmt);
close_db:
	glite_lbu_DBClose(ctx->dbctx);
	return ret;
}

edg_wll_ErrorCode edg_wll_Close(edg_wll_Context ctx)
{
	if (ctx->dbctx) {
		glite_lbu_DBClose(ctx->dbctx);
		glite_lbu_FreeDBContext(ctx->dbctx);
		ctx->dbctx = NULL;
	}
	return edg_wll_ResetError(ctx);
}
