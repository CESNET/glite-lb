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

/**
 * Simple postgres module with org.glite.lbjp-common.db interface.
 * 
 * PostgreSQL limitations:
 *  - prepared commands requires server >= 8.2
 *  - binary data need to be handled manually (libpq limitation)
 */

#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libpq-fe.h>

#include "glite/lbu/trio.h"
#include "glite/lbu/log.h"
#include "db.h"
#include "db-int.h"

#ifdef WIN32
#define STDCALL __stdcall
#else
#define STDCALL
#endif

#define DB_CONNECT_TIMEOUT "20"

/*#ifdef LOG
  #define lprintf(FMT...) fprintf(stdout, "[db-pg] %s: ", __FUNCTION__); fprintf(stdout, ##FMT);
#else
  #define lprintf(FMT...)
#endif*/

#define set_error(CTX, CODE, DESC...) glite_lbu_DBSetError((glite_lbu_DBContext)(CTX), (CODE), __FUNCTION__, __LINE__, ##DESC)


struct glite_lbu_DBContextPsql_s {
	struct glite_lbu_DBContext_s generic;
	PGconn *conn;
	int prepared_counts[4];
};
typedef struct glite_lbu_DBContextPsql_s *glite_lbu_DBContextPsql;

struct glite_lbu_StatementPsql_s {
	glite_lbu_Statement_t generic;
	PGresult *res;
	int row, nrows;
	char *sql, *name;
};
typedef struct glite_lbu_StatementPsql_s *glite_lbu_StatementPsql;


/* backend module declaration */
int glite_lbu_InitDBContextPsql(glite_lbu_DBContext *ctx_gen);
void glite_lbu_FreeDBContextPsql(glite_lbu_DBContext ctx_gen);
int glite_lbu_DBConnectPsql(glite_lbu_DBContext ctx_gen, const char *cs);
void glite_lbu_DBClosePsql(glite_lbu_DBContext ctx_gen);
int glite_lbu_DBQueryCapsPsql(glite_lbu_DBContext ctx_gen);
void glite_lbu_DBSetCapsPsql(glite_lbu_DBContext commmon_ctx, int caps);
int glite_lbu_TransactionPsql(glite_lbu_DBContext ctx_gen);
int glite_lbu_CommitPsql(glite_lbu_DBContext ctx_gen);
int glite_lbu_RollbackPsql(glite_lbu_DBContext ctx_gen);
int glite_lbu_FetchRowPsql(glite_lbu_Statement stmt, unsigned int n, unsigned long *lengths, char **results);
void glite_lbu_FreeStmtPsql(glite_lbu_Statement *stmt);
//int glite_lbu_QueryIndicesPsql(glite_lbu_DBContext ctx_gen, const char *table, char ***key_names, char ****column_names);
int glite_lbu_ExecSQLPsql(glite_lbu_DBContext ctx_gen, const char *cmd, glite_lbu_Statement *stmt);
int glite_lbu_QueryColumnsPsql(glite_lbu_Statement stmt_gen, char **cols);
int glite_lbu_PrepareStmtPsql(glite_lbu_DBContext ctx_gen, const char *sql, glite_lbu_Statement *stmt_gen);
int glite_lbu_ExecPreparedStmtPsql_v(glite_lbu_Statement stmt_gen, int n, va_list ap);
//long int glite_lbu_LastidPsql(glite_lbu_Statement stmt_gen);

glite_lbu_DBBackend_t psql_backend = {
	backend: GLITE_LBU_DB_BACKEND_PSQL,

	initContext: glite_lbu_InitDBContextPsql,
	freeContext: glite_lbu_FreeDBContextPsql,
	connect: glite_lbu_DBConnectPsql,
	close: glite_lbu_DBClosePsql,
	queryCaps: glite_lbu_DBQueryCapsPsql,
	setCaps: glite_lbu_DBSetCapsPsql,
	transaction: glite_lbu_TransactionPsql,
	commit: glite_lbu_CommitPsql,
	rollback: glite_lbu_RollbackPsql,
	fetchRow: glite_lbu_FetchRowPsql,
	freeStmt: glite_lbu_FreeStmtPsql,
	queryIndices: NULL /*glite_lbu_QueryIndicesPsql*/,
	execSQL: glite_lbu_ExecSQLPsql,
	queryColumns: glite_lbu_QueryColumnsPsql,

	timeToDB: glite_lbu_TimeToStr,
	timestampToDB: glite_lbu_TimestampToStr,
	DBToTime: glite_lbu_StrToTime,
	DBToTimestamp: glite_lbu_StrToTimestamp,

	prepareStmt: glite_lbu_PrepareStmtPsql,
	execPreparedStmt_v: glite_lbu_ExecPreparedStmtPsql_v,
	lastid: NULL/*glite_lbu_LastidPsql*/,
};


