#ident "$Header$"
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
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glite/lbu/trio.h>
#include "glite/lbu/log.h"

#include "db.h"
#include "db-int.h"


#define VALID(BACKEND) ((BACKEND) >= 0 && (BACKEND) < GLITE_LBU_DB_BACKEND_LAST)


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


/* possible backends */
#ifdef MYSQL_ENABLED
extern glite_lbu_DBBackend_t mysql_backend;
#else
#define mysql_backend no_backend
#endif
#ifdef PSQL_ENABLED
extern glite_lbu_DBBackend_t psql_backend;
#else
#define psql_backend no_backend
#endif

glite_lbu_DBBackend_t no_backend = {
	backend: -1,

	/* functions unspecified */
};

glite_lbu_DBBackend_t *backends[GLITE_LBU_DB_BACKEND_LAST] = {
	&mysql_backend,
	&psql_backend
};


/* --- internal functions ---- */

int glite_lbu_DBClearError(glite_lbu_DBContext ctx) {
	ctx->err.code = 0;
	if (ctx->err.desc) {
		free(ctx->err.desc);
		ctx->err.desc = NULL;
	}
	return 0;
}


int glite_lbu_DBSetError(glite_lbu_DBContext ctx, int code, const char *func, int line, const char *desc, ...) {
	va_list ap;

	if (!code) return ctx->err.code;

	ctx->err.code = code;
	free(ctx->err.desc);
	if (desc) {
		va_start(ap, desc);
		vasprintf(&ctx->err.desc, desc, ap);
		va_end(ap);
	} else
		ctx->err.desc = NULL;
	glite_common_log(ctx->log_category, LOG_PRIORITY_WARN, 
		"[db %d] %s:%d %s\n", getpid(), func, line, ctx->err.desc);
	return code;
}


void glite_lbu_TimeToStrGeneric(time_t t, char **str, const char *amp) {
	struct tm	*tm = gmtime(&t);

	asprintf(str,"%s%4d-%02d-%02d %02d:%02d:%02d%s",amp,tm->tm_year+1900,tm->tm_mon+1,
		tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec,amp);
}


void glite_lbu_TimeToStr(time_t t, char **str) {
	glite_lbu_TimeToStrGeneric(t, str, "'");
}


void glite_lbu_TimestampToStrGeneric(double t, char **str, const char *amp) {
	time_t tsec = t;
	struct tm *tm = gmtime(&tsec);

	t = t - tsec + tm->tm_sec;
	asprintf(str,"%s%4d-%02d-%02d %02d:%02d:%02.09f%s",amp,tm->tm_year+1900,tm->tm_mon+1,
	         tm->tm_mday,tm->tm_hour,tm->tm_min,t,amp);
}


void glite_lbu_TimestampToStr(double t, char **str) {
	glite_lbu_TimestampToStrGeneric(t, str, "'");
}


static time_t tm2time(struct tm *tm) {
	static struct tm tm_last = { tm_year:0, tm_mon:0 };
	static time_t t = (time_t)-1;
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	char *tz;

	pthread_mutex_lock(&lock);
	if (tm->tm_year == tm_last.tm_year && tm->tm_mon == tm_last.tm_mon) {
		t = t + (tm->tm_sec - tm_last.tm_sec)
		      + (tm->tm_min - tm_last.tm_min) * 60
		      + (tm->tm_hour - tm_last.tm_hour) * 3600
		      + (tm->tm_mday - tm_last.tm_mday) * 86400;
		memcpy(&tm_last, tm, sizeof tm_last);
	} else {
		tz = getenv("TZ");
		if (tz) tz = strdup(tz);
		setenv("TZ", "UTC", 1);
		tzset();

		t =  mktime(tm);
		memcpy(&tm_last, tm, sizeof tm_last);

		if (tz) setenv("TZ", tz, 1);
		else unsetenv("TZ");
		free(tz);
		tzset();
	}
	pthread_mutex_unlock(&lock);

	return t;
}


