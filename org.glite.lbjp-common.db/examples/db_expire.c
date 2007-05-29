/*
 * Example (and quick test) of prepared statements expirations.
 * Use 'SET GLOBAL wait_timeout=...' for experimenting.
 *
 * Requires existing database with appropriate access and example table:
 *
 *   mysqladmin -u root -p create test
 *   mysql -u root -p -e 'GRANT ALL on test.* to testuser@localhost'
 *   ./db_test
 *
 * Use CS environment variable when using different user/pwd@machine:dbname.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"

#define CS "testuser/@localhost:test"
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
	char *name, *user;
	const char *cs;
	glite_lbu_DBContext ctx;
	glite_lbu_Statement stmt;
	int caps, i, nr, c;
	unsigned long lens[3];
	char *res[3];

	if ((name = strrchr(argv[0], '/')) != NULL) name++;
	else name = argv[0];
	if ((cs = getenv("CS")) == NULL) cs = CS;

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
	glite_lbu_DBSetCaps(ctx, caps);
	dprintf(("capabilities: %d\n", caps));

	user = NULL;
	dprintf(("preparing '%s'...\n", user));
	if ((glite_lbu_PrepareStmt(ctx, SELECT_CMD, &stmt)) != 0) goto failcon;

	do {
		user = "cicomexocitl.civ";
		dprintf(("executing '%s'...\n", user));
		if (glite_lbu_ExecStmt(stmt, 1, GLITE_LBU_DB_TYPE_VARCHAR, user) == -1) goto failstmt;
		dprintf(("fetching '%s'...\n", user));
		while ((nr = glite_lbu_FetchRow(stmt, 3, lens, res)) > 0) {
			dprintf(("Result: n=%d, res=%p\n", nr, res));
			print_free_result(name, lens, res);
		}
		if (nr < 0) {
			dprintf(("fetch '%s' failed\n", user));
			break;
		}
		dprintf(("\n"));

		c = fgetc(stdin);
	} while (c != -1 && (c == '\r' || c == '\n'));

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
	dprintf(("failed\n"));
	return 1;
}