/* nicer identifiers in PREPARE/EXECUTE commands */
static const char *prepared_names[4] = {"select", "update", "insert", "other"};


int glite_lbu_InitDBContextPsql(glite_lbu_DBContext *ctx_gen) {
	glite_lbu_DBContextPsql ctx;
	int err = 0;

	ctx = calloc(1, sizeof *ctx);
	if (!ctx) return ENOMEM;
	*ctx_gen = (glite_lbu_DBContext)ctx;

	return 0;
}


void glite_lbu_FreeDBContextPsql(glite_lbu_DBContext ctx_gen) {
	glite_lbu_DBContextPsql ctx = (glite_lbu_DBContextPsql)ctx_gen;

	if (ctx) {
		assert(ctx->conn == NULL);
		free(ctx);
	}
}


int glite_lbu_DBConnectPsql(glite_lbu_DBContext ctx_gen, const char *cs) {
	glite_lbu_DBContextPsql ctx = (glite_lbu_DBContextPsql)ctx_gen;
	char *buf, *slash, *at, *colon, *host, *user, *pw, *db, *pgcsbuf, *pgcs;
	char *err;

	host = user = pw = db = NULL;
	buf = strdup(cs);
	slash = strchr(buf,'/');
	at = strrchr(buf,'@');
	colon = strrchr(buf,':');
	if (!slash || !at || !colon) {
		free(buf);
		return set_error(ctx, EINVAL, "Invalid DB connect string");
	}
	*slash = *at = *colon = 0;
	host = at+1;
	user = buf;
	pw = slash+1;
	db = colon+1;

	trio_asprintf(&pgcsbuf, "host='%|Ss' dbname='%|Ss' user='%|Ss' password='%|Ss' connect_timeout=" DB_CONNECT_TIMEOUT, host, db, user, pw);
	/* simulate socket acces via localhost similar to MySQL */
	if (strcmp(host, "localhost") == 0) pgcs = pgcsbuf + strlen("host='localhost' ");
	else pgcs = pgcsbuf;
	free(buf);

	 glite_common_log(ctx_gen->log_category, LOG_PRIORITY_DEBUG, 
		"connection string = %s\n", pgcs);
	ctx->conn = PQconnectdb(pgcs);
	free(pgcsbuf);
	if (!ctx->conn) return ENOMEM;

	

	if (PQstatus(ctx->conn) != CONNECTION_OK) {
		asprintf(&err, "Can't connect, %s", PQerrorMessage(ctx->conn));
		PQfinish(ctx->conn);
		ctx->conn = NULL;
		set_error(ctx, EIO, err);
		free(err);
		return EIO;
	}

	return 0;
}


void glite_lbu_DBClosePsql(glite_lbu_DBContext ctx_gen) {
	glite_lbu_DBContextPsql ctx = (glite_lbu_DBContextPsql)ctx_gen;

	if (ctx->conn) {
		PQfinish(ctx->conn);
		ctx->conn = NULL;
	}
}


int glite_lbu_DBQueryCapsPsql(glite_lbu_DBContext ctx_gen) {
	glite_lbu_DBContextPsql ctx = (glite_lbu_DBContextPsql)ctx_gen;
	glite_lbu_Statement stmt;
	int major, minor, sub, version;
	int has_prepared = 0;
	char *res = NULL;

	glite_common_log(ctx_gen->log_category, LOG_PRIORITY_DEBUG, "SHOW server_version");
	if (glite_lbu_ExecSQLPsql(ctx_gen, "SHOW server_version", &stmt) == -1) return -1;
	switch (glite_lbu_FetchRowPsql(stmt, 1, NULL, &res)) {
	case 1:
		break;
	case -1:
		has_prepared = -1;
		goto quit;
	default:
		goto quit;
	}
	if (sscanf(res, "%d.%d.%d", &major, &minor, &sub) != 3) {
		set_error(ctx, EIO, "can't parse PostgreSQL version string");
		goto quit;
	}
	version = 10000*major + 100*minor + sub;
	has_prepared = version >= 80200 ? GLITE_LBU_DB_CAP_PREPARED : 0;

quit:
	free(res);
	glite_lbu_FreeStmtPsql(&stmt);
	return has_prepared;
}


