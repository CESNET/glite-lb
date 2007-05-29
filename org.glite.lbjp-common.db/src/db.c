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

#include <mysql.h>
#include <mysqld_error.h>
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
#define ERR(CTX, CODE, DESC) lbu_err((CTX), (CODE), (DESC), __FUNCTION__, __LINE__)
#define STATUS(CTX) ((CTX)->err.code)
#define MY_ERR(CTX) myerr((CTX), __FUNCTION__, __LINE__)
#define MY_ERRSTMT(STMT) myerrstmt((STMT), __FUNCTION__, __LINE__)
#define MY_ISOKSTMT(STMT, RETRY) myisokstmt((STMT), __FUNCTION__, __LINE__, (RETRY))

#define USE_TRANS(CTX) ((CTX->caps & GLITE_LBU_DB_CAP_TRANSACTIONS) != 0)

#define dprintf(CTX, FMT...) if (CTX->caps & GLITE_LBU_DB_CAP_ERRORS) fprintf(stderr, ##FMT)


struct glite_lbu_DBContext_s {
	MYSQL *mysql;
	const char *cs;
	int have_caps;
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



static int lbu_clrerr(glite_lbu_DBContext ctx);
static int lbu_err(glite_lbu_DBContext ctx, int code, const char *desc, const char *func, int line);
static int myerr(glite_lbu_DBContext ctx, const char *source, int line);
static int myerrstmt(glite_lbu_Statement stmt, const char *source, int line);
static int myisokstmt(glite_lbu_Statement stmt, const char *source, int line, int *retry);
static int db_connect(glite_lbu_DBContext ctx, const char *cs, MYSQL **mysql);
static void db_close(MYSQL *mysql);
static int transaction_test(glite_lbu_DBContext ctx, MYSQL *m2, int *have_transactions);
static int FetchRowSimple(glite_lbu_DBContext ctx, MYSQL_RES *result, unsigned long *lengths, char **results);
static int FetchRowPrepared(glite_lbu_DBContext ctx, glite_lbu_Statement stmt, unsigned int n, unsigned long *lengths, char **results);
void set_time(MYSQL_TIME *mtime, const time_t time);
time_t get_time(const MYSQL_TIME *mtime);


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
	*ctx = calloc(1, sizeof **ctx);
	return *ctx == NULL ? ENOMEM : 0;
}


void glite_lbu_FreeDBContext(glite_lbu_DBContext ctx) {
	if (ctx) {
		assert(ctx->mysql == NULL);
		free(ctx->err.desc);
		free(ctx);
	}
}


int glite_lbu_DBConnect(glite_lbu_DBContext ctx, const char *cs) {
	if (db_connect(ctx, cs, &ctx->mysql) != 0) return STATUS(ctx);
	return 0;
}


void glite_lbu_DBClose(glite_lbu_DBContext ctx) {
	db_close(ctx->mysql);
	ctx->mysql = NULL;
}


int glite_lbu_DBQueryCaps(glite_lbu_DBContext ctx) {
	MYSQL	*m = ctx->mysql;
	MYSQL	*m2;
	int	major,minor,sub,version,caps,have_transactions=0;
	const char *ver_s;

	if (ctx->have_caps) return ctx->caps;

	caps = 0;

	ver_s = mysql_get_server_info(m);
	if (!ver_s || 3 != sscanf(ver_s,"%d.%d.%d",&major,&minor,&sub))
		return ERR(ctx, EINVAL, "problem retreiving MySQL version");
	version = 10000*major + 100*minor + sub;

	if (version >= GLITE_LBU_MYSQL_INDEX_VERSION) caps |= GLITE_LBU_DB_CAP_INDEX;
	if (version >= GLITE_LBU_MYSQL_PREPARED_VERSION) caps |= GLITE_LBU_DB_CAP_PREPARED;

	CLR_ERR(ctx);

	if (db_connect(ctx, ctx->cs, &m2) == 0) {
		transaction_test(ctx, m2, &have_transactions);
		db_close(m2);
	}
	if (have_transactions) caps |= GLITE_LBU_DB_CAP_TRANSACTIONS;

	if (STATUS(ctx) == 0) {
		ctx->have_caps = 1;
		return caps;
	} else return -1;
}


