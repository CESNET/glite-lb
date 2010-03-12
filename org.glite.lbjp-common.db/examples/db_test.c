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

/*
 * Example (and quick test) of this DB module.
 *
 * Requires existing database with appropriate access:
 *
 *   mysqladmin -u root -p create test
 *   mysql -u root -p -e 'GRANT ALL on test.* to testuser@localhost'
 *
 * Or postgres:
 *
 *   createuser -U postgres testuser
 *   createdb -U postgres --owner testuser test
 *
 * Use CS environment variable when using different user/pwd@machine:dbname.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glite/lbu/trio.h>

#include "db.h"

#define CS "testuser/@localhost:test"

#ifdef PSQL_BACKEND
#define CREATE_CMD "CREATE TABLE \"data\" (\n\
    \"id\"    INTEGER NOT NULL,\n\
    \"user\"  VARCHAR(32) NOT NULL,\n\
    \"info\"  BYTEA,\n\
    PRIMARY KEY (\"id\")\n\
)"
#define AMP "\""
#define INSERT_CMD "INSERT INTO " AMP "data" AMP " (" AMP "id" AMP ", " AMP "user" AMP ", " AMP "info" AMP ") VALUES ($1, $2, $3)"
#define SELECT_CMD "SELECT " AMP "id" AMP ", " AMP "user" AMP ", " AMP "info" AMP " FROM " AMP "data" AMP " WHERE " AMP "user" AMP " = $1"
#define DB_TEST_BACKEND GLITE_LBU_DB_BACKEND_PSQL

#else

#define CREATE_CMD "CREATE TABLE data (\n\
    `id`    INT NOT NULL,\n\
    `user`  VARCHAR(32) NOT NULL,\n\
    `info`  BLOB,\n\
    PRIMARY KEY (id),\n\
    INDEX(`user`)\n\
) engine=innodb"
#define AMP "`"
#define INSERT_CMD "INSERT INTO " AMP "data" AMP " (" AMP "id" AMP ", " AMP "user" AMP ", " AMP "info" AMP ") VALUES (?, ?, ?)"
#define SELECT_CMD "SELECT " AMP "id" AMP ", " AMP "user" AMP ", " AMP "info" AMP " FROM " AMP "data" AMP " WHERE " AMP "user" AMP " = ?"
#define DB_TEST_BACKEND GLITE_LBU_DB_BACKEND_MYSQL
#endif

#define DROP_CMD "DROP TABLE " AMP "data" AMP
#define INSERT_TRIO_CMD "INSERT INTO " AMP "data" AMP " (" AMP "id" AMP ", " AMP "user" AMP ", " AMP "info" AMP ") VALUES (%d, %s, %s)"
#define SELECT_TRIO_CMD "SELECT " AMP "id" AMP ", " AMP "user" AMP ", " AMP "info" AMP " FROM " AMP "data" AMP " WHERE " AMP "user" AMP " = '%s'"

#define dprintf(ARGS) { printf("%s: ", name); printf ARGS; }


static void print_blob(unsigned long len, char *blob) {
	unsigned int i;

	for (i = 0; i < len; i++) printf("%02X ", blob[i]);
	printf("(='");
	for (i = 0; i < len; i++) printf("%c", blob[i]);
	printf("')");
}


static void print_free_result(const char *name, unsigned long *lens, char **res) {
	dprintf(("  id='%s'=%d\n", res[0], atoi(res[0])));

	dprintf(("  user='%s'\n", res[1]));

	dprintf(("  blob="));
	if (res[2] && lens) print_blob(lens[2], res[2]);
	else printf("null");
	printf("\n");

	free(res[0]);
	free(res[1]);
	free(res[2]);
}


int main(int argn __attribute((unused)), char *argv[]) {
	char *name, *cmd;
	const char *cs;
	glite_lbu_DBContext ctx;
	glite_lbu_Statement stmt;
	int caps;

#ifndef NO_PREPARED
	char blob1[] = "Guess: blob or _string?"; blob1[15] = 0;
	char blob2[] = {0, 1, 2, 3, 4, 5};
#endif

	int nr;	
	char *res[3];
	unsigned long lens[3];

	if ((name = strrchr(argv[0], '/')) != NULL) name++;
	else name = argv[0];
	if ((cs = getenv("CS")) == NULL) cs = CS;
	cmd = NULL;

	// init
	dprintf(("connecting to %s...\n", cs));
	if (glite_lbu_InitDBContext(&ctx, DB_TEST_BACKEND) != 0) goto failctx;
	if (glite_lbu_DBConnect(ctx, cs) != 0) goto failctx;
	if ((caps = glite_lbu_DBQueryCaps(ctx)) == -1) goto failcon;
#ifndef NO_PREPARED
	if ((caps & GLITE_LBU_DB_CAP_PREPARED) == 0) {
		dprintf(("can't do prepared commands, exiting."));
		goto failcon;
	}
#endif
	// caps
	glite_lbu_DBSetCaps(ctx, caps | GLITE_LBU_DB_CAP_ERRORS);
	dprintf(("capabilities: %d\n", caps));
	// create all needed tables and data
	dprintf(("creating tables...\n"));
	glite_lbu_ExecSQL(ctx, DROP_CMD, NULL);
	if (glite_lbu_ExecSQL(ctx, CREATE_CMD, NULL) == -1) goto failcon;
	// trio-insert
	dprintf(("trio-insert...\n"));
	asprintf(&cmd, INSERT_TRIO_CMD, 1, "'hyperochus'", "NULL");
	if (glite_lbu_ExecSQL(ctx, cmd, NULL) != 1) goto failcon;
	free(cmd); cmd = NULL;
#ifndef NO_PREPARED
	// prepared-insert
	dprintf(("prepare-insert...\n"));
	if (glite_lbu_PrepareStmt(ctx, INSERT_CMD, &stmt) != 0) goto failcon;
	dprintf(("execute 1. insert...\n"));
	if (glite_lbu_ExecPreparedStmt(stmt, 3,
	                       GLITE_LBU_DB_TYPE_INT, 2,
	                       GLITE_LBU_DB_TYPE_VARCHAR, "cicomexocitl.civ",
	                       GLITE_LBU_DB_TYPE_BLOB, blob1, sizeof(blob1) - 1) != 1) goto failstmt;
	dprintf(("execute 2. insert...\n"));
	if (glite_lbu_ExecPreparedStmt(stmt, 3,
	                       GLITE_LBU_DB_TYPE_INT, 3,
	                       GLITE_LBU_DB_TYPE_VARCHAR, "tartarus",
	                       GLITE_LBU_DB_TYPE_NULL) != 1) goto failstmt;
	dprintf(("execute 3. insert...\n"));
	if (glite_lbu_ExecPreparedStmt(stmt, 3,
	                       GLITE_LBU_DB_TYPE_INT, 4,
	                       GLITE_LBU_DB_TYPE_VARCHAR, "harpia",
	                       GLITE_LBU_DB_TYPE_BLOB, blob2, sizeof(blob2)) != 1) goto failstmt;
	glite_lbu_FreeStmt(&stmt);
	dprintf(("\n"));
#endif

	// trio-query
{
	const char *user;

	user = "harpia";
	dprintf(("selecting '%s'...\n", user));
	asprintf(&cmd, SELECT_TRIO_CMD, user);
	if (glite_lbu_ExecSQL(ctx, cmd, &stmt) == -1) goto failcon;
	free(cmd); cmd = NULL;
	dprintf(("fetching '%s'...\n", user));
	while ((nr = glite_lbu_FetchRow(stmt, 3, lens, res)) > 0) {
		dprintf(("Result: n=%d, res=%p\n", nr, res));
		print_free_result(name, lens, res);
	}
	if (nr < 0) dprintf(("fetch '%s' failed\n", user));
	dprintf(("closing stmt...\n"));
	glite_lbu_FreeStmt(&stmt);
	dprintf(("\n"));

	user = "hyperochus";
	dprintf(("selecting '%s'...\n", user));
	asprintf(&cmd, SELECT_TRIO_CMD, user);
	if (glite_lbu_ExecSQL(ctx, cmd, &stmt) == -1) goto failcon;
	free(cmd); cmd = NULL;
	dprintf(("fetching '%s'...\n", user));
	while ((nr = glite_lbu_FetchRow(stmt, 3, lens, res)) > 0) {
		dprintf(("Result: n=%d, res=%p\n", nr, res));
		print_free_result(name, lens, res);
	}
	if (nr < 0) dprintf(("fetch '%s' failed\n", user));
	dprintf(("closing stmt...\n"));
	glite_lbu_FreeStmt(&stmt);
	dprintf(("\n"));

	user = "nobody";
	dprintf(("selecting '%s'...\n", user));
	asprintf(&cmd, SELECT_TRIO_CMD, user);
	if (glite_lbu_ExecSQL(ctx, cmd, &stmt) == -1) goto failcon;
	free(cmd); cmd = NULL;
	dprintf(("fetching '%s'...\n", user));
	while ((nr = glite_lbu_FetchRow(stmt, 3, lens, res)) > 0) {
		dprintf(("Result: n=%d, res=%p\n", nr, res));
		print_free_result(name, lens, res);
	}
	if (nr < 0) dprintf(("fetch '%s' failed\n", user));
	dprintf(("closing stmt...\n"));
	glite_lbu_FreeStmt(&stmt);
	dprintf(("\n"));

	dprintf(("nonsese...\n"));
	if (glite_lbu_ExecSQL(ctx, "nonsense", NULL) == -1) {
		dprintf(("was error, OK\n"));
	} else {
		dprintf(("this should file\n"));
		goto failcon;
	}
}

#ifndef NO_PREPARED
	// "param" queries
{
	const char *user = NULL;

	dprintf(("preparing '%s'...\n", user));
	if ((glite_lbu_PrepareStmt(ctx, SELECT_CMD, &stmt)) != 0) goto failcon;

	user = "cicomexocitl.civ";
	dprintf(("executing '%s'...\n", user));
	if (glite_lbu_ExecPreparedStmt(stmt, 1, GLITE_LBU_DB_TYPE_VARCHAR, user) == -1) goto failstmt;
	dprintf(("fetching '%s'...\n", user));
	while ((nr = glite_lbu_FetchRow(stmt, 3, lens, res)) > 0) {
		dprintf(("Result: n=%d, res=%p\n", nr, res));
		print_free_result(name, lens, res);
	}
	if (nr < 0) dprintf(("fetch '%s' failed\n", user));
	dprintf(("\n"));

	dprintf(("closing stmt...\n"));
	glite_lbu_FreeStmt(&stmt);
	dprintf(("\n"));
}
#endif

	dprintf(("closing...\n"));
	glite_lbu_DBClose(ctx);
	glite_lbu_FreeDBContext(ctx);
	return 0;

#ifndef NO_PREPARED
failstmt:
	dprintf(("closing stmt...\n"));
	glite_lbu_FreeStmt(&stmt);
#endif
failcon:
	dprintf(("closing...\n"));
	glite_lbu_DBClose(ctx);
failctx:
	if (ctx) {
		char *t, *d;

		glite_lbu_DBError(ctx, &t, &d);
		printf("Error %s: %s\n", t, d);
		free(t); free(d);
	}
	glite_lbu_FreeDBContext(ctx);
	free(cmd);
	dprintf(("failed\n"));
	return 1;
}
