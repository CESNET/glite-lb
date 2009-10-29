/**
 * Simple postgres module with org.glite.lbjp-common.db interface.
 * 
 * Limitations:
 *  - binary results as escaped strings
 *
 * PostgreSQL limitations:
 *  - prepared commands requires server >= 8.2
 */

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libpq-fe.h>

#include "glite/lbu/trio.h"
#include "glite/lbu/db.h"

#define DB_CONNECT_TIMEOUT "20"

#ifdef LOG
  #define lprintf(ARGS...) printf("[db-pg] %s: ", __FUNCTION__); printf(##ARGS)
#else
  #define lprintf(ARGS...)
#endif


struct glite_lbu_DBContext_s {
	int caps;
	struct {
		int code;
		char *desc;
	} err;
	PGconn *conn;
	int prepared_counts[4];
};

struct glite_lbu_Statement_s {
        glite_lbu_DBContext  ctx;
	PGresult *res;
	int row, nrows;
	char *sql, *name;
};


/* nicer identifiers in PREPARE/EXECUTE commands */
static const char *prepared_names[4] = {"select", "update", "insert", "other"};

//static void time2str(time_t, char **str);


#define set_error(CTX, CODE, DESC) set_error_func((CTX), (CODE), (DESC), __FUNCTION__, __LINE__)
static int set_error_func(glite_lbu_DBContext ctx, int code, const char *desc, const char *func, int line) {
	char *pos;

	if (ctx->err.desc) free(ctx->err.desc);
	ctx->err.code = code;
	ctx->err.desc = strdup(desc ? desc : "(null)");
	pos = strrchr(desc, '\n'); if (pos) pos[0] = '\0';

	if ((ctx->caps & GLITE_LBU_DB_CAP_ERRORS) != 0) fprintf(stderr, "[db %d] %s:%d %s\n", getpid(), func, line, desc);
	return code;
}


int glite_lbu_DBError(glite_lbu_DBContext ctx, char **text, char **desc) {
	if (text) *text = strdup(strerror(ctx->err.code));
	if (desc) {
		if (ctx->err.desc) *desc = strdup(ctx->err.desc);
		else *desc = NULL;
	}

	return ctx->err.code;
}


int glite_lbu_InitDBContext(glite_lbu_DBContext *ctx) {
	*ctx = calloc(1, sizeof **ctx);
	if (!*ctx) return ENOMEM;

	return 0;
}


void glite_lbu_FreeDBContext(glite_lbu_DBContext ctx) {
	if (ctx) {
		free(ctx->err.desc);
		free(ctx);
	}
}


int glite_lbu_DBConnect(glite_lbu_DBContext ctx, const char *cs) {
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

	lprintf("connection string = %s\n", pgcs);
	ctx->conn = PQconnectdb(pgcs);
	free(pgcsbuf);
	if (!ctx->conn) return ENOMEM;

	

	if (PQstatus(ctx->conn) != CONNECTION_OK) {
		asprintf(&err, "Can't connect, %s", PQerrorMessage(ctx->conn));
		PQfinish(ctx->conn);
		ctx->conn = NULL;
		set_error(ctx, EIO, err);
		free(err);
		return ctx->err.code;
	}

	return 0;
}


void glite_lbu_DBClose(glite_lbu_DBContext ctx) {
	if (ctx->conn) PQfinish(ctx->conn);
}


