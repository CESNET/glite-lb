#ident "$Header$"

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <pthread.h>

#include <mysql.h>
#include <mysqld_error.h>
#include <mysql_version.h>
#include <errmsg.h>

#include "glite/lbu/trio.h"
#include "db.h"


#define GLITE_LBU_MYSQL_INDEX_VERSION 40001
#define GLITE_LBU_MYSQL_PREPARED_VERSION 40102
#define BUF_INSERT_ROW_ALLOC_BLOCK	1000
#ifndef GLITE_LBU_DEFAULT_RESULT_BUFFER_LENGTH
#define GLITE_LBU_DEFAULT_RESULT_BUFFER_LENGTH 256
#endif

#define CLR_ERR(CTX) lbu_clrerr((CTX))
#define ERR(CTX, CODE, ARGS...) lbu_err((CTX), (CODE), __FUNCTION__, __LINE__, ##ARGS)
#define STATUS(CTX) ((CTX)->err.code)
#define MY_ERR(CTX) myerr((CTX), __FUNCTION__, __LINE__)
#define MY_ERRSTMT(STMT) myerrstmt((STMT), __FUNCTION__, __LINE__)
#define MY_ISOKSTMT(STMT, RETRY) myisokstmt((STMT), __FUNCTION__, __LINE__, (RETRY))

#define USE_TRANS(CTX) ((CTX->caps & GLITE_LBU_DB_CAP_TRANSACTIONS) != 0)
#define LOAD(SYM, SYM2) if ((*(void **)(&db_handle.SYM) = dlsym(db_handle.lib, SYM2)) == NULL) { \
	err = ERR(*ctx, ENOENT, "can't load symbol %s from mysql library (%s)", SYM2, dlerror()); \
	break; \
}

#define dprintf(CTX, FMT...) if (CTX->caps & GLITE_LBU_DB_CAP_ERRORS) fprintf(stderr, ##FMT)


struct glite_lbu_DBContext_s {
	MYSQL *mysql;
	const char *cs;
	int caps;
	struct {
		int code;
		char *desc;
	} err;
};


struct glite_lbu_Statement_s {
	glite_lbu_DBContext  ctx;

	/* for simple commands */
	MYSQL_RES           *result;

	/* for prepared commands */
	MYSQL_STMT          *stmt;
	unsigned long        nrfields;
	char                *sql;
};


struct glite_lbu_bufInsert_s {
	glite_lbu_DBContext ctx;
	char	*table_name;
	char	*columns;	/* names of columns to be inserted into 
				 * (values separated with commas) */
	char	**rows;		/* each row hold string of one row to be inserted
				 * (values separated with commas) */
	long	rec_num, 	/* actual number of rows in structure */
		rec_size;	/* approx. size of a real insert string */
	long	size_limit, 	/* size and # of records limit which trigger */
		record_limit;	/* real insert; zero means unlimitted */
};


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


typedef struct {
	void *lib;
	pthread_mutex_t lock;

	void *(*mysql_init)(void *);
	unsigned long (*mysql_get_client_version)(void);
	int (*mysql_options)(MYSQL *mysql, enum mysql_option option, const char *arg);
	unsigned int (*mysql_errno)(MYSQL *mysql);
	const char *(*mysql_error)(MYSQL *mysql);
	unsigned int (*mysql_stmt_errno)(MYSQL_STMT *stmt);
	const char *(*mysql_stmt_error)(MYSQL_STMT *stmt);
	MYSQL *(*mysql_real_connect)(MYSQL *mysql, const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned long client_flag);
	void (*mysql_close)(MYSQL *mysql);
	int (*mysql_query)(MYSQL *mysql, const char *stmt_str);
	MYSQL_RES *(*mysql_store_result)(MYSQL *mysql);
	void (*mysql_free_result)(MYSQL_RES *result);
	my_ulonglong (*mysql_affected_rows)(MYSQL *mysql);
	my_bool (*mysql_stmt_close)(MYSQL_STMT *);
	unsigned int (*mysql_num_fields)(MYSQL_RES *result);
	unsigned long *(*mysql_fetch_lengths)(MYSQL_RES *result);
	my_bool (*mysql_stmt_bind_result)(MYSQL_STMT *stmt, MYSQL_BIND *bind);
	int (*mysql_stmt_prepare)(MYSQL_STMT *stmt, const char *stmt_str, unsigned long length);
	int (*mysql_stmt_store_result)(MYSQL_STMT *stmt);
	MYSQL_ROW (*mysql_fetch_row)(MYSQL_RES *result);
	MYSQL_FIELD *(*mysql_fetch_field)(MYSQL_RES *result);
	const char *(*mysql_get_server_info)(MYSQL *mysql);
	MYSQL_STMT *(*mysql_stmt_init)(MYSQL *mysql);
	my_bool (*mysql_stmt_bind_param)(MYSQL_STMT *stmt, MYSQL_BIND *bind);
	int (*mysql_stmt_execute)(MYSQL_STMT *stmt);
	int (*mysql_stmt_fetch)(MYSQL_STMT *stmt);
	my_ulonglong (*mysql_stmt_insert_id)(MYSQL_STMT *stmt);
	my_ulonglong (*mysql_stmt_affected_rows)(MYSQL_STMT *stmt);
	MYSQL_RES *(*mysql_stmt_result_metadata)(MYSQL_STMT *stmt);
	int (*mysql_stmt_fetch_column)(MYSQL_STMT *stmt, MYSQL_BIND *bind, unsigned int column, unsigned long offset);
} mysql_lib_handle_t;


