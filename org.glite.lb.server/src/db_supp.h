#ifndef EDG_WLL_DB_SUPP_H
#define EDG_WLL_DB_SUPP_H

#ident "$Header:"

#include "glite/lbu/db.h"

#define DEFAULTCS "lbserver/@localhost:lbserver20"

/**
 * Set the current database error.
 */
int edg_wll_SetErrorDB(edg_wll_Context ctx);

int edg_wll_ExecSQL(edg_wll_Context ctx, const char *cmd, glite_lbu_Statement *stmt);
int edg_wll_FetchRow(edg_wll_Context ctx, glite_lbu_Statement stmt, unsigned int n, unsigned long *lengths, char **results);

int edg_wll_bufferedInsertInit(edg_wll_Context ctx, glite_lbu_bufInsert *bi, const char *table_name, long size_limit, long record_limit, const char *columns);
int edg_wll_bufferedInsert(edg_wll_Context ctx, glite_lbu_bufInsert bi, const char *row);
int edg_wll_bufferedInsertClose(edg_wll_Context ctx, glite_lbu_bufInsert bi);

int edg_wll_Transaction(edg_wll_Context ctx);
int edg_wll_Commit(edg_wll_Context ctx);
int edg_wll_Rollback(edg_wll_Context ctx);
int edg_wll_TransNeedRetry(edg_wll_Context ctx);

#endif
