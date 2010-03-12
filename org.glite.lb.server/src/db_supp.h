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