static mysql_lib_handle_t db_handle = {
	lib: NULL,
	lock: PTHREAD_MUTEX_INITIALIZER
};


static int lbu_clrerr(glite_lbu_DBContext ctx);
static int lbu_err(glite_lbu_DBContext ctx, int code, const char *func, int line, const char *desc, ...);
static int myerr(glite_lbu_DBContext ctx, const char *source, int line);
static int myerrstmt(glite_lbu_Statement stmt, const char *source, int line);
static int myisokstmt(glite_lbu_Statement stmt, const char *source, int line, int *retry);
static int db_connect(glite_lbu_DBContext ctx, const char *cs, MYSQL **mysql);
static void db_close(MYSQL *mysql);
static int transaction_test(glite_lbu_DBContext ctx);
static int FetchRowSimple(glite_lbu_DBContext ctx, MYSQL_RES *result, unsigned long *lengths, char **results);
static int FetchRowPrepared(glite_lbu_DBContext ctx, glite_lbu_Statement stmt, unsigned int n, unsigned long *lengths, char **results);
static void set_time(MYSQL_TIME *mtime, const time_t time);
static void glite_lbu_DBCleanup(void);
static void glite_lbu_FreeStmt_int(glite_lbu_Statement stmt);


/* ---- common ---- */


int glite_lbu_DBError(glite_lbu_DBContext ctx, char **text, char **desc) {
	if (text) *text = strdup(strerror(ctx->err.code));
	if (desc) {
		if (ctx->err.desc) *desc = strdup(ctx->err.desc);
		else *desc = NULL;
	}

	return ctx->err.code;
}


