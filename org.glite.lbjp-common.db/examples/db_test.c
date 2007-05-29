/*
 * Example (and quick test) of this DB module.
 *
 * Requires existing database with appropriate access:
 *
 *   mysqladmin -u root -p create test
 *   mysql -u root -p -e 'GRANT ALL on test.* to testuser@localhost'
 *
 * Use CS environment variable when using different user/pwd@machine:dbname.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"

#define CS "testuser/@localhost:test"
#define CREATE_CMD "CREATE TABLE data (\n\
    id    INT NOT NULL,\n\
    user  VARCHAR(32) NOT NULL,\n\
    info  BLOB,\n\
    PRIMARY KEY (id),\n\
    INDEX(user)\n\
) engine=innodb"
#define DROP_CMD "DROP TABLE data"
#define INSERT_TRIO_CMD "INSERT INTO data (id, user, info) VALUES (%d, %s, %s)"
#define SELECT_TRIO_CMD "SELECT id, user, info FROM data WHERE user = '%s'"
#define INSERT_CMD "INSERT INTO data (id, user, info) VALUES (?, ?, ?)"
#define SELECT_CMD "SELECT id, user, info FROM data WHERE user = ?"

#define dprintf(ARGS) { printf("%s: ", name); printf ARGS; }


static void print_blob(unsigned long len, char *blob) {
	int i;
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


int main(int argn, char *argv[]) {
	char *name, *cmd;
	const char *cs;
	glite_lbu_DBContext ctx;
	glite_lbu_Statement stmt;
	int caps;

	char blob1[] = "Guess: blob or \000string?";
	char blob2[] = {0, 1, 2, 3, 4, 5};

	int nr;	
	char *res[3];
	unsigned long lens[3];

	if ((name = strrchr(argv[0], '/')) != NULL) name++;
	else name = argv[0];
	if ((cs = getenv("CS")) == NULL) cs = CS;
	cmd = NULL;

	// init
	dprintf(("connecting to %s...\n", cs));
	if (glite_lbu_InitDBContext(&ctx) != 0) goto fail;
	if (glite_lbu_DBConnect(ctx, cs) != 0) goto failctx;
	if ((caps = glite_lbu_DBQueryCaps(ctx)) == -1) goto failcon;
	if ((caps & GLITE_LBU_DB_CAP_PREPARED) == 0) {
		dprintf(("can't do prepared commands, exiting."));
		goto failcon;
	}
	// caps
	glite_lbu_DBSetCaps(ctx, caps || GLITE_LBU_DB_CAP_ERRORS);
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
	// prepared-insert
	dprintf(("prepare-insert...\n"));
	if (glite_lbu_PrepareStmt(ctx, INSERT_CMD, &stmt) != 0) goto failcon;
	dprintf(("execute 1. insert...\n"));
	if (glite_lbu_ExecStmt(stmt, 3,
	                       GLITE_LBU_DB_TYPE_INT, 2,
	                       GLITE_LBU_DB_TYPE_VARCHAR, "cicomexocitl.civ",
	                       GLITE_LBU_DB_TYPE_BLOB, blob1, sizeof(blob1) - 1) != 1) goto failstmt;
	dprintf(("execute 2. insert...\n"));
	if (glite_lbu_ExecStmt(stmt, 3,
	                       GLITE_LBU_DB_TYPE_INT, 3,
	                       GLITE_LBU_DB_TYPE_VARCHAR, "tartarus",
	                       GLITE_LBU_DB_TYPE_NULL) != 1) goto failstmt;
	dprintf(("execute 3. insert...\n"));
	if (glite_lbu_ExecStmt(stmt, 3,
	                       GLITE_LBU_DB_TYPE_INT, 4,
	                       GLITE_LBU_DB_TYPE_VARCHAR, "harpia",
	                       GLITE_LBU_DB_TYPE_BLOB, blob2, sizeof(blob2)) != 1) goto failstmt;
	glite_lbu_FreeStmt(&stmt);
	dprintf(("\n"));

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
}

	// "param" queries
{
	const char *user = NULL;

	dprintf(("preparing '%s'...\n", user));
	if ((glite_lbu_PrepareStmt(ctx, SELECT_CMD, &stmt)) != 0) goto failcon;

	user = "cicomexocitl.civ";
	dprintf(("executing '%s'...\n", user));
	if (glite_lbu_ExecStmt(stmt, 1, GLITE_LBU_DB_TYPE_VARCHAR, user) == -1) goto failstmt;
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

	dprintf(("closing...\n"));
	glite_lbu_DBClose(ctx);
	glite_lbu_FreeDBContext(ctx);
	return 0;

failstmt:
	printf("closing stmt...\n");
	glite_lbu_FreeStmt(&stmt);
failcon:
	dprintf(("closing...\n"));
	glite_lbu_DBClose(ctx);
failctx:
	glite_lbu_FreeDBContext(ctx);
fail:
	free(cmd);
	dprintf(("failed\n"));
	return 1;
}