void glite_lbu_DBSetCapsPsql(glite_lbu_DBContext ctx_gen, int caps) {
	ctx_gen->caps = caps;
}


int glite_lbu_TransactionPsql(glite_lbu_DBContext ctx_gen __attribute((unused))) {
	return 0;
}


int glite_lbu_CommitPsql(glite_lbu_DBContext ctx_gen __attribute((unused))) {
	return 0;
}


int glite_lbu_RollbackPsql(glite_lbu_DBContext ctx_gen __attribute((unused))) {
	return 0;
}


int glite_lbu_FetchRowPsql(glite_lbu_Statement stmt_gen, unsigned int maxn, unsigned long *lengths, char **results) {
	glite_lbu_StatementPsql stmt = (glite_lbu_StatementPsql)stmt_gen;
	unsigned int i, n;
	char *s;

	if (stmt->row >= stmt->nrows) return 0;

	n = PQnfields(stmt->res);
	if (n <= 0) {
		set_error(stmt->generic.ctx, EINVAL, "Result set w/o columns");
		return -1;
	}
	if (n > maxn) {
		set_error(stmt->generic.ctx, EINVAL, "Not enough room for the result");
		return -1;
	}
	for (i = 0; i < n; i++) {
		/* sanity check for internal error (NULL when invalid row) */
		s = PQgetvalue(stmt->res, stmt->row, i) ? : "";
		if ((results[i] = strdup(s)) == NULL)
			goto nomem;
		if (lengths) lengths[i] = strlen(s);
	}

	stmt->row++;
	return n;
nomem:
	for (n = i, i = 0; i < n; i++) {
		free(results[i]);
		results[i] = NULL;
	}
	set_error(stmt->generic.ctx, ENOMEM, "insufficient memory for field data");
	return -1;
}


void glite_lbu_FreeStmtPsql(glite_lbu_Statement *stmt_gen) {
	glite_lbu_DBContextPsql ctx;
	glite_lbu_StatementPsql stmt;
	char *sql;

	if (!*stmt_gen) return;
	stmt = (glite_lbu_StatementPsql)(*stmt_gen);
	ctx = (glite_lbu_DBContextPsql)stmt->generic.ctx;
	if (stmt->res) PQclear(stmt->res);
	if (stmt->name) {
		asprintf(&sql, "DEALLOCATE %s", stmt->name);
		glite_common_log_msg(ctx->generic.log_category, LOG_PRIORITY_DEBUG, sql);
		stmt->res = PQexec(ctx->conn, sql);
		free(sql);
		PQclear(stmt->res);
	}
	free(stmt->name);
	free(stmt->sql);
	free(stmt);
	*stmt_gen = NULL;
}


int glite_lbu_ExecSQLPsql(glite_lbu_DBContext ctx_gen, const char *cmd, glite_lbu_Statement *stmt_out) {
	glite_lbu_DBContextPsql ctx = (glite_lbu_DBContextPsql)ctx_gen;
	glite_lbu_StatementPsql stmt = NULL;
	int status, n;
	char *nstr, *errmsg, *pos;
	PGresult *res;

	//lprintf("command = %s\n", cmd);
	glite_common_log(ctx_gen->log_category, LOG_PRIORITY_DEBUG, "command = %s\n", cmd);
	if (stmt_out) *stmt_out = NULL;
	if ((res = PQexec(ctx->conn, cmd)) == NULL) {
		ctx->generic.err.code = ENOMEM;
		return -1;
	}

	status = PQresultStatus(res);
	if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
		errmsg = PQresultErrorMessage(res);
		if (errmsg) {
			errmsg = strdup(errmsg);
			if ((pos = strrchr(errmsg, '\n')) != NULL) pos[0] = '\0';
		}
		set_error(ctx, EIO, errmsg);
		free(errmsg);
		PQclear(res);
		return -1;
	}

	nstr = PQcmdTuples(res);
	if (nstr && nstr[0]) n = atoi(nstr);
	else n = PQntuples(res);
	if (stmt_out) {
		stmt = calloc(1, sizeof(*stmt));
		stmt->generic.ctx = ctx_gen;
		stmt->res = res;
		stmt->nrows = n;
		*stmt_out = (glite_lbu_Statement)stmt;
	} else {
		PQclear(res);
	}
	return n;
}