time_t glite_lbu_StrToTime(const char *str) {
	struct tm       tm;

	memset(&tm,0,sizeof(tm));
	sscanf(str,"%4d-%02d-%02d %02d:%02d:%02d",
	        &tm.tm_year,&tm.tm_mon,&tm.tm_mday,
        	&tm.tm_hour,&tm.tm_min,&tm.tm_sec);
	tm.tm_year -= 1900;
	tm.tm_mon--;

	return tm2time(&tm);
}


double glite_lbu_StrToTimestamp(const char *str) {
	struct tm	tm;
	double	sec;

	memset(&tm,0,sizeof(tm));
	sscanf(str,"%4d-%02d-%02d %02d:%02d:%lf",
		&tm.tm_year,&tm.tm_mon,&tm.tm_mday,
		&tm.tm_hour,&tm.tm_min,&sec);
	tm.tm_year -= 1900;
	tm.tm_mon--;
	tm.tm_sec = sec;

	return (sec - tm.tm_sec) + tm2time(&tm);
}


/* ---- API ---- */


int glite_lbu_DBError(glite_lbu_DBContext ctx, char **text, char **desc) {
	if (text) *text = strdup(strerror(ctx->err.code));
	if (desc) {
		if (ctx->err.desc) *desc = strdup(ctx->err.desc);
		else *desc = NULL;
	}

	return ctx->err.code;
}


int glite_lbu_InitDBContext(glite_lbu_DBContext *ctx, int backend, char *log_category) {
	int ret;

	if (!VALID(backend)) return EINVAL;
	if (backends[backend]->backend != backend) return ENOTSUP;
	ret = backends[backend]->initContext(ctx);
	if (ctx && *ctx) {
		(*ctx)->backend = backend;
		(*ctx)->log_category = log_category;
	}

	return ret;
}


void glite_lbu_FreeDBContext(glite_lbu_DBContext ctx) {
	if (!ctx || !VALID(ctx->backend)) return;

	free(ctx->err.desc);
	ctx->err.desc = NULL;
	backends[ctx->backend]->freeContext(ctx);
}


int glite_lbu_DBConnect(glite_lbu_DBContext ctx, const char *cs) {
	if (!VALID(ctx->backend)) return EINVAL;
	ctx->connection_string = strdup(cs);
	return backends[ctx->backend]->connect(ctx, cs);
}


void glite_lbu_DBClose(glite_lbu_DBContext ctx) {
	if (!VALID(ctx->backend)) return;
	backends[ctx->backend]->close(ctx);
}


int glite_lbu_DBQueryCaps(glite_lbu_DBContext ctx) {
	if (!VALID(ctx->backend)) return -1;
	return backends[ctx->backend]->queryCaps(ctx);
}


void glite_lbu_DBSetCaps(glite_lbu_DBContext ctx, int caps) {
	if (!VALID(ctx->backend)) return;
	return backends[ctx->backend]->setCaps(ctx, caps);
}


int glite_lbu_Transaction(glite_lbu_DBContext ctx) {
	if (!VALID(ctx->backend)) return EINVAL;
	return backends[ctx->backend]->transaction(ctx);
}


int glite_lbu_Commit(glite_lbu_DBContext ctx) {
	if (!VALID(ctx->backend)) return EINVAL;
	return backends[ctx->backend]->commit(ctx);
}


int glite_lbu_Rollback(glite_lbu_DBContext ctx) {
	if (!VALID(ctx->backend)) return EINVAL;
	return backends[ctx->backend]->rollback(ctx);
}


int glite_lbu_FetchRow(glite_lbu_Statement stmt, unsigned int n, unsigned long *lengths, char **results) {
	if (!VALID(stmt->ctx->backend)) return -1;
	return backends[stmt->ctx->backend]->fetchRow(stmt, n, lengths, results);
}


void glite_lbu_FreeStmt(glite_lbu_Statement *stmt) {
	if (!stmt || !*stmt || !VALID((*stmt)->ctx->backend)) return;
	return backends[(*stmt)->ctx->backend]->freeStmt(stmt);
}


int glite_lbu_QueryIndices(glite_lbu_DBContext ctx, const char *table, char ***key_names, char ****column_names) {
	if (!VALID(ctx->backend)) return EINVAL;
	return backends[ctx->backend]->queryIndices(ctx, table, key_names, column_names);
}


