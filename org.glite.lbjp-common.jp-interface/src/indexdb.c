#ident "$Header:"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include <glite/jobid/strmd5.h>
#include <glite/lbu/db.h>
#include <glite/lbu/trio.h>

#define GLITE_JP_INDEX_COMPILE 1

#include "indexdb.h"

char *glite_jp_indexdb_attr2id(const char *name) {
	size_t i, len;
	char *lname, *id;

	len = strlen(name);
	lname = malloc(len + 1);
	for (i = 0; i < len + 1; i++) lname[i] = tolower(name[i]);
	id = str2md5(lname);
	free(lname);

	return id;
}


int glite_jp_indexdb_attr2select_from_index(const char *name, int indexed __attribute__((unused)), char **column, char **full_value_column, char **table, char **where) {
	char *id;

	if (column) *column = strdup("value");
	if (full_value_column) *full_value_column = strdup("full_value");
	if (table) {
		id = glite_jp_indexdb_attr2id(name);
		asprintf(table, GLITE_JP_INDEXDB_TABLE_ATTR_PREFIX "%s", id);
		free(id);
	}
	if (where) *where = strdup("");

	return 0;
}


int glite_jp_indexdb_attr2select_from_db(const char *name, glite_lbu_DBContext dbctx, char **column, char **full_value_column, char **table, char **where) {
	char *sql, *id;
	glite_lbu_Statement stmt;
	int ret;

	if (table) {
		trio_asprintf(&sql, "SELECT attrid FROM attrs WHERE name='%|Ss'", name);
		ret = glite_lbu_ExecSQL(dbctx, sql, &stmt);
		free(sql);
		switch (ret) {
		case -1: return glite_lbu_DBError(dbctx, NULL, NULL);
		case 1: break;
		default: return EINVAL;
		}
		if (glite_lbu_FetchRow(stmt, 1, NULL, &id) < 0) return glite_lbu_DBError(dbctx, NULL, NULL);
		asprintf(table, GLITE_JP_INDEXDB_TABLE_ATTR_PREFIX "%s", id);
		free(id);
		glite_lbu_FreeStmt(&stmt);
	}
	if (column) *column = strdup("value");
	if (full_value_column) *full_value_column = strdup("full_value");
	if (where) *where = strdup("");

	return 0;
}