int glite_lbu_QueryColumnsPsql(glite_lbu_Statement stmt_gen, char **cols) {
	glite_lbu_StatementPsql stmt = (glite_lbu_StatementPsql)stmt_gen;
	int n, i;

	n = PQnfields(stmt->res);
	for (i = 0; i < n; i++) {
		cols[i] = PQfname(stmt->res, i);
	}
	return -1;
}


int glite_lbu_PrepareStmtPsql(glite_lbu_DBContext ctx_gen, const char *sql, glite_lbu_Statement *stmt_out) {
	glite_lbu_DBContextPsql ctx = (glite_lbu_DBContextPsql)ctx_gen;
	int i, retval = -1;
	const char *selectp, *updatep, *insertp, *minp;
	char *sqlPrep = NULL, *s = NULL;
	glite_lbu_StatementPsql stmt;
	PGresult *res = NULL;

	// init
	stmt = calloc(1, sizeof(*stmt));
	stmt->generic.ctx = ctx_gen;
	stmt->sql = strdup(sql);

	// name of the prepared command used as ID in postgres
	i = -1;
	minp = stmt->sql + strlen(stmt->sql);
	selectp = strcasestr(stmt->sql, "SELECT");
	updatep = strcasestr(stmt->sql, "UPDATE");
	insertp = strcasestr(stmt->sql, "INSERT");
	if (selectp && selectp < minp) { minp = selectp; i = 0; }
	if (updatep && updatep < minp) { minp = updatep; i = 1; }
	if (insertp && insertp < minp) { minp = insertp; i = 2; }
	if (i == -1 || minp[0] == '\0') i = 3;
	asprintf(&stmt->name, "%s%d", prepared_names[i], ++ctx->prepared_counts[i]);

	asprintf(&sqlPrep, "PREPARE %s AS %s", stmt->name, stmt->sql);
	glite_common_log_msg(ctx_gen->log_category, LOG_PRIORITY_DEBUG, sqlPrep);
	res = PQexec(ctx->conn, sqlPrep);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		asprintf(&s, "error preparing command: %s", PQerrorMessage(ctx->conn));
		set_error(ctx, EIO, s);
		free(s); s = NULL;
		goto quit;
	}

	*stmt_out = (glite_lbu_Statement)stmt;
	retval = 0;

quit:
	free(sqlPrep);
	if (res) PQclear(res);
	if (!retval) return 0;

	free(stmt->name);
	free(stmt->sql);
	free(stmt);
	return retval;
}