int glite_lbu_InitDBContext(glite_lbu_DBContext *ctx) {
	int err = 0;
	unsigned int ver_u;

	*ctx = calloc(1, sizeof **ctx);
	if (!*ctx) return ENOMEM;

	/* dynamic load the mysql library */
	pthread_mutex_lock(&db_handle.lock);
	if (!db_handle.lib) {
		if ((!MYSQL_LIBPATH[0] || (db_handle.lib = dlopen(MYSQL_LIBPATH, RTLD_LAZY | RTLD_LOCAL)) == NULL) &&
		    (db_handle.lib = dlopen("libmysqlclient.so", RTLD_LAZY | RTLD_LOCAL)) == NULL)
			return ERR(*ctx, ENOENT, "can't load '%s' or 'libmysqlclient.so' (%s)", MYSQL_LIBPATH, dlerror());
		do {
			LOAD(mysql_init, "mysql_init");
			LOAD(mysql_get_client_version, "mysql_get_client_version");
			LOAD(mysql_options, "mysql_options");
			LOAD(mysql_errno, "mysql_errno");
			LOAD(mysql_error, "mysql_error");
			LOAD(mysql_stmt_errno, "mysql_stmt_errno");
			LOAD(mysql_stmt_error, "mysql_stmt_error");
			LOAD(mysql_real_connect, "mysql_real_connect");
			LOAD(mysql_close, "mysql_close");
			LOAD(mysql_query, "mysql_query");
			LOAD(mysql_store_result, "mysql_store_result");
			LOAD(mysql_free_result, "mysql_free_result");
			LOAD(mysql_affected_rows, "mysql_affected_rows");
			LOAD(mysql_stmt_close, "mysql_stmt_close");
			LOAD(mysql_num_fields, "mysql_num_fields");
			LOAD(mysql_fetch_lengths, "mysql_fetch_lengths");
			LOAD(mysql_stmt_bind_result, "mysql_stmt_bind_result");
			LOAD(mysql_stmt_prepare, "mysql_stmt_prepare");
			LOAD(mysql_stmt_store_result, "mysql_stmt_store_result");
			LOAD(mysql_fetch_row, "mysql_fetch_row");
			LOAD(mysql_fetch_field, "mysql_fetch_field");
			LOAD(mysql_get_server_info, "mysql_get_server_info");
			LOAD(mysql_stmt_init, "mysql_stmt_init");
			LOAD(mysql_stmt_bind_param, "mysql_stmt_bind_param");
			LOAD(mysql_stmt_execute, "mysql_stmt_execute");
			LOAD(mysql_stmt_fetch, "mysql_stmt_fetch");
			LOAD(mysql_stmt_insert_id, "mysql_stmt_insert_id");
			LOAD(mysql_stmt_affected_rows, "mysql_stmt_affected_rows");
			LOAD(mysql_stmt_result_metadata, "mysql_stmt_result_metadata");
			LOAD(mysql_stmt_fetch_column, "mysql_stmt_fetch_column");

			// check the runtime version
			ver_u = db_handle.mysql_get_client_version();
			if (ver_u != MYSQL_VERSION_ID) {
				err = ERR(*ctx, EINVAL, "version mismatch (compiled '%lu', runtime '%lu')", MYSQL_VERSION_ID, ver_u);
				break;
			}

			pthread_mutex_unlock(&db_handle.lock);
			atexit(glite_lbu_DBCleanup);
		} while(0);

		if (err) {
			dlclose(db_handle.lib);
			db_handle.lib = NULL;
			pthread_mutex_unlock(&db_handle.lock);
			return err;
		}
	} else pthread_mutex_unlock(&db_handle.lock);

	return 0;
}


void glite_lbu_FreeDBContext(glite_lbu_DBContext ctx) {
	if (ctx) {
		assert(ctx->mysql == NULL);
		free(ctx->err.desc);
		free(ctx);
	}
}


