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

#include <sys/types.h>
#ifdef LBS_DB_PROFILE
#include <sys/time.h>
#endif
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <mysql.h>
#include <mysqld_error.h>
#include <mysql_version.h>
#include <errmsg.h>

#include "glite/lbu/trio.h"
#include "glite/lbu/log.h"
#include "db.h"
#include "db-int.h"


#define GLITE_LBU_MYSQL_INDEX_VERSION 40001
#define GLITE_LBU_MYSQL_PREPARED_VERSION 40102
#define BUF_INSERT_ROW_ALLOC_BLOCK	1000
#ifndef GLITE_LBU_DEFAULT_RESULT_BUFFER_LENGTH
#define GLITE_LBU_DEFAULT_RESULT_BUFFER_LENGTH 256
#endif

#define CLR_ERR(CTX) glite_lbu_DBClearError((glite_lbu_DBContext)(CTX))
#define ERR(CTX, CODE, ARGS...) glite_lbu_DBSetError((glite_lbu_DBContext)((CTX)), (CODE), __FUNCTION__, __LINE__, ##ARGS)
#define STATUS(CTX) glite_lbu_DBError((glite_lbu_DBContext)((CTX)), NULL, NULL)
#define MY_ERR(CTX) myerr((CTX), __FUNCTION__, __LINE__)
#define MY_ERRSTMT(STMT) myerrstmt((STMT), __FUNCTION__, __LINE__)
#define MY_ISOKSTMT(STMT, RETRY) myisokstmt((STMT), __FUNCTION__, __LINE__, (RETRY))

#define USE_TRANS(CTX) ((CTX->generic.caps & GLITE_LBU_DB_CAP_TRANSACTIONS) != 0)



struct glite_lbu_DBContextMysql_s {
	struct glite_lbu_DBContext_s generic;
	int in_transaction;	/* this flag is set whenever we are in DB transaction */
	MYSQL *mysql;
};
typedef struct glite_lbu_DBContextMysql_s *glite_lbu_DBContextMysql;


struct glite_lbu_StatementMysql_s {
	glite_lbu_Statement_t generic;

	/* for simple commands */
	MYSQL_RES           *result;

	/* for prepared commands */
	MYSQL_STMT          *stmt;
	unsigned long        nrfields;
	char                *sql;
};
typedef struct glite_lbu_StatementMysql_s *glite_lbu_StatementMysql;


/*
 * mapping glite DB types to mysql types
 */
int glite_type_to_mysql[] = {
	MYSQL_TYPE_NULL,
	MYSQL_TYPE_TINY,
	MYSQL_TYPE_LONG,
	MYSQL_TYPE_TINY_BLOB,
	MYSQL_TYPE_TINY_BLOB,
	MYSQL_TYPE_BLOB,
	MYSQL_TYPE_BLOB,
	MYSQL_TYPE_MEDIUM_BLOB,
	MYSQL_TYPE_MEDIUM_BLOB,
	MYSQL_TYPE_LONG_BLOB,
	MYSQL_TYPE_LONG_BLOB,
	MYSQL_TYPE_VAR_STRING,
	MYSQL_TYPE_STRING,
	MYSQL_TYPE_DATE,
	MYSQL_TYPE_TIME,
	MYSQL_TYPE_DATETIME,
	MYSQL_TYPE_TIMESTAMP,
};


/* backend module declaration */
int glite_lbu_InitDBContextMysql(glite_lbu_DBContext *ctx_gen);
void glite_lbu_FreeDBContextMysql(glite_lbu_DBContext ctx_gen);
int glite_lbu_DBConnectMysql(glite_lbu_DBContext ctx_gen, const char *cs);
void glite_lbu_DBCloseMysql(glite_lbu_DBContext ctx_gen);
int glite_lbu_DBQueryCapsMysql(glite_lbu_DBContext ctx_gen);
void glite_lbu_DBSetCapsMysql(glite_lbu_DBContext commmon_ctx, int caps);
int glite_lbu_TransactionMysql(glite_lbu_DBContext ctx_gen);
int glite_lbu_CommitMysql(glite_lbu_DBContext ctx_gen);
int glite_lbu_RollbackMysql(glite_lbu_DBContext ctx_gen);
int glite_lbu_FetchRowMysql(glite_lbu_Statement stmt, unsigned int n, unsigned long *lengths, char **results);
void glite_lbu_FreeStmtMysql(glite_lbu_Statement *stmt);
int glite_lbu_QueryIndicesMysql(glite_lbu_DBContext ctx_gen, const char *table, char ***key_names, char ****column_names);
int glite_lbu_ExecSQLMysql(glite_lbu_DBContext ctx_gen, const char *cmd, glite_lbu_Statement *stmt);
int glite_lbu_QueryColumnsMysql(glite_lbu_Statement stmt_gen, char **cols);
int glite_lbu_PrepareStmtMysql(glite_lbu_DBContext ctx_gen, const char *sql, glite_lbu_Statement *stmt_gen);
int glite_lbu_ExecPreparedStmtMysql_v(glite_lbu_Statement stmt_gen, int n, va_list ap);
long int glite_lbu_LastidMysql(glite_lbu_Statement stmt_gen);