int glite_lbu_ExecPreparedStmtPsql_v(glite_lbu_Statement stmt_gen, int n, va_list ap) {
	glite_lbu_StatementPsql stmt = (glite_lbu_StatementPsql)stmt_gen;
	glite_lbu_DBContextPsql ctx = (glite_lbu_DBContextPsql)stmt_gen->ctx;
	int i, retval = -1, status;
	char **tmpdata = NULL;
	char *sql = NULL, *s, *nstr;
	size_t data_len = 0;

	glite_lbu_DBType type;

	if (!stmt || !stmt->sql || !stmt->name)
		return set_error(ctx, EINVAL, "PrepareStmt() not called");

	if (stmt->res) {
		PQclear(stmt->res);
		stmt->res = NULL;
	}

	// gather parameters
	if (n) {
		tmpdata = calloc(n, sizeof(char *));
	}
	for (i = 0; i < n; i++) {
		type = va_arg(ap, glite_lbu_DBType);

		switch(type) {
		case GLITE_LBU_DB_TYPE_TINYINT:
			asprintf(&tmpdata[i], "%d", va_arg(ap, int));
			break;

		case GLITE_LBU_DB_TYPE_INT:
			asprintf(&tmpdata[i], "%ld", va_arg(ap, long int));
			break;

		case GLITE_LBU_DB_TYPE_TINYBLOB:
		case GLITE_LBU_DB_TYPE_TINYTEXT:
		case GLITE_LBU_DB_TYPE_BLOB:
		case GLITE_LBU_DB_TYPE_TEXT:
		case GLITE_LBU_DB_TYPE_MEDIUMBLOB:
		case GLITE_LBU_DB_TYPE_MEDIUMTEXT:
		case GLITE_LBU_DB_TYPE_LONGBLOB:
		case GLITE_LBU_DB_TYPE_LONGTEXT: {
			char *tmp, *s;
			unsigned long binary_len;

			s = va_arg(ap, char *);
			binary_len = va_arg(ap, unsigned long);
			glite_common_log(ctx->generic.log_category, LOG_PRIORITY_DEBUG, 
				"blob, len = %lu, ptr = %p\n", binary_len, s);
			if (s) {
				tmp = malloc(2*binary_len + 1);
				PQescapeStringConn(ctx->conn, tmp, s, binary_len, NULL);
				asprintf(&tmpdata[i], "'%s'", tmp);
				glite_common_log(ctx->generic.log_category, LOG_PRIORITY_DEBUG, "escaped: '%s'\n", tmpdata[i]);
				free(tmp);
			} else
				tmpdata[i] = strdup("NULL");
			break;
		}


		case GLITE_LBU_DB_TYPE_VARCHAR:
		case GLITE_LBU_DB_TYPE_CHAR:
			s = va_arg(ap, char *);
			if (s) trio_asprintf(&tmpdata[i], "'%|Ss'", s);
			else tmpdata[i] = strdup("NULL");
			break;

		case GLITE_LBU_DB_TYPE_DATE:
		case GLITE_LBU_DB_TYPE_TIME:
		case GLITE_LBU_DB_TYPE_DATETIME:
			glite_lbu_TimeToStr(va_arg(ap, time_t), &tmpdata[i]);
			break;

		case GLITE_LBU_DB_TYPE_TIMESTAMP:
			glite_lbu_TimestampToStr(va_arg(ap, double), &tmpdata[i]);
			break;

		case GLITE_LBU_DB_TYPE_NULL:
			tmpdata[i] = strdup("NULL");
			break;

		case GLITE_LBU_DB_TYPE_BOOLEAN:
			tmpdata[i] = strdup(va_arg(ap, int) ? "true" : "false");
			break;

		default:
			glite_common_log(ctx->generic.log_category, LOG_PRIORITY_DEBUG,
				"unknown type %d\n", type);
			set_error(ctx, EINVAL, "unimplemented type");
			goto quit;
		}

		data_len += strlen(tmpdata[i]);
	}

	asprintf(&sql, "EXECUTE %s", stmt->name);
	s = realloc(sql, strlen(sql) + (2 * n - 2) + strlen(" ()") + data_len + 1);
	if (!s) goto quit;
	sql = s;
	for (i = 0; i < n; i++) {
		strcat(sql, i ? ", " : " (" ); s += 2;
		strcat(sql, tmpdata[i]);
	}
	if (n) strcat(sql, ")");

	glite_common_log_msg(ctx->generic.log_category, LOG_PRIORITY_DEBUG, sql);
	stmt->res = PQexec(ctx->conn, sql);
	status = PQresultStatus(stmt->res);
	if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
		asprintf(&s, "error executing prepared command '%s' parameters '%s': %s", stmt->sql, sql, PQerrorMessage(ctx->conn));
		set_error(ctx, EIO, s);
		free(s); s = NULL;
		goto quit;
	}
	nstr = PQcmdTuples(stmt->res);
	//lprintf("cmdtuples: '%s'\n", nstr);
	if (nstr && nstr[0]) retval = atoi(nstr);
	else retval = PQntuples(stmt->res);
	stmt->nrows = retval;
	stmt->row = 0;
	//lprintf("ntuples/retval: %d\n", retval);

quit:
	for (i = 0; i < n; i++) free(tmpdata[i]);
	free(tmpdata);
	free(sql);
	return retval;
}
