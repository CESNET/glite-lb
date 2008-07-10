#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <stdio.h>


#include "glite/lbu/db.h"
#include "glite/lb/context-int.h"

extern int 	debug;	// declared and set in bkserver.c


int edg_wll_SetErrorDB(edg_wll_Context ctx) {
	int code;
	char *ed;

	if (ctx->dbctx) {
		code = glite_lbu_DBError(ctx->dbctx, NULL, &ed);
		if (code == EDEADLOCK) code = EDG_WLL_ERROR_DB_TRANS_DEADLOCK;
		edg_wll_SetError(ctx, code, ed);
		free(ed);
	} else {
		code = EINVAL;
		edg_wll_SetError(ctx, EINVAL, "DB context isn't created");
	}

	return code;
}


int edg_wll_ExecSQL(edg_wll_Context ctx, const char *cmd, glite_lbu_Statement *stmt) {
	int retval;

	if ((retval = glite_lbu_ExecSQL(ctx->dbctx, cmd, stmt)) < 0) edg_wll_SetErrorDB(ctx);
	return retval;
}


int edg_wll_FetchRow(edg_wll_Context ctx, glite_lbu_Statement stmt, unsigned int n, unsigned long *lengths, char **results) {
	int retval;

	if ((retval = glite_lbu_FetchRow(stmt, n, lengths, results)) < 0) edg_wll_SetErrorDB(ctx);
	return retval;
}

int edg_wll_bufferedInsertInit(edg_wll_Context ctx, glite_lbu_bufInsert *bi, const char *table_name, long size_limit, long record_limit, const char *columns) {
	int retval;

	if ((retval = glite_lbu_bufferedInsertInit(ctx->dbctx, bi, table_name, size_limit, record_limit, columns)) != 0) edg_wll_SetErrorDB(ctx);
	return retval;
}

int edg_wll_bufferedInsert(edg_wll_Context ctx, glite_lbu_bufInsert bi, const char *row) {
	int retval;

	if ((retval = glite_lbu_bufferedInsert(bi, row)) != 0) edg_wll_SetErrorDB(ctx);
	return retval;
}

int edg_wll_bufferedInsertClose(edg_wll_Context ctx, glite_lbu_bufInsert bi) {
	int retval;

	if ((retval = glite_lbu_bufferedInsertClose(bi)) != 0) edg_wll_SetErrorDB(ctx);
	return retval;
}

int edg_wll_Transaction(edg_wll_Context ctx) {
	int retval;

	if ((retval = glite_lbu_Transaction(ctx->dbctx)) != 0) edg_wll_SetErrorDB(ctx);

// printf("edg_wll_Transaction(%d)\n", retval);
	return retval;
}

int edg_wll_Commit(edg_wll_Context ctx) {
	int retval;

	if ((retval = glite_lbu_Commit(ctx->dbctx)) != 0) edg_wll_SetErrorDB(ctx);
// printf("edg_wll_Commit(%d)\n", retval);
	return retval;
}

int edg_wll_Rollback(edg_wll_Context ctx) {
	int retval;

	if ((retval = glite_lbu_Rollback(ctx->dbctx)) != 0) edg_wll_SetErrorDB(ctx);
// printf("edg_wll_Rollback(%d)\n", retval);
	return retval;
}

int edg_wll_TransNeedRetry(edg_wll_Context ctx) {
	int ret;
	char *errd;

	ret = edg_wll_Error(ctx,NULL,NULL);
	if (ret == EDG_WLL_ERROR_DB_TRANS_DEADLOCK) {
		if (debug)
			printf("[%d]: DB deadlock detected. Rolling back transaction and retrying... \n",getpid());
		else 
			syslog(LOG_INFO,"[%d]: DB deadlock detected. Rolling back transaction and retrying... \n",getpid());

		edg_wll_Rollback(ctx);
		edg_wll_ResetError(ctx);
		return 1;
	} else if (ret==0) {
		edg_wll_Commit(ctx); /* errors propagated further */
		return 0;
	} else {
		edg_wll_Error(ctx, NULL, &errd);
		edg_wll_Rollback(ctx);
		edg_wll_SetError(ctx, ret, errd);
		free(errd);
		return 0;
	}
}