glite_lbu_DBBackend_t mysql_backend = {
	backend: GLITE_LBU_DB_BACKEND_MYSQL,

	initContext: glite_lbu_InitDBContextMysql,
	freeContext: glite_lbu_FreeDBContextMysql,
	connect: glite_lbu_DBConnectMysql,
	close: glite_lbu_DBCloseMysql,
	queryCaps: glite_lbu_DBQueryCapsMysql,
	setCaps: glite_lbu_DBSetCapsMysql,
	transaction: glite_lbu_TransactionMysql,
	commit: glite_lbu_CommitMysql,
	rollback: glite_lbu_RollbackMysql,
	fetchRow: glite_lbu_FetchRowMysql,
	freeStmt: glite_lbu_FreeStmtMysql,
	queryIndices: glite_lbu_QueryIndicesMysql,
	execSQL: glite_lbu_ExecSQLMysql,
	queryColumns: glite_lbu_QueryColumnsMysql,

	timeToDB: glite_lbu_TimeToStr,
	timestampToDB: glite_lbu_TimestampToStr,
	DBToTime: glite_lbu_StrToTime,
	DBToTimestamp: glite_lbu_StrToTimestamp,

	prepareStmt: glite_lbu_PrepareStmtMysql,
	execPreparedStmt_v: glite_lbu_ExecPreparedStmtMysql_v,
	lastid: glite_lbu_LastidMysql,
};


static int myerr(glite_lbu_DBContextMysql ctx, const char *source, int line);
static int myerrstmt(glite_lbu_StatementMysql stmt, const char *source, int line);
static int myisokstmt(glite_lbu_StatementMysql stmt, const char *source, int line, int *retry);
static int db_connect(glite_lbu_DBContextMysql ctx, const char *cs, MYSQL **mysql);
static void db_close(MYSQL *mysql);
static int transaction_test(glite_lbu_DBContext ctx, int *caps);
static int FetchRowSimple(glite_lbu_DBContextMysql ctx, MYSQL_RES *result, unsigned long *lengths, char **results);
static int FetchRowPrepared(glite_lbu_DBContextMysql ctx, glite_lbu_StatementMysql stmt, unsigned int n, unsigned long *lengths, char **results);
static void set_time(MYSQL_TIME *mtime, const double time);
static void glite_lbu_FreeStmt_int(glite_lbu_StatementMysql stmt);


/* ---- common ---- */



int glite_lbu_InitDBContextMysql(glite_lbu_DBContext *ctx_gen) {
	glite_lbu_DBContextMysql ctx;

	ctx = calloc(1, sizeof *ctx);
	if (!ctx) return ENOMEM;
	*ctx_gen = (glite_lbu_DBContext)ctx;

	return 0;
}


void glite_lbu_FreeDBContextMysql(glite_lbu_DBContext ctx_gen) {
	glite_lbu_DBContextMysql ctx = (glite_lbu_DBContextMysql)ctx_gen;

	if (ctx) {
		assert(ctx->mysql == NULL);
		free(ctx);
	}
}