int glite_lbu_ExecSQL(glite_lbu_DBContext ctx, const char *cmd, glite_lbu_Statement *stmt) {
	if (!VALID(ctx->backend)) return -1;
	return backends[ctx->backend]->execSQL(ctx, cmd, stmt);
}


int glite_lbu_QueryColumns(glite_lbu_Statement stmt, char **cols) {
	if (!VALID(stmt->ctx->backend)) return EINVAL;
	return backends[stmt->ctx->backend]->queryColumns(stmt, cols);
}


int glite_lbu_PrepareStmt(glite_lbu_DBContext ctx, const char *sql, glite_lbu_Statement *stmt) {
	if (!VALID(ctx->backend)) return EINVAL;
	return backends[ctx->backend]->prepareStmt(ctx, sql, stmt);
}


int glite_lbu_ExecPreparedStmt_v(glite_lbu_Statement stmt, int n, va_list ap) {
	if (!VALID(stmt->ctx->backend)) return -1;
	return backends[stmt->ctx->backend]->execPreparedStmt_v(stmt, n, ap);
}


int glite_lbu_ExecPreparedStmt(glite_lbu_Statement stmt, int n, ...) {
	va_list ap;
	int retval;

	va_start(ap, n);
	retval = glite_lbu_ExecPreparedStmt_v(stmt, n, ap);
	va_end(ap);

	return retval;
}


long int glite_lbu_Lastid(glite_lbu_Statement stmt) {
	if (!VALID(stmt->ctx->backend)) return 0;
	return backends[stmt->ctx->backend]->lastid(stmt);
}


void glite_lbu_TimeToDB(glite_lbu_DBContext ctx, time_t t, char **str) {
	if (!VALID(ctx->backend)) return;
	return backends[ctx->backend]->timeToDB(t, str);
}


void glite_lbu_TimestampToDB(glite_lbu_DBContext ctx, double t, char **str) {
	if (!VALID(ctx->backend)) return;
	return backends[ctx->backend]->timestampToDB(t, str);
}


time_t glite_lbu_DBToTime(glite_lbu_DBContext ctx, const char *str) {
	if (!VALID(ctx->backend)) return (time_t)-1;
	return backends[ctx->backend]->DBToTime(str);
}


double glite_lbu_DBToTimestamp(glite_lbu_DBContext ctx, const char *str) {
	if (!VALID(ctx->backend)) return -1;
	return backends[ctx->backend]->DBToTimestamp(str);
}

char *glite_lbu_DBGetConnectionString(glite_lbu_DBContext ctx) {
	return strdup(ctx->connection_string);
}

char *glite_lbu_DBGetHost(glite_lbu_DBContext ctx) {
	//XXX this is same for msql and pg, move it into backends, when some difference occurs

	char *host, *buf, *slash, *at, *colon;
	if (! ctx->connection_string)
		return NULL;

	buf = strdup(ctx->connection_string);
	slash = strchr(buf,'/');
        at = strrchr(buf,'@');
        colon = strrchr(buf,':');

	if (!slash || !at || !colon){
		//XXX should never happen when DB is opened
		free(buf);
		return NULL;
	}

	*slash = *at = *colon = 0;

	host = strdup(at+1);
	free(buf);
	
	return host;
}

char *glite_lbu_DBGetName(glite_lbu_DBContext ctx) {
        //XXX this is same for msql and pg, move it into backends, when some difference occurs

        char *name, *buf, *slash, *at, *colon;
        if (! ctx->connection_string)
                return NULL;

        buf = strdup(ctx->connection_string);
        slash = strchr(buf,'/');
        at = strrchr(buf,'@');
        colon = strrchr(buf,':');

        if (!slash || !at || !colon){
                //XXX should never happen when DB is opened
                free(buf);
                return NULL;
        }

        *slash = *at = *colon = 0;

        name = strdup(colon+1);
        free(buf);

        return name;
}



#define STATUS(CTX) ((CTX)->err.code)
#define CLR_ERR(CTX) glite_lbu_DBClearError((CTX))
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
	glite_common_log_msg(bi->ctx->log_category, LOG_PRIORITY_DEBUG, stmt);

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