int glite_lbu_DBQueryCaps(glite_lbu_DBContext ctx) {
	glite_lbu_Statement stmt;
	int major, minor, sub, version;
	int has_prepared = 0;
	char *res = NULL;

	if (glite_lbu_ExecSQL(ctx, "SHOW server_version", &stmt) == -1) return -1;
	switch (glite_lbu_FetchRow(stmt, 1, NULL, &res)) {
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
	glite_lbu_FreeStmt(&stmt);
	return has_prepared;
}


void glite_lbu_DBSetCaps(glite_lbu_DBContext ctx, int caps) {
	ctx->caps = caps;
}


int glite_lbu_Transaction(glite_lbu_DBContext ctx) {
	return 0;
}


int glite_lbu_Commit(glite_lbu_DBContext ctx) {
	return 0;
}


int glite_lbu_Rollback(glite_lbu_DBContext ctx) {
	return 0;
}


int glite_lbu_FetchRow(glite_lbu_Statement stmt, unsigned int maxn, unsigned long *lengths, char **results) {
	unsigned int i, n;

	if (stmt->row >= stmt->nrows) return 0;

	n = PQnfields(stmt->res);
	if (n <= 0) {
		set_error(stmt->ctx, EINVAL, "Result set w/o columns");
		return -1;
	}
	if (n > maxn) {
		set_error(stmt->ctx, EINVAL, "Not enough room for the result");
		return -1;
	}
	for (i = 0; i < n; i++) {
		results[i] = PQgetvalue(stmt->res, stmt->row, i);
		/* sanity check for internal error (NULL when invalid row) */
		results[i] = strdup(results[i] ? : "");
		if (lengths) lengths[i] = PQgetlength(stmt->res, stmt->row, i);
	}

	stmt->row++;
	return n;
}


void glite_lbu_FreeStmt(glite_lbu_Statement *stmt) {
	char *sql;

	if (!*stmt) return;
	if ((*stmt)->res) PQclear((*stmt)->res);
	if ((*stmt)->name) {
		asprintf(&sql, "DEALLOCATE %s", (*stmt)->name);
		(*stmt)->res = PQexec((*stmt)->ctx->conn, sql);
		free(sql);
		PQclear((*stmt)->res);
	}
	free((*stmt)->name);
	free((*stmt)->sql);
	free(*stmt);
	*stmt = NULL;
}


int glite_lbu_ExecSQL(glite_lbu_DBContext ctx, const char *cmd, glite_lbu_Statement *stmt_out) {
	glite_lbu_Statement stmt = NULL;
	int status, n;
	char *nstr;
	PGresult *res;

	lprintf("command = %s\n", cmd);
	if (stmt_out) *stmt_out = NULL;
	if ((res = PQexec(ctx->conn, cmd)) == NULL) {
		ctx->err.code = ENOMEM;
		return -1;
	}

	status = PQresultStatus(res);
	if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
		set_error(ctx, EIO, PQresultErrorMessage(res));
		PQclear(res);
		return -1;
	}

	nstr = PQcmdTuples(res);
	if (nstr && nstr[0]) n = atoi(nstr);
	else n = PQntuples(res);
	if (stmt_out) {
		stmt = calloc(1, sizeof(*stmt));
		stmt->ctx = ctx;
		stmt->res = res;
		stmt->nrows = n;
		*stmt_out = stmt;
	} else {
		PQclear(res);
	}
	return n;
}


int glite_lbu_QueryColumns(glite_lbu_Statement stmt, char **cols) {
	int n, i;

	n = PQnfields(stmt->res);
	for (i = 0; i < n; i++) {
		cols[i] = PQfname(stmt->res, i);
	}
	return -1;
}