int glite_lbu_DBConnectMysql(glite_lbu_DBContext ctx_gen, const char *cs) {
	glite_lbu_DBContextMysql ctx = (glite_lbu_DBContextMysql)ctx_gen;

	if (db_connect(ctx, cs, &ctx->mysql) != 0 ||
	    glite_lbu_ExecSQLMysql(ctx_gen, "SET AUTOCOMMIT=1", NULL) < 0 ||
	    glite_lbu_ExecSQLMysql(ctx_gen, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ", NULL) < 0)
		return STATUS(ctx);
	else
		return 0;
}


void glite_lbu_DBCloseMysql(glite_lbu_DBContext ctx_gen) {
	glite_lbu_DBContextMysql ctx = (glite_lbu_DBContextMysql)ctx_gen;

	db_close(ctx->mysql);
	ctx->mysql = NULL;
	CLR_ERR(ctx);
}


int glite_lbu_DBQueryCapsMysql(glite_lbu_DBContext ctx_gen) {
	glite_lbu_DBContextMysql ctx = (glite_lbu_DBContextMysql)ctx_gen;
	MYSQL	*m = ctx->mysql;
	int	major,minor,sub,version,caps;
	const char *ver_s;

	caps = 0;

	ver_s = mysql_get_server_info(m);
	if (!ver_s || 3 != sscanf(ver_s,"%d.%d.%d",&major,&minor,&sub))
		return ERR(ctx, EINVAL, "problem retreiving MySQL version");
	version = 10000*major + 100*minor + sub;

	if (version >= GLITE_LBU_MYSQL_INDEX_VERSION) caps |= GLITE_LBU_DB_CAP_INDEX;
	if (version >= GLITE_LBU_MYSQL_PREPARED_VERSION) caps |= GLITE_LBU_DB_CAP_PREPARED;

	CLR_ERR(ctx);
	transaction_test(ctx_gen, &caps);

	if (STATUS(ctx) == 0) return caps;
	else return -1;
}


void glite_lbu_DBSetCapsMysql(glite_lbu_DBContext ctx_gen, int caps) {
	ctx_gen->caps = caps;
}


int glite_lbu_TransactionMysql(glite_lbu_DBContext ctx_gen) {
	glite_lbu_DBContextMysql ctx = (glite_lbu_DBContextMysql)ctx_gen;

	CLR_ERR(ctx);
	if (USE_TRANS(ctx)) {
		if (glite_lbu_ExecSQLMysql(ctx_gen, "SET AUTOCOMMIT=0", NULL) < 0) goto err;
		if (glite_lbu_ExecSQLMysql(ctx_gen, "BEGIN", NULL) < 0) goto err;
		ctx->in_transaction = 1;
	}
err:
	return STATUS(ctx);
}


int glite_lbu_CommitMysql(glite_lbu_DBContext ctx_gen) {
	glite_lbu_DBContextMysql ctx = (glite_lbu_DBContextMysql)ctx_gen;

	CLR_ERR(ctx);
	if (USE_TRANS(ctx)) {
		if (glite_lbu_ExecSQLMysql(ctx_gen, "COMMIT", NULL) < 0) goto err;
		if (glite_lbu_ExecSQLMysql(ctx_gen, "SET AUTOCOMMIT=1", NULL) < 0) goto err;
		ctx->in_transaction = 0;
	}
err:
	return STATUS(ctx);
}


int glite_lbu_RollbackMysql(glite_lbu_DBContext ctx_gen) {
	glite_lbu_DBContextMysql ctx = (glite_lbu_DBContextMysql)ctx_gen;

	CLR_ERR(ctx);
	if (USE_TRANS(ctx)) { 
		if (glite_lbu_ExecSQLMysql(ctx_gen, "ROLLBACK", NULL) < 0) goto err;
		if (glite_lbu_ExecSQLMysql(ctx_gen, "SET AUTOCOMMIT=1", NULL) < 0) goto err;
		ctx->in_transaction = 0;
	}
err:
	return STATUS(ctx);
}


int glite_lbu_FetchRowMysql(glite_lbu_Statement stmt_gen, unsigned int n, unsigned long *lengths, char **results) {
	glite_lbu_StatementMysql stmt = (glite_lbu_StatementMysql)stmt_gen;
	glite_lbu_DBContextMysql ctx = (glite_lbu_DBContextMysql)stmt->generic.ctx;

	memset(results, 0, n * sizeof(*results));
	if (stmt->result) return FetchRowSimple(ctx, stmt->result, lengths, results);
	else return FetchRowPrepared(ctx, stmt, n, lengths, results);
}


static void glite_lbu_FreeStmt_int(glite_lbu_StatementMysql stmt) {
	if (stmt) {
		if (stmt->result) mysql_free_result(stmt->result);
		if (stmt->stmt) mysql_stmt_close(stmt->stmt);
		free(stmt->sql);
	}
}


void glite_lbu_FreeStmtMysql(glite_lbu_Statement *stmt_gen) {
	glite_lbu_StatementMysql *stmt = (glite_lbu_StatementMysql*)stmt_gen;

	glite_lbu_FreeStmt_int(*stmt);
	free(*stmt);
	*stmt = NULL;
}


int glite_lbu_QueryIndicesMysql(glite_lbu_DBContext ctx_gen, const char *table, char ***key_names, char ****column_names) {
	glite_lbu_DBContextMysql ctx = (glite_lbu_DBContextMysql)ctx_gen;
	glite_lbu_Statement stmt;

	size_t	i,j,ret;

/* XXX: "show index from" columns. Matches at least MySQL 4.0.11 */
	char	*sql, *showcol[13];
	int	Key_name,Seq_in_index,Column_name,Sub_part;

	char	**keys = NULL;
	size_t	*cols = NULL;
	char	**col_names = NULL;

	size_t	nkeys = 0;

	char	***idx = NULL;

	Key_name = Seq_in_index = Column_name = Sub_part = -1;

	asprintf(&sql, "show index from %s", table);
	glite_common_log_msg(ctx_gen->log_category, LOG_PRIORITY_DEBUG, sql);
	if (glite_lbu_ExecSQLMysql(ctx_gen,sql,&stmt)<0) {
		free(sql);
		return STATUS(ctx);
	}
	free(sql);

	while ((ret = glite_lbu_FetchRowMysql(stmt,sizeof(showcol)/sizeof(showcol[0]),NULL,showcol)) > 0) {
		assert(ret <= (int)(sizeof showcol/sizeof showcol[0]));

		if (!col_names) {
			col_names = malloc(ret * sizeof col_names[0]);
			glite_lbu_QueryColumnsMysql(stmt,col_names);
			for (i=0; i<ret; i++) 
				if (!strcasecmp(col_names[i],"Key_name")) Key_name = i;
				else if (!strcasecmp(col_names[i],"Seq_in_index")) Seq_in_index = i;
				else if (!strcasecmp(col_names[i],"Column_name")) Column_name = i;
				else if (!strcasecmp(col_names[i],"Sub_part")) Sub_part = i;

			assert(Key_name >= 0 && Seq_in_index >= 0 && 
					Column_name >= 0 && Sub_part >= 0);

		}

		for (i=0; i<nkeys && strcasecmp(showcol[Key_name],keys[i]); i++);

		if (i == nkeys) {
			keys = realloc(keys,(i+2) * sizeof keys[0]);
			keys[i] = strdup(showcol[Key_name]);
//printf("** KEY [%d] %s\n", i, keys[i]);
			keys[i+1] = NULL;
			cols = realloc(cols,(i+1) * sizeof cols[0]); 
			cols[i] = 0;
			idx = realloc(idx,(i+2) * sizeof idx[0]);
			idx[i] = idx[i+1] = NULL;
			nkeys++;
		}

		j = atoi(showcol[Seq_in_index])-1;
		if (cols[i] <= j) {
			cols[i] = j+1;
			idx[i] = realloc(idx[i],(j+2)*sizeof idx[i][0]);
			memset(&idx[i][j+1],0,sizeof idx[i][0]);
		}
		idx[i][j] = strdup(showcol[Column_name]);
//printf("****** [%d, %d] %s\n", i, j, idx[i][j]);
//FIXME: needed?idx[i][j].value.i = atoi(showcol[Sub_part]);
		for (i = 0; i<ret; i++) free(showcol[i]);
	}

	glite_lbu_FreeStmtMysql(&stmt);
	free(cols);
	free(col_names);

	if (ret == 0) CLR_ERR(ctx);
	else {
		free(keys);
		keys = NULL;
		for (i = 0; idx[i]; i++) {
			for (j = 0; idx[i][j]; j++) free(idx[i][j]);
			free(idx[i]);
		}
		free(idx);
		idx = NULL;
	}

	if (key_names) *key_names = keys;
	else {
		for (i = 0; keys[i]; i++) free(keys[i]);
		free(keys);
	}
	*column_names = idx;

	return STATUS(ctx);
}


/* ---- simple ---- */

int glite_lbu_ExecSQLMysql(glite_lbu_DBContext ctx_gen, const char *cmd, glite_lbu_Statement *stmt_gen) {
	glite_lbu_DBContextMysql ctx = (glite_lbu_DBContextMysql)ctx_gen;
	glite_lbu_StatementMysql stmt;
	int	merr;
	int	retry_nr = 0;
	int	do_reconnect = 0;
#ifdef LBS_DB_PROFILE
	struct timeval	start,end;
	int	pid;

	static struct timeval sum = {
		tv_sec: 0,
		tv_usec: 0
	};
#endif

	CLR_ERR(ctx);

	if (stmt_gen) *stmt_gen = NULL;
	stmt = NULL;

#ifdef LBS_DB_PROFILE
	gettimeofday(&start,NULL);
#endif

	while (retry_nr == 0 || do_reconnect) {
		do_reconnect = 0;
		if (mysql_query(ctx->mysql, cmd)) {
			/* error occured */
			switch (merr = mysql_errno(ctx->mysql)) {
				case 0:
					break;
				case ER_DUP_ENTRY: 
					ERR(ctx, EEXIST, mysql_error(ctx->mysql));
					return -1;
					break;
				case CR_SERVER_LOST:
				case CR_SERVER_GONE_ERROR:
					if (ctx->in_transaction) {
						ERR(ctx, ENOTCONN, mysql_error(ctx->mysql));
						return -1;
					}
					else if (retry_nr <= 0) 
						do_reconnect = 1;
					break;
				case ER_LOCK_DEADLOCK:
					ERR(ctx, EDEADLK, mysql_error(ctx->mysql));
					return -1;
					break;	
				default:
					MY_ERR(ctx);
					return -1;
					break;
			}
		}
		retry_nr++;
	}

	if (stmt_gen) {
		stmt = calloc(1, sizeof(*stmt));
		if (!stmt) {
			ERR(ctx, ENOMEM, NULL);
			return -1;
		}
		stmt->generic.ctx = ctx_gen;
		stmt->result = mysql_store_result(ctx->mysql);
		if (!stmt->result) {
			if (mysql_errno(ctx->mysql)) {
				MY_ERR(ctx);
				free(stmt);
				return -1;
			}
		}
		*stmt_gen = (glite_lbu_Statement)stmt;
	} else {
		MYSQL_RES	*r = mysql_store_result(ctx->mysql);
		mysql_free_result(r);
	}
#ifdef LBS_DB_PROFILE
	pid = getpid();
	gettimeofday(&end,NULL);
	end.tv_usec -= start.tv_usec;
	end.tv_sec -= start.tv_sec;
	if (end.tv_usec < 0) { end.tv_sec--; end.tv_usec += 1000000; }

	sum.tv_usec += end.tv_usec;
	sum.tv_sec += end.tv_sec + sum.tv_usec / 1000000;
	sum.tv_usec -= 1000000 * (sum.tv_usec / 1000000);
	fprintf(stderr,"[%d] %s\n[%d] %3ld.%06ld (sum: %3ld.%06ld)\n",pid,cmd,pid,end.tv_sec,end.tv_usec,sum.tv_sec,sum.tv_usec);
#endif

	return mysql_affected_rows(ctx->mysql);
}


int glite_lbu_QueryColumnsMysql(glite_lbu_Statement stmt_gen, char **cols)
{
	glite_lbu_StatementMysql stmt = (glite_lbu_StatementMysql)stmt_gen;
	int	i = 0;
	MYSQL_FIELD 	*f;

	CLR_ERR(stmt->generic.ctx);
	if (!stmt->result) return ERR(stmt->generic.ctx, ENOTSUP, "QueryColumns implemented only for simple API");
	while ((f = mysql_fetch_field(stmt->result))) cols[i++] = f->name;
	return i == 0;
}


/* ---- prepared --- */

int glite_lbu_PrepareStmtMysql(glite_lbu_DBContext ctx_gen, const char *sql, glite_lbu_Statement *stmt_gen) {
	glite_lbu_DBContextMysql ctx = (glite_lbu_DBContextMysql)ctx_gen;
	glite_lbu_StatementMysql stmt;
	int ret, retry;
	MYSQL_RES *meta;

	// init
	stmt = calloc(1, sizeof(*stmt));
	stmt->generic.ctx = ctx_gen;
	*stmt_gen = NULL;

	// create the SQL command
	if ((stmt->stmt = mysql_stmt_init(ctx->mysql)) == NULL)
		return STATUS(ctx);

	// prepare the SQL command
	retry = 1;
	do {
		mysql_stmt_prepare(stmt->stmt, sql, strlen(sql));
		ret = MY_ISOKSTMT(stmt, &retry);
	} while (ret == 0);
	if (ret == -1) goto failed;

	// number of fields (0 for no results)
	if ((meta = mysql_stmt_result_metadata(stmt->stmt)) != NULL) {
		stmt->nrfields = mysql_num_fields(meta);
		mysql_free_result(meta);
	} else
		stmt->nrfields = 0;

	// remember the command
	stmt->sql = strdup(sql);

	*stmt_gen = (glite_lbu_Statement)stmt;
	return CLR_ERR(ctx);

failed:
	glite_lbu_FreeStmt_int(stmt);
	free(stmt);
	return STATUS(ctx);
}


int glite_lbu_ExecPreparedStmtMysql_v(glite_lbu_Statement stmt_gen, int n, va_list ap) {
	glite_lbu_StatementMysql stmt = (glite_lbu_StatementMysql)stmt_gen;
	int i, prepare_retry;
	glite_lbu_DBType type;
	char *pchar;
	int *pint;
	long int *plint;
	MYSQL_TIME *ptime;
	int ret, retry;
	MYSQL_BIND *binds = NULL;
	void **data = NULL;
	unsigned long *lens = NULL;
	glite_lbu_Statement newstmt;

	// gather parameters
	if (n) {
		binds = calloc(n, sizeof(MYSQL_BIND));
		data = calloc(n, sizeof(void *));
		lens = calloc(n, sizeof(unsigned long *));
	}
	for (i = 0; i < n; i++) {
		type = va_arg(ap, glite_lbu_DBType);
		switch (type) {
		case GLITE_LBU_DB_TYPE_TINYINT:
			pchar = binds[i].buffer = data[i] = malloc(sizeof(char));
			*pchar = va_arg(ap, int);
			break;

		case GLITE_LBU_DB_TYPE_INT:
			plint = binds[i].buffer = data[i] = malloc(sizeof(long int));
			*plint = va_arg(ap, long int);
			break;

		case GLITE_LBU_DB_TYPE_BOOLEAN:
			pint = binds[i].buffer = data[i] = malloc(sizeof(int));
			*pint = va_arg(ap, int) ? 1 : 0;
			break;

		case GLITE_LBU_DB_TYPE_TINYBLOB:
		case GLITE_LBU_DB_TYPE_TINYTEXT:
		case GLITE_LBU_DB_TYPE_BLOB:
		case GLITE_LBU_DB_TYPE_TEXT:
		case GLITE_LBU_DB_TYPE_MEDIUMBLOB:
		case GLITE_LBU_DB_TYPE_MEDIUMTEXT:
		case GLITE_LBU_DB_TYPE_LONGBLOB:
		case GLITE_LBU_DB_TYPE_LONGTEXT:
			binds[i].buffer = va_arg(ap, void *);
			binds[i].length = &lens[i];
			lens[i] = va_arg(ap, unsigned long);
			break;

		case GLITE_LBU_DB_TYPE_VARCHAR:
		case GLITE_LBU_DB_TYPE_CHAR:
			binds[i].buffer = va_arg(ap, char *);
			binds[i].length = &lens[i];
			lens[i] = binds[i].buffer ? strlen((char *)binds[i].buffer) : 0;
			break;

		case GLITE_LBU_DB_TYPE_DATE:
		case GLITE_LBU_DB_TYPE_TIME:
		case GLITE_LBU_DB_TYPE_DATETIME:
			ptime = binds[i].buffer = data[i] = malloc(sizeof(MYSQL_TIME));
			set_time(ptime, va_arg(ap, time_t));
			break;

		case GLITE_LBU_DB_TYPE_TIMESTAMP:
			ptime = binds[i].buffer = data[i] = malloc(sizeof(MYSQL_TIME));
			set_time(ptime, va_arg(ap, double));
			break;

		case GLITE_LBU_DB_TYPE_NULL:
			break;

		default:
			assert("unimplemented parameter assign" == NULL);
			break;
		}
		binds[i].buffer_type = glite_type_to_mysql[type];
	}

	prepare_retry = 2;
	do {
		// bind parameters
		if (n) {
			if (mysql_stmt_bind_param(stmt->stmt, binds) != 0) {
				MY_ERRSTMT(stmt);
				ret = -1;
				goto statement_failed;
			}
		}

		// run
		retry = 1;
		do {
			mysql_stmt_execute(stmt->stmt);
			ret = MY_ISOKSTMT(stmt, &retry);
		} while (ret == 0);
	statement_failed:
		if (ret == -1) {
			if (mysql_stmt_errno(stmt->stmt) == ER_UNKNOWN_STMT_HANDLER) {
				// expired the prepared command ==> restore it
				if (glite_lbu_PrepareStmtMysql(stmt->generic.ctx, stmt->sql, &newstmt) == -1) goto failed;
				glite_lbu_FreeStmt_int(stmt);
				memcpy(stmt, newstmt, sizeof(struct glite_lbu_StatementMysql_s));
				prepare_retry--;
				ret = 0;
			} else goto failed;
		}
	} while (ret == 0 && prepare_retry > 0);

	// result
	retry = 1;
	do {
		mysql_stmt_store_result(stmt->stmt);
		ret = MY_ISOKSTMT(stmt, &retry);
	} while (ret == 0);
	if (ret == -1) goto failed;

	// free params
	if (n) {
		for (i = 0; i < n; i++) free(data[i]);
		free(data);
		free(binds);
		free(lens);
	}
	CLR_ERR(stmt->generic.ctx);
	return mysql_stmt_affected_rows(stmt->stmt);

failed:
	for (i = 0; i < n; i++) free(data[i]);
	free(data);
	free(binds);
	free(lens);
	return -1;
}


long int glite_lbu_LastidMysql(glite_lbu_Statement stmt_gen) {
	glite_lbu_StatementMysql stmt = (glite_lbu_StatementMysql)stmt_gen;
	my_ulonglong i;

	CLR_ERR(stmt_gen->ctx);
	i = mysql_stmt_insert_id(stmt->stmt);
	assert(i < ((unsigned long int)-1) >> 1);
	return (long int)i;
}


/*
 * helping function: find oud mysql error and sets on the context
 */
static int myerr(glite_lbu_DBContextMysql ctx, const char *source, int line) {
	return glite_lbu_DBSetError(&ctx->generic, EIO, source, line, mysql_error(ctx->mysql));
}


/*
 * helping function: find oud mysql stmt error and sets on the context
 */
static int myerrstmt(glite_lbu_StatementMysql stmt, const char *source, int line) {	
	return glite_lbu_DBSetError(stmt->generic.ctx, EIO, source, line, mysql_stmt_error(stmt->stmt));
}


/*
 * helping function: error handle
 *
 * \return -1 failed
 * \return  0 retry
 * \return  1 OK
 */
static int myisokstmt(glite_lbu_StatementMysql stmt, const char *source, int line, int *retry) {
	switch (mysql_stmt_errno(stmt->stmt)) {
		case 0:
			return 1;
			break;
		case ER_DUP_ENTRY:
			glite_lbu_DBSetError(stmt->generic.ctx, EEXIST, source, line, mysql_stmt_error(stmt->stmt));
			return -1;
			break;
		case CR_SERVER_LOST:
		case CR_SERVER_GONE_ERROR:
			if (*retry > 0) {
				(*retry)--;
				return 0;
			} else {
				myerrstmt(stmt, source, line);
				return -1;
			}
			break;
		default:
			myerrstmt(stmt, source, line);
			return -1;
			break;
	}
}


/*
 * mysql connect
 */
static int db_connect(glite_lbu_DBContextMysql ctx, const char *cs, MYSQL **mysql) {
	char	*buf = NULL;
	char	*host,*user,*pw,*db; 
	char	*slash,*at,*colon;
	int      ret;
#if MYSQL_VERSION_ID >= 50013
	my_bool reconnect = 1;
#endif

	// needed for SQL result parameters
	assert(sizeof(int) >= sizeof(my_bool));

	if (!cs) return ERR(ctx, EINVAL, "connect string not specified");
	
	if (!(*mysql = mysql_init(NULL))) return ERR(ctx, ENOMEM, NULL);

	mysql_options(*mysql, MYSQL_READ_DEFAULT_FILE, "my");
#if MYSQL_VERSION_ID >= 50013
	/* XXX: may result in weird behaviour in the middle of transaction */
	mysql_options(*mysql, MYSQL_OPT_RECONNECT, &reconnect);
#endif

	host = user = pw = db = NULL;

	buf = strdup(cs);
	slash = strchr(buf,'/');
	at = strrchr(buf,'@');
	colon = strrchr(buf,':');

	if (!slash || !at || !colon) {
		free(buf);
		db_close(*mysql);
		*mysql = NULL;
		return ERR(ctx, EINVAL, "Invalid DB connect string");
	}

	*slash = *at = *colon = 0;
	host = at+1;
	user = buf;
	pw = slash+1;
	db = colon+1;

	/* ljocha: CLIENT_FOUND_ROWS added to make authorization check
	 * working in update_notif(). 
	 * Hope it does not break anything else */ 
	if (!mysql_real_connect(*mysql,host,user,pw,db,0,NULL,CLIENT_FOUND_ROWS)) {
		ret = MY_ERR(ctx);
		db_close(*mysql);
		*mysql = NULL;
		free(buf);
		return ret;
	}
	free(buf);

	return CLR_ERR(ctx);
}


/*
 * mysql close
 */
static void db_close(MYSQL *mysql) {
	if (mysql) mysql_close(mysql);
}


/*
 * test transactions capability:
 */
static int transaction_test(glite_lbu_DBContext ctx, int *caps) {
	glite_lbu_Statement stmt;
	char *table[1] = { NULL }, *res[2] = { NULL, NULL }, *cmd = NULL;
	int retval;

	(*caps) &= ~GLITE_LBU_DB_CAP_TRANSACTIONS;

	glite_common_log(ctx->log_category, LOG_PRIORITY_DEBUG, "SHOW TABLES");
	if ((retval = glite_lbu_ExecSQLMysql(ctx, "SHOW TABLES", &stmt)) <= 0 || glite_lbu_FetchRowMysql(stmt, 1, NULL, table) < 0) goto quit;
	glite_lbu_FreeStmtMysql(&stmt);

	trio_asprintf(&cmd, "SHOW CREATE TABLE %|Ss", table[0]);
	glite_common_log_msg(ctx->log_category, LOG_PRIORITY_DEBUG, cmd);
	if (glite_lbu_ExecSQLMysql(ctx, cmd, &stmt) <= 0 || (retval = glite_lbu_FetchRowMysql(stmt, 2, NULL, res)) < 0 ) goto quit;
	if (retval != 2 || strcmp(res[0], table[0])) {
		ERR(ctx, EIO, "unexpected show create result");
		goto quit;
	}

	if (strstr(res[1],"ENGINE=InnoDB"))
		(*caps) |= GLITE_LBU_DB_CAP_TRANSACTIONS;

#ifdef LBS_DB_PROFILE
	fprintf(stderr, "[%d] use_transactions = %d\n", getpid(), USE_TRANS(ctx));
#endif

quit:
	glite_lbu_FreeStmtMysql(&stmt);
	free(table[0]);
	free(res[0]);
	free(res[1]);
	free(cmd);
	return STATUS(ctx);
}


/*
 * simple version of the fetch
 */
static int FetchRowSimple(glite_lbu_DBContextMysql ctx, MYSQL_RES *result, unsigned long *lengths, char **results) {
	MYSQL_ROW            row;
	unsigned int         nr, i;
	unsigned long       *len;

	CLR_ERR(ctx);

	if (!(row = mysql_fetch_row(result))) {
		if (mysql_errno((MYSQL *) ctx->mysql)) {
			MY_ERR(ctx);
			return -1;
		} else return 0;
	}

	nr = mysql_num_fields(result);
	len = mysql_fetch_lengths(result);
	for (i=0; i<nr; i++) {
		if (lengths) lengths[i] = len[i];
		if (len[i]) {
			if ((results[i] = malloc(len[i] + 1)) == NULL)
				goto nomem;
			memcpy(results[i], row[i], len[i]);
			results[i][len[i]] = '\000';
		} else {
			if ((results[i] = strdup("")) == NULL)
				goto nomem;
		}
	}

	return nr;
nomem:
	for (nr = i, i = 0; i < nr; i++) {
		free(results[i]);
		results[i] = NULL;
	}
	ERR(ctx, ENOMEM, "insufficient memory for field data");
	return -1;
}


/*
 * prepared version of the fetch
 */
static int FetchRowPrepared(glite_lbu_DBContextMysql ctx, glite_lbu_StatementMysql stmt, unsigned int n, unsigned long *lengths, char **results) {
	int ret, retry;
	unsigned int i;
	MYSQL_BIND *binds = NULL;
	unsigned long *lens = NULL;

	if (n != stmt->nrfields) {
		ERR(ctx, EINVAL, "bad number of result fields");
		return -1;
	}

	// space for results
	if (n) binds = calloc(n, sizeof(MYSQL_BIND));
	if (!lengths) {
		lens = calloc(n, sizeof(unsigned long));
		lengths = lens;
	}
	for (i = 0; i < n; i++) {
		binds[i].buffer_type = MYSQL_TYPE_VAR_STRING;
		binds[i].buffer_length = GLITE_LBU_DEFAULT_RESULT_BUFFER_LENGTH - 1;
		binds[i].length = &lengths[i];
		if ((binds[i].buffer = results[i] = calloc(1, GLITE_LBU_DEFAULT_RESULT_BUFFER_LENGTH)) == NULL) {
			ERR(ctx, ENOMEM, "insufficient memory for field data");
			goto failed;
		}
	}
	if (mysql_stmt_bind_result(stmt->stmt, binds) != 0) goto failedstmt;

	// fetch data, all can be truncated
	retry = 1;
	do {
		switch(mysql_stmt_fetch(stmt->stmt)) {
#ifdef MYSQL_DATA_TRUNCATED
			case MYSQL_DATA_TRUNCATED:
#endif
			case 0:
				ret = 1; break;
			case 1: ret = MY_ISOKSTMT(stmt, &retry); break;
			case MYSQL_NO_DATA: ret = 0; goto quit; /* it's OK */
			default: ERR(ctx, EIO, "other fetch error"); goto failed;
		}
	} while (ret == 0);
	if (ret == -1) goto failed;

	// check if all fileds had enough buffer space
	for (i = 0; i < n; i++) {
		// fetch the rest if needed
		if (lengths[i] > binds[i].buffer_length) {
			unsigned int fetched;

			fetched = binds[i].buffer_length;
			if ((results[i] = realloc(results[i], lengths[i] + 1)) == NULL) {
				ERR(ctx, ENOMEM, "insufficient memory for field data");
				goto failed;
			}
			results[i][lengths[i]] = '\000';
			binds[i].buffer = results[i] + fetched;
			binds[i].buffer_length = lengths[i] - fetched;

			retry = 1;
			do {
				switch (mysql_stmt_fetch_column(stmt->stmt, binds + i, i, fetched)) {
					case 0: ret = 1; break;
					case 1: ret = MY_ISOKSTMT(stmt, &retry); break;
					case MYSQL_NO_DATA: ret = 0; goto quit; /* it's OK */
					default: ERR(ctx, EIO, "other fetch error"); goto failed;
				}
			} while (ret == 0);
			if (ret == -1) goto failed;
		}
	}

	CLR_ERR(ctx);
	free(binds);
	free(lens);
	return n;

failedstmt:
	MY_ERRSTMT(stmt);
failed:
	ret = -1;
quit:
	free(binds);
	free(lens);
	for (i = 0; i < n; i++) {
		free(results[i]);
		results[i] = NULL;
	}
	return ret;
}


static void set_time(MYSQL_TIME *mtime, const double time) {
	struct tm tm;
	time_t itime;

	itime = time;
	gmtime_r(&itime, &tm);
	memset(mtime, 0, sizeof *mtime);
	mtime->year = tm.tm_year + 1900;
	mtime->month = tm.tm_mon + 1;
	mtime->day = tm.tm_mday;
	mtime->hour = tm.tm_hour;
	mtime->minute = tm.tm_min;
	mtime->second = tm.tm_sec;
	mtime->second_part = (time - itime) * 1000;
}