int glite_lbu_DBConnect(glite_lbu_DBContext ctx, const char *cs) {
	if (db_connect(ctx, cs, &ctx->mysql) != 0 ||
	    glite_lbu_ExecSQL(ctx, "SET AUTOCOMMIT=1", NULL) < 0 ||
	    glite_lbu_ExecSQL(ctx, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ", NULL) < 0)
		return STATUS(ctx);
	else
		return 0;
}


void glite_lbu_DBClose(glite_lbu_DBContext ctx) {
	db_close(ctx->mysql);
	ctx->mysql = NULL;
	CLR_ERR(ctx);
}


int glite_lbu_DBQueryCaps(glite_lbu_DBContext ctx) {
	MYSQL	*m = ctx->mysql;
	int	major,minor,sub,version,caps,origcaps;
	const char *ver_s;

	origcaps = ctx->caps;
	caps = 0;

	ver_s = db_handle.mysql_get_server_info(m);
	if (!ver_s || 3 != sscanf(ver_s,"%d.%d.%d",&major,&minor,&sub))
		return ERR(ctx, EINVAL, "problem retreiving MySQL version");
	version = 10000*major + 100*minor + sub;

	if (version >= GLITE_LBU_MYSQL_INDEX_VERSION) caps |= GLITE_LBU_DB_CAP_INDEX;
	if (version >= GLITE_LBU_MYSQL_PREPARED_VERSION) caps |= GLITE_LBU_DB_CAP_PREPARED;

	CLR_ERR(ctx);
	transaction_test(ctx);
	if ((ctx->caps & GLITE_LBU_DB_CAP_TRANSACTIONS)) caps |= GLITE_LBU_DB_CAP_TRANSACTIONS;

	ctx->caps = origcaps;
	if (STATUS(ctx) == 0) return caps;
	else return -1;
}


void glite_lbu_DBSetCaps(glite_lbu_DBContext ctx, int caps) {
	ctx->caps = caps;
}


int glite_lbu_Transaction(glite_lbu_DBContext ctx) {
	CLR_ERR(ctx);
	if (USE_TRANS(ctx)) {
		if (glite_lbu_ExecSQL(ctx, "SET AUTOCOMMIT=0", NULL) < 0) goto err;
		if (glite_lbu_ExecSQL(ctx, "BEGIN", NULL) < 0) goto err;
	}
err:
	return STATUS(ctx);
}


int glite_lbu_Commit(glite_lbu_DBContext ctx) {
	CLR_ERR(ctx);
	if (USE_TRANS(ctx)) {
		if (glite_lbu_ExecSQL(ctx, "COMMIT", NULL) < 0) goto err;
		if (glite_lbu_ExecSQL(ctx, "SET AUTOCOMMIT=1", NULL) < 0) goto err;
	}
err:
	return STATUS(ctx);
}


int glite_lbu_Rollback(glite_lbu_DBContext ctx) {
	CLR_ERR(ctx);
	if (USE_TRANS(ctx)) { 
		if (glite_lbu_ExecSQL(ctx, "ROLLBACK", NULL) < 0) goto err;
		if (glite_lbu_ExecSQL(ctx, "SET AUTOCOMMIT=1", NULL) < 0) goto err;
	}
err:
	return STATUS(ctx);
}


int glite_lbu_FetchRow(glite_lbu_Statement stmt, unsigned int n, unsigned long *lengths, char **results) {
	memset(results, 0, n * sizeof(*results));
	if (stmt->result) return FetchRowSimple(stmt->ctx, stmt->result, lengths, results);
	else return FetchRowPrepared(stmt->ctx, stmt, n, lengths, results);
}


static void glite_lbu_FreeStmt_int(glite_lbu_Statement stmt) {
	if (stmt) {
		if (stmt->result) db_handle.mysql_free_result(stmt->result);
		if (stmt->stmt) db_handle.mysql_stmt_close(stmt->stmt);
		free(stmt->sql);
	}
}


void glite_lbu_FreeStmt(glite_lbu_Statement *stmt) {
	glite_lbu_FreeStmt_int(*stmt);
	free(*stmt);
	*stmt = NULL;
}


int glite_lbu_QueryIndices(glite_lbu_DBContext ctx, const char *table, char ***key_names, char ****column_names) {
	glite_lbu_Statement       stmt = NULL;

	int	i,j,ret;

/* XXX: "show index from" columns. Matches at least MySQL 4.0.11 */
	char	*sql, *showcol[12];
	int	Key_name,Seq_in_index,Column_name,Sub_part;

	char	**keys = NULL;
	int	*cols = NULL;
	char	**col_names = NULL;

	int	nkeys = 0;

	char	***idx = NULL;

	Key_name = Seq_in_index = Column_name = Sub_part = -1;

	asprintf(&sql, "show index from %s", table);
	if (glite_lbu_ExecSQL(ctx,sql,&stmt)<0) {
		free(sql);
		return STATUS(ctx);
	}
	free(sql);

	while ((ret = glite_lbu_FetchRow(stmt,sizeof(showcol)/sizeof(showcol[0]),NULL,showcol)) > 0) {
		assert(ret <= (int)(sizeof showcol/sizeof showcol[0]));

		if (!col_names) {
			col_names = malloc(ret * sizeof col_names[0]);
			glite_lbu_QueryColumns(stmt,col_names);
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
			keys[i] = showcol[Key_name];
//printf("** KEY [%d] %s\n", i, keys[i]);
			keys[i+1] = NULL;
			cols = realloc(cols,(i+1) * sizeof cols[0]); 
			cols[i] = 0;
			idx = realloc(idx,(i+2) * sizeof idx[0]);
			idx[i] = idx[i+1] = NULL;
			showcol[Key_name] = NULL;
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
		for (i = 0; showcol[i]; i++) free(showcol[i]);
	}

	glite_lbu_FreeStmt(&stmt);
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
	else free(keys);
	*column_names = idx;

	return STATUS(ctx);
}


/* ---- simple ---- */

int glite_lbu_ExecSQL(glite_lbu_DBContext ctx, const char *cmd, glite_lbu_Statement *stmt) {
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

	if (stmt) *stmt = NULL;

#ifdef LBS_DB_PROFILE
	gettimeofday(&start,NULL);
#endif

	while (retry_nr == 0 || do_reconnect) {
		do_reconnect = 0;
		if (db_handle.mysql_query(ctx->mysql, cmd)) {
			/* error occured */
			switch (merr = db_handle.mysql_errno(ctx->mysql)) {
				case 0:
					break;
				case ER_DUP_ENTRY: 
					ERR(ctx, EEXIST, db_handle.mysql_error(ctx->mysql));
					return -1;
					break;
				case CR_SERVER_LOST:
				case CR_SERVER_GONE_ERROR:
					if (retry_nr <= 0) 
						do_reconnect = 1;
					break;
				case ER_LOCK_DEADLOCK:
					ERR(ctx, EDEADLOCK, db_handle.mysql_error(ctx->mysql));
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

	if (stmt) {
		*stmt = calloc(1, sizeof(**stmt));
		if (!*stmt) {
			ERR(ctx, ENOMEM, NULL);
			return -1;
		}
		(**stmt).ctx = ctx;
		(**stmt).result = db_handle.mysql_store_result(ctx->mysql);
		if (!(**stmt).result) {
			if (db_handle.mysql_errno(ctx->mysql)) {
				MY_ERR(ctx);
				*stmt = NULL;
				return -1;
			}
		}
	} else {
		MYSQL_RES	*r = db_handle.mysql_store_result(ctx->mysql);
		db_handle.mysql_free_result(r);
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

	return db_handle.mysql_affected_rows(ctx->mysql);
}


int glite_lbu_QueryColumns(glite_lbu_Statement stmt, char **cols)
{
	int	i = 0;
	MYSQL_FIELD 	*f;

	CLR_ERR(stmt->ctx);
	if (!stmt->result) return ERR(stmt->ctx, EINVAL, "QueryColumns implemented only for simple API");
	while ((f = db_handle.mysql_fetch_field(stmt->result))) cols[i++] = f->name;
	return i == 0;
}


void glite_lbu_TimeToDB(time_t t, char **str) {
	struct tm	*tm = gmtime(&t);

	asprintf(str,"'%4d-%02d-%02d %02d:%02d:%02d'",tm->tm_year+1900,tm->tm_mon+1,
		tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
}


time_t glite_lbu_DBToTime(const char *str) {
	struct tm	tm;

	memset(&tm,0,sizeof(tm));
	setenv("TZ","UTC",1); tzset();
	sscanf(str,"%4d-%02d-%02d %02d:%02d:%02d",
		&tm.tm_year,&tm.tm_mon,&tm.tm_mday,
		&tm.tm_hour,&tm.tm_min,&tm.tm_sec);
	tm.tm_year -= 1900;
	tm.tm_mon--;

	return mktime(&tm);
}

/* ---- prepared --- */

int glite_lbu_PrepareStmt(glite_lbu_DBContext ctx, const char *sql, glite_lbu_Statement *stmt) {
	int ret, retry;
	MYSQL_RES *meta;

	// init
	*stmt = calloc(1, sizeof(**stmt));
	(*stmt)->ctx = ctx;

	// create the SQL command
	if (((*stmt)->stmt = db_handle.mysql_stmt_init(ctx->mysql)) == NULL)
		return MY_ERRSTMT(*stmt);

	// prepare the SQL command
	retry = 1;
	do {
		db_handle.mysql_stmt_prepare((*stmt)->stmt, sql, strlen(sql));
		ret = MY_ISOKSTMT(*stmt, &retry);
	} while (ret == 0);
	if (ret == -1) goto failed;

	// number of fields (0 for no results)
	if ((meta = db_handle.mysql_stmt_result_metadata((*stmt)->stmt)) != NULL) {
		(*stmt)->nrfields = db_handle.mysql_num_fields(meta);
		db_handle.mysql_free_result(meta);
	} else
		(*stmt)->nrfields = 0;

	// remember the command
	(*stmt)->sql = strdup(sql);

	return CLR_ERR(ctx);

failed:
	glite_lbu_FreeStmt(stmt);
	return STATUS(ctx);
}


int glite_lbu_ExecPreparedStmt_v(glite_lbu_Statement stmt, int n, va_list ap) {
	int i, prepare_retry;
	glite_lbu_DBType type;
	char *pchar;
	long int *plint;
	MYSQL_TIME *ptime;
	glite_lbu_DBContext ctx;
	int ret, retry;
	MYSQL_BIND *binds = NULL;
	void **data = NULL;
	unsigned long *lens;
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
		case GLITE_LBU_DB_TYPE_TIMESTAMP:
			ptime = binds[i].buffer = data[i] = malloc(sizeof(MYSQL_TIME));
			set_time(ptime, va_arg(ap, time_t));
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
			if (db_handle.mysql_stmt_bind_param(stmt->stmt, binds) != 0) {
				MY_ERRSTMT(stmt);
				ret = -1;
				goto statement_failed;
			}
		}

		// run
		ctx = stmt->ctx;
		retry = 1;
		do {
			db_handle.mysql_stmt_execute(stmt->stmt);
			ret = MY_ISOKSTMT(stmt, &retry);
		} while (ret == 0);
	statement_failed:
		if (ret == -1) {
			if (db_handle.mysql_stmt_errno(stmt->stmt) == ER_UNKNOWN_STMT_HANDLER) {
				// expired the prepared command ==> restore it
				if (glite_lbu_PrepareStmt(stmt->ctx, stmt->sql, &newstmt) == -1) goto failed;
				glite_lbu_FreeStmt_int(stmt);
				memcpy(stmt, newstmt, sizeof(struct glite_lbu_Statement_s));
				prepare_retry--;
				ret = 0;
			} else goto failed;
		}
	} while (ret == 0 && prepare_retry > 0);

	// result
	retry = 1;
	do {
		db_handle.mysql_stmt_store_result(stmt->stmt);
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
	CLR_ERR(ctx);
	return db_handle.mysql_stmt_affected_rows(stmt->stmt);

failed:
	for (i = 0; i < n; i++) free(data[i]);
	free(data);
	free(binds);
	free(lens);
	return -1;
}


int glite_lbu_ExecPreparedStmt(glite_lbu_Statement stmt, int n, ...) {
	va_list ap;
	int retval;

	va_start(ap, n);
	retval = glite_lbu_ExecPreparedStmt_v(stmt, n, ap);
	va_end(ap);

	return retval;
}


int glite_lbu_bufferedInsertInit(glite_lbu_DBContext ctx, glite_lbu_bufInsert *bi, const char *table_name, long size_limit, long record_limit, const char *columns)
{
	*bi = calloc(1, sizeof(*bi));
	(*bi)->ctx = ctx;
	(*bi)->table_name = strdup(table_name);
	(*bi)->columns = strdup(columns);
	(*bi)->rec_num = 0;
	(*bi)->rec_size = 0;
	(*bi)->rows = calloc(record_limit, sizeof(*((*bi)->rows)) );
	(*bi)->size_limit = size_limit;
	(*bi)->record_limit = record_limit;

        return CLR_ERR(ctx);
}


static int flush_bufferd_insert(glite_lbu_bufInsert bi)
{
	char *stmt, *vals, *temp;
	long i;


	if (!bi->rec_num)
		return STATUS(bi->ctx);

	asprintf(&vals,"(%s)", bi->rows[0]);
	for (i=1; i < bi->rec_num; i++) {
		// XXX:  use string add (preallocated memory)
		asprintf(&temp,"%s,(%s)", vals, bi->rows[i]);
		free(vals); vals = temp; temp = NULL;
		free(bi->rows[i]);
		bi->rows[i] = NULL;
	}
	
	trio_asprintf(&stmt, "insert into %|Ss(%|Ss) values %s;",
		bi->table_name, bi->columns, vals);

	if (glite_lbu_ExecSQL(bi->ctx,stmt,NULL) < 0) {
                if (STATUS(bi->ctx) == EEXIST)
                        CLR_ERR(bi->ctx);
        }

	/* reset bi counters */
	bi->rec_size = 0;
	bi->rec_num = 0;
	
	free(vals);
	free(stmt);

	return STATUS(bi->ctx);
}


int glite_lbu_bufferedInsert(glite_lbu_bufInsert bi, const char *row)
{
	bi->rows[bi->rec_num++] = strdup(row);
	bi->rec_size += strlen(row);

	if ((bi->size_limit && bi->rec_size >= bi->size_limit) ||
		(bi->record_limit && bi->rec_num >= bi->record_limit))
	{
		if (flush_bufferd_insert(bi))
			return STATUS(bi->ctx);
	}

	return CLR_ERR(bi->ctx);
}


static void free_buffered_insert(glite_lbu_bufInsert bi) {
	long i;

	free(bi->table_name);
	free(bi->columns);
	for (i=0; i < bi->rec_num; i++) {
		free(bi->rows[i]);
	}
	free(bi->rows);
}


int glite_lbu_bufferedInsertClose(glite_lbu_bufInsert bi)
{
	if (flush_bufferd_insert(bi))
		return STATUS(bi->ctx);
	free_buffered_insert(bi);

	return CLR_ERR(bi->ctx);
}


long int glite_lbu_Lastid(glite_lbu_Statement stmt) {
	my_ulonglong i;

	CLR_ERR(stmt->ctx);
	i = db_handle.mysql_stmt_insert_id(stmt->stmt);
	assert(i < ((unsigned long int)-1) >> 1);
	return (long int)i;
}


/*
 * helping compatibility function: clear error from the context
 */
static int lbu_clrerr(glite_lbu_DBContext ctx) {
	ctx->err.code = 0;
	if (ctx->err.desc) {
		free(ctx->err.desc);
		ctx->err.desc = NULL;
	}
	return 0;
}


/*
 * helping compatibility function: sets error on the context
 */
static int lbu_err(glite_lbu_DBContext ctx, int code, const char *func, int line, const char *desc, ...) {
	va_list ap;

	if (code) {
		ctx->err.code = code;
		free(ctx->err.desc);
		if (desc) {
			va_start(ap, desc);
			vasprintf(&ctx->err.desc, desc, ap);
			va_end(ap);
		} else
			ctx->err.desc = NULL;
		dprintf(ctx, "[db %d] %s:%d %s\n", getpid(), func, line, desc);
		return code;
	} else
		return ctx->err.code;
}


/*
 * helping function: find oud mysql error and sets on the context
 */
static int myerr(glite_lbu_DBContext ctx, const char *source, int line) {
	return lbu_err(ctx, EIO, source, line, db_handle.mysql_error(ctx->mysql));
}


/*
 * helping function: find oud mysql stmt error and sets on the context
 */
static int myerrstmt(glite_lbu_Statement stmt, const char *source, int line) {	
	return lbu_err(stmt->ctx, EIO, source, line, db_handle.mysql_stmt_error(stmt->stmt));
}


/*
 * helping function: error handle
 *
 * \return -1 failed
 * \return  0 retry
 * \return  1 OK
 */
static int myisokstmt(glite_lbu_Statement stmt, const char *source, int line, int *retry) {
	switch (db_handle.mysql_stmt_errno(stmt->stmt)) {
		case 0:
			return 1;
			break;
		case ER_DUP_ENTRY:
			lbu_err(stmt->ctx, EEXIST, source, line, db_handle.mysql_stmt_error(stmt->stmt));
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
static int db_connect(glite_lbu_DBContext ctx, const char *cs, MYSQL **mysql) {
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
	
	if (!(*mysql = db_handle.mysql_init(NULL))) return ERR(ctx, ENOMEM, NULL);

	db_handle.mysql_options(*mysql, MYSQL_READ_DEFAULT_FILE, "my");
#if MYSQL_VERSION_ID >= 50013
	/* XXX: may result in weird behaviour in the middle of transaction */
	db_handle.mysql_options(*mysql, MYSQL_OPT_RECONNECT, &reconnect);
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
	if (!db_handle.mysql_real_connect(*mysql,host,user,pw,db,0,NULL,CLIENT_FOUND_ROWS)) {
		char *desc;
		free(buf);
		ret = MY_ERR(ctx);
 		desc = ctx->err.desc;
		ctx->err.desc = NULL;
		glite_lbu_DBClose(ctx);
		ctx->err.desc = desc;
		return ctx->err.code = ret;
	}
	free(buf);

	ctx->cs = cs;
	return CLR_ERR(ctx);
}


/*
 * mysql close
 */
static void db_close(MYSQL *mysql) {
	if (mysql) db_handle.mysql_close(mysql);
}


/*
 * test transactions capability:
 */
static int transaction_test(glite_lbu_DBContext ctx) {
	glite_lbu_Statement stmt;
	char *table[1] = { NULL }, *res[2] = { NULL, NULL }, *cmd = NULL;
	int retval;

	ctx->caps &= ~GLITE_LBU_DB_CAP_TRANSACTIONS;

	if ((retval = glite_lbu_ExecSQL(ctx, "SHOW TABLES", &stmt)) <= 0 || glite_lbu_FetchRow(stmt, 1, NULL, table) < 0) goto quit;
	glite_lbu_FreeStmt(&stmt);

	trio_asprintf(&cmd, "SHOW CREATE TABLE %|Ss", table[0]);
	if (glite_lbu_ExecSQL(ctx, cmd, &stmt) <= 0 || (retval = glite_lbu_FetchRow(stmt, 2, NULL, res)) < 0 ) goto quit;
	if (retval != 2 || strcmp(res[0], table[0])) {
		ERR(ctx, EIO, "unexpected show create result");
		goto quit;
	}

	if (strstr(res[1],"ENGINE=InnoDB"))
		ctx->caps |= GLITE_LBU_DB_CAP_TRANSACTIONS;

#ifdef LBS_DB_PROFILE
	fprintf(stderr, "[%d] use_transactions = %d\n", getpid(), USE_TRANS(ctx));
#endif

quit:
	glite_lbu_FreeStmt(&stmt);
	free(table[0]);
	free(res[0]);
	free(res[1]);
	free(cmd);
	return STATUS(ctx);
}


/*
 * simple version of the fetch
 */
static int FetchRowSimple(glite_lbu_DBContext ctx, MYSQL_RES *result, unsigned long *lengths, char **results) {
	MYSQL_ROW            row;
	unsigned int         nr, i;
	unsigned long       *len;

	CLR_ERR(ctx);

	if (!(row = db_handle.mysql_fetch_row(result))) {
		if (db_handle.mysql_errno((MYSQL *) ctx->mysql)) {
			MY_ERR(ctx);
			return -1;
		} else return 0;
	}

	nr = db_handle.mysql_num_fields(result);
	len = db_handle.mysql_fetch_lengths(result);
	for (i=0; i<nr; i++) {
		if (lengths) lengths[i] = len[i];
		if (len[i]) {
			results[i] = malloc(len[i] + 1);
			memcpy(results[i], row[i], len[i]);
			results[i][len[i]] = '\000';
		} else
			results[i] = strdup("");
	}

	return nr;
}


/*
 * prepared version of the fetch
 */
static int FetchRowPrepared(glite_lbu_DBContext ctx, glite_lbu_Statement stmt, unsigned int n, unsigned long *lengths, char **results) {
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
		binds[i].buffer = results[i] = calloc(1, GLITE_LBU_DEFAULT_RESULT_BUFFER_LENGTH);
	}
	if (db_handle.mysql_stmt_bind_result(stmt->stmt, binds) != 0) goto failedstmt;

	// fetch data, all can be truncated
	retry = 1;
	do {
		switch(db_handle.mysql_stmt_fetch(stmt->stmt)) {
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
				switch (db_handle.mysql_stmt_fetch_column(stmt->stmt, binds + i, i, fetched)) {
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


static void set_time(MYSQL_TIME *mtime, const time_t time) {
	struct tm tm;

	gmtime_r(&time, &tm);
	memset(mtime, 0, sizeof *mtime);
	mtime->year = tm.tm_year + 1900;
	mtime->month = tm.tm_mon + 1;
	mtime->day = tm.tm_mday;
	mtime->hour = tm.tm_hour;
	mtime->minute = tm.tm_min;
	mtime->second = tm.tm_sec;
}


static void glite_lbu_DBCleanup(void) {
	pthread_mutex_lock(&db_handle.lock);
	if (db_handle.lib) {
		dlclose(db_handle.lib);
		db_handle.lib = NULL;
	}
	pthread_mutex_unlock(&db_handle.lock);
}