void glite_lbu_DBSetCaps(glite_lbu_DBContext ctx, int caps) {
	ctx->caps = caps;
}


int glite_lbu_Transaction(glite_lbu_DBContext ctx) {
	if (USE_TRANS(ctx)) {
		if (glite_lbu_ExecSQL(ctx, "SET AUTOCOMMIT=0", NULL) < 0) goto err;
		if (glite_lbu_ExecSQL(ctx, "BEGIN", NULL) < 0) goto err;
	}
err:
	return STATUS(ctx);
}


int glite_lbu_Commit(glite_lbu_DBContext ctx) {
	if (USE_TRANS(ctx)) {
		if (glite_lbu_ExecSQL(ctx, "COMMIT", NULL) < 0) goto err;
		if (glite_lbu_ExecSQL(ctx, "SET AUTOCOMMIT=1", NULL) < 0) goto err;
	}
err:
	return STATUS(ctx);
}


int glite_lbu_Rollback(glite_lbu_DBContext ctx) {
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


void glite_lbu_FreeStmt(glite_lbu_Statement *stmt) {
	if (*stmt) {
		if ((*stmt)->result) mysql_free_result((*stmt)->result);
		if ((*stmt)->stmt) mysql_stmt_close((*stmt)->stmt);
		free(*stmt);
		*stmt = NULL;
	}
}


int glite_lbu_QueryIndices(glite_lbu_DBContext ctx, const char *table, char ***key_names, char ****column_names) {
	glite_lbu_Statement       stmt = NULL;

	int	i,j,ret;

/* XXX: "show index from" columns. Matches at least MySQL 4.0.11 */
	char	*showcol[12];
	int	Key_name,Seq_in_index,Column_name,Sub_part;

	char	**keys = NULL;
	int	*cols = NULL;
	char	**col_names = NULL;

	int	nkeys = 0;

	char	***idx = NULL;

	Key_name = Seq_in_index = Column_name = Sub_part = -1;

	if (glite_lbu_ExecSQL(ctx,"show index from states",&stmt)<0) 
		return STATUS(ctx);

	while ((ret = glite_lbu_FetchRow(stmt,sizeof(showcol)/sizeof(showcol[0]),NULL,showcol)) > 0) {
		assert(ret <= sizeof showcol/sizeof showcol[0]);

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
					if (retry_nr <= 0) 
						do_reconnect = 1;
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
		(**stmt).result = mysql_store_result(ctx->mysql);
		if (!(**stmt).result) {
			if (mysql_errno(ctx->mysql)) {
				MY_ERR(ctx);
				*stmt = NULL;
				return -1;
			}
		}
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
	fprintf(stderr,"[%d] %s\n[%d] %3ld.%06ld (sum: %3ld.%06ld)\n",pid,txt,pid,end.tv_sec,end.tv_usec,sum.tv_sec,sum.tv_usec);
#endif

	return mysql_affected_rows(ctx->mysql);
}


int glite_lbu_QueryColumns(glite_lbu_Statement stmt, char **cols)
{
	int	i = 0;
	MYSQL_FIELD 	*f;

	if (!stmt->result) return ERR(stmt->ctx, EINVAL, "QueryColumns implemented only for simple API");
	while ((f = mysql_fetch_field(stmt->result))) cols[i++] = f->name;
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
	if (((*stmt)->stmt = mysql_stmt_init(ctx->mysql)) == NULL)
		return MY_ERRSTMT(*stmt);

	// prepare the SQL command
	retry = 1;
	do {
		mysql_stmt_prepare((*stmt)->stmt, sql, strlen(sql));
		ret = MY_ISOKSTMT(*stmt, &retry);
	} while (ret == 0);
	if (ret == -1) goto failed;

	// number of fields (0 for no results)
	if ((meta = mysql_stmt_result_metadata((*stmt)->stmt)) != NULL) {
		(*stmt)->nrfields = mysql_num_fields(meta);
		mysql_free_result(meta);
	} else
		(*stmt)->nrfields = 0;

	return CLR_ERR(ctx);

failed:
	glite_lbu_FreeStmt(stmt);
	return STATUS(ctx);
}


int glite_lbu_ExecStmt(glite_lbu_Statement stmt, int n, ...) {
	int i;
	va_list ap;
	glite_lbu_DBType type;
	char *pchar;
	long int *plint;
	MYSQL_TIME *ptime;
	glite_lbu_DBContext ctx;
	int ret, retry;
	MYSQL_BIND *binds = NULL;
	void **data = NULL;
	unsigned long *lens;

	// gather parameters
	if (n) {
		binds = calloc(n, sizeof(MYSQL_BIND));
		data = calloc(n, sizeof(void *));
		lens = calloc(n, sizeof(unsigned long *));
	}
	va_start(ap, n);
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
	va_end(ap);

	// bind parameters
	if (mysql_stmt_bind_param(stmt->stmt, binds) != 0) {
		MY_ERRSTMT(stmt);
		goto failed;
	}

	// run
	ctx = stmt->ctx;
	retry = 1;
	do {
		mysql_stmt_execute(stmt->stmt);
		ret = MY_ISOKSTMT(stmt, &retry);
	} while (ret == 0);
	if (ret == -1) goto failed;

	// result
	retry = 1;
	do {
		mysql_stmt_store_result(stmt->stmt);
		ret = MY_ISOKSTMT(stmt, &retry);
	} while (ret == 0);
	if (ret == -1) goto failed;

	// free params
	for (i = 0; i < n; i++) free(data[i]);
	free(data);
	free(binds);
	free(lens);
	CLR_ERR(ctx);
	return mysql_stmt_affected_rows(stmt->stmt);

failed:
	for (i = 0; i < n; i++) free(data[i]);
	free(data);
	free(binds);
	free(lens);
	return -1;
}


int glite_lbu_bufferedInsertInit(glite_lbu_DBContext ctx, glite_lbu_bufInsert *bi, void *mysql, const char *table_name, long size_limit, long record_limit, const char *columns)
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
static int lbu_err(glite_lbu_DBContext ctx, int code, const char *desc, const char *func, int line) {
	if (code) {
		ctx->err.code = code;
		free(ctx->err.desc);
		ctx->err.desc = desc ? strdup(desc) : NULL;
		dprintf(ctx, "[db %d] %s:%d %s\n", getpid(), func, line, desc);
		return code;
	} else
		return ctx->err.code;
}


/*
 * helping function: find oud mysql error and sets on the context
 */
static int myerr(glite_lbu_DBContext ctx, const char *source, int line) {
	return lbu_err(ctx, EIO, mysql_error(ctx->mysql), source, line);
}


/*
 * helping function: find oud mysql stmt error and sets on the context
 */
static int myerrstmt(glite_lbu_Statement stmt, const char *source, int line) {	
	return lbu_err(stmt->ctx, EIO, mysql_stmt_error(stmt->stmt), source, line);
}


/*
 * Ehelping function: error handle
 *
 * \return -1 failed
 * \return  0 retry
 * \return  1 OK
 */
static int myisokstmt(glite_lbu_Statement stmt, const char *source, int line, int *retry) {
	switch (mysql_stmt_errno(stmt->stmt)) {
		case 0:
			return 1;
			break;
		case ER_DUP_ENTRY:
			lbu_err(stmt->ctx, EEXIST, mysql_stmt_error(stmt->stmt), source, line);
			return -1;
			break;
		case CR_SERVER_LOST:
			if (*retry > 0) {
				(*retry)--;
				return 0;
			} else
				return -1;
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

	// needed for SQL result parameters
	assert(sizeof(int) >= sizeof(my_bool));

	if (!cs) return ERR(ctx, EINVAL, "connect string not specified");
	
	if (!(*mysql = mysql_init(NULL))) return ERR(ctx, ENOMEM, NULL);

	mysql_options(*mysql, MYSQL_READ_DEFAULT_FILE, "my");

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
		free(buf);
		ret = MY_ERR(ctx);
		glite_lbu_DBClose(ctx);
		return ret;
	}
	free(buf);

	ctx->cs = cs;
	return 0;
}


/*
 * mysql close
 */
static void db_close(MYSQL *mysql) {
	if (mysql) mysql_close(mysql);
}


/*
 * test transactions capability:
 *
 * 1) with connection 1 create testing table test<pid>
 * 2) with connection 1 insert a value
 * 3) with connection 2 look for a value, transactions are for no error and
 *    no items found
 * 4) with connection 1 commit and drop  the table
 */
static int transaction_test(glite_lbu_DBContext ctx, MYSQL *m2, int *have_transactions) {
	MYSQL *m1;
	char *desc, *cmd_create, *cmd_insert, *cmd_select, *cmd_drop;
	int retval;
	int err;
	pid_t pid;

	ctx->caps |= GLITE_LBU_DB_CAP_TRANSACTIONS;
	pid = getpid();
	*have_transactions = 0;

	asprintf(&cmd_create, "CREATE TABLE test%d (item INT) ENGINE='innodb'", pid);
	asprintf(&cmd_insert, "INSERT INTO test%d (item) VALUES (1)", pid);
	asprintf(&cmd_select, "SELECT item FROM test%d", pid);
	asprintf(&cmd_drop, "DROP TABLE test%d", pid);

	m1 = ctx->mysql;
	//glite_lbu_ExecSQL(ctx, cmd_drop, NULL);
	if (glite_lbu_ExecSQL(ctx, cmd_create, NULL) != 0) goto err1;
	if (glite_lbu_Transaction(ctx) != 0) goto err2;
	if (glite_lbu_ExecSQL(ctx, cmd_insert, NULL) != 1) goto err2;

	ctx->mysql = m2;
	if ((retval = glite_lbu_ExecSQL(ctx, cmd_select, NULL)) == -1) goto err2;

	ctx->mysql = m1;
	if (glite_lbu_Commit(ctx) != 0) goto err2;
	if (glite_lbu_ExecSQL(ctx, cmd_drop, NULL) != 0) goto err1;

#ifdef LBS_DB_PROFILE
	fprintf(stderr, "[%d] use_transactions = %d\n", getpid(), USE_TRANS(ctx));
#endif

	*have_transactions = retval == 0;
	goto ok;
err2:
	err = ctx->err.code;
	desc = ctx->err.desc;
	glite_lbu_ExecSQL(ctx, cmd_drop, NULL);
	ctx->err.code = err;
	ctx->err.desc = desc;
err1:
ok:
	free(cmd_create);
	free(cmd_insert);
	free(cmd_select);
	free(cmd_drop);
	return STATUS(ctx);
}


/*
 * simple version of the fetch
 */
static int FetchRowSimple(glite_lbu_DBContext ctx, MYSQL_RES *result, unsigned long *lengths, char **results) {
	MYSQL_ROW            row;
	int                  nr, i;
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
	int ret, retry, i;
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


void set_time(MYSQL_TIME *mtime, const time_t time) {
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


time_t get_time(const MYSQL_TIME *mtime) {
	struct tm tm;

	memset(&tm, 0, sizeof(tm));
	setenv("TZ","UTC",1); tzset();
	tm.tm_year = mtime->year - 1900;
	tm.tm_mon = mtime->month - 1;
	tm.tm_mday = mtime->day;
	tm.tm_hour = mtime->hour;
	tm.tm_min = mtime->minute;
	tm.tm_sec = mtime->second;

	return mktime(&tm);
}