#if 0
static void time2str(time_t t, char **str) {
	struct tm       *tm = gmtime(&t);

	asprintf(str,"%4d-%02d-%02d %02d:%02d:%02d",tm->tm_year+1900,tm->tm_mon+1,
	         tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
}
#endif


void glite_lbu_TimeToDB(time_t t, char **str) {
	struct tm       *tm = gmtime(&t);

	asprintf(str,"'%4d-%02d-%02d %02d:%02d:%02d'",tm->tm_year+1900,tm->tm_mon+1,
	         tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
}


void glite_lbu_TimestampToDB(double t, char **str) {
	time_t tsec = t;
	struct tm *tm = gmtime(&tsec);

	t = t - tsec + tm->tm_sec;
	asprintf(str,"'%4d-%02d-%02d %02d:%02d:%02.09f'",tm->tm_year+1900,tm->tm_mon+1,
	         tm->tm_mday,tm->tm_hour,tm->tm_min,t);
}


time_t glite_lbu_DBToTime(const char *str) {
	struct tm       tm;

	memset(&tm,0,sizeof(tm));
	setenv("TZ","UTC",1); tzset();
	sscanf(str,"%4d-%02d-%02d %02d:%02d:%02d",
	        &tm.tm_year,&tm.tm_mon,&tm.tm_mday,
        	&tm.tm_hour,&tm.tm_min,&tm.tm_sec);
	tm.tm_year -= 1900;
	tm.tm_mon--;

	return mktime(&tm);
}


/*
int glite_lbu_QueryIndices(glite_lbu_DBContext ctx, const char *table, char ***key_names, char ****column_names) { }
long int glite_lbu_Lastid(glite_lbu_Statement stmt) {}
*/


int glite_lbu_PrepareStmt(glite_lbu_DBContext ctx, const char *sql, glite_lbu_Statement *stmtout) {
	int i, retval = -1;
	const char *selectp, *updatep, *insertp, *minp;
	char *sqlPrep = NULL, *s = NULL;
	glite_lbu_Statement stmt;
	PGresult *res = NULL;

	// init
	stmt = calloc(1, sizeof(*stmt));
	stmt->ctx = ctx;
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
	asprintf(&stmt->name, "%s%d", prepared_names[i], ++stmt->ctx->prepared_counts[i]);

	asprintf(&sqlPrep, "PREPARE %s AS %s", stmt->name, stmt->sql);
	lprintf("prepare = %s\n", sqlPrep);
	res = PQexec(stmt->ctx->conn, sqlPrep);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		asprintf(&s, "error preparing command: %s", PQerrorMessage(stmt->ctx->conn));
		set_error(stmt->ctx, EIO, s);
		free(s); s = NULL;
		goto quit;
	}

	*stmtout = stmt;
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


int glite_lbu_ExecPreparedStmt_v(glite_lbu_Statement stmt, int n, va_list ap) {
	int i, retval = -1, status;
	char **tmpdata = NULL;
	char *sql = NULL, *s, *nstr;
	size_t data_len = 0;

	glite_lbu_DBType type;

	if (!stmt || !stmt->sql || !stmt->name)
		return set_error(stmt->ctx, EINVAL, "PrepareStmt() not called");

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
			unsigned char *tmp, *s;
			unsigned long binary_len;
			size_t result_len;

			s = (unsigned char *)va_arg(ap, char *);
			binary_len = va_arg(ap, unsigned long);
			if (s) {
				tmp = PQescapeByteaConn(stmt->ctx->conn, s, binary_len, &result_len);
				asprintf(&tmpdata[i], "'%s'", tmp);
				PQfreemem(tmp);
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
			glite_lbu_TimeToDB(va_arg(ap, time_t), &tmpdata[i]);
			break;

		case GLITE_LBU_DB_TYPE_TIMESTAMP:
			glite_lbu_TimestampToDB(va_arg(ap, double), &tmpdata[i]);
			break;

		case GLITE_LBU_DB_TYPE_NULL:
			tmpdata[i] = strdup("NULL");
			break;

		case GLITE_LBU_DB_TYPE_BOOLEAN:
			tmpdata[i] = strdup(va_arg(ap, int) ? "true" : "false");
			break;

		default:
			lprintf("unknown type %d\n", type);
			set_error(stmt->ctx, EINVAL, "unimplemented type");
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

	lprintf("exec prepared: n = %d, sql = '%s'\n", n, sql);
	stmt->res = PQexec(stmt->ctx->conn, sql);
	status = PQresultStatus(stmt->res);
	if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
		asprintf(&s, "error executing prepared command '%s' parameters '%s': %s", stmt->sql, sql, PQerrorMessage(stmt->ctx->conn));
		set_error(stmt->ctx, EIO, s);
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


int glite_lbu_ExecPreparedStmt(glite_lbu_Statement stmt, int n, ...) {
	va_list ap;
	int retval;

	va_start(ap, n);
	retval = glite_lbu_ExecPreparedStmt_v(stmt, n, ap);
	va_end(ap);

	return retval;
}


/*
struct glite_lbu_bufInsert_s {
        glite_lbu_DBContext ctx;
}
int glite_lbu_bufferedInsertInit(glite_lbu_DBContext ctx, glite_lbu_bufInsert *bi, const char *table_name, long size_limit, long record_limit, const char *columns)
{}
int glite_lbu_bufferedInsert(glite_lbu_bufInsert bi, const char *row) {}
int glite_lbu_bufferedInsertClose(glite_lbu_bufInsert bi) {}
*/
