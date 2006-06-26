#ident "$Header$"

#include "mysql.h"	// MySql header file
#include "mysqld_error.h"
#include "errmsg.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>

#include "lbs_db.h"
#include "glite/lb/context-int.h"
#include "glite/lb/trio.h"


#define DEFAULTCS	"lbserver/@localhost:lbserver20"

#define my_err() edg_wll_SetError(ctx,EDG_WLL_ERROR_DB_CALL,mysql_error((MYSQL *) ctx->mysql))

struct _edg_wll_Stmt {
	MYSQL_RES	*result;
	edg_wll_Context	ctx;
};

edg_wll_ErrorCode edg_wll_DBConnect(edg_wll_Context ctx,char *cs)
{
	char	*buf = NULL;
	char	*host,*user,*pw,*db; 
	char	*slash,*at,*colon;

	if (!cs) cs = DEFAULTCS;

	if (!(ctx->mysql = (void *) mysql_init(NULL))) 
		return edg_wll_SetError(ctx,ENOMEM,NULL);

	mysql_options(ctx->mysql, MYSQL_READ_DEFAULT_FILE, "my");

	host = user = pw = db = NULL;

	buf = strdup(cs);
	slash = strchr(buf,'/');
	at = strrchr(buf,'@');
	colon = strrchr(buf,':');

	if (!slash || !at || !colon) {
		free(buf);
		return edg_wll_SetError(ctx,EINVAL,"DB connect string");
	}

	*slash = *at = *colon = 0;
	host = at+1;
	user = buf;
	pw = slash+1;
	db = colon+1;

	/* ljocha: CLIENT_FOUND_ROWS added to make authorization check
	 * working in update_notif(). 
	 * Hope it does not break anything else */ 
	if (!mysql_real_connect((MYSQL *) ctx->mysql,host,user,pw,db,0,NULL,CLIENT_FOUND_ROWS)) {
		free(buf);
		return my_err();
	}

	free(buf);
#ifdef LBS_DB_PROFILE
	fprintf(stderr, "[%d] use_transactions = %d\n", getpid(), ctx->use_transactions);
#endif
	return edg_wll_ResetError(ctx);
}

void edg_wll_DBClose(edg_wll_Context ctx)
{
	mysql_close((MYSQL *) ctx->mysql);
	ctx->mysql = NULL;
}

int edg_wll_ExecStmt(edg_wll_Context ctx,char *txt,edg_wll_Stmt *stmt)
{
	int	err;
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

	edg_wll_ResetError(ctx);

	if (stmt) {
		*stmt = NULL;
	}
/* 
fputs(txt,stderr);
putc(10,stderr);
*/

#ifdef LBS_DB_PROFILE
	gettimeofday(&start,NULL);
#endif

	while (retry_nr == 0 || do_reconnect) {
		do_reconnect = 0;
		if (mysql_query((MYSQL *) ctx->mysql,txt)) {
			/* error occured */
			switch (err = mysql_errno((MYSQL *) ctx->mysql)) {
				case 0:
					break;
				case ER_DUP_ENTRY: 
					edg_wll_SetError(ctx,EEXIST,mysql_error((MYSQL *) ctx->mysql));
					return -1;
					break;
				case CR_SERVER_LOST:
					if (retry_nr <= 0) 
						do_reconnect = 1;
					break;
				default:
					my_err();
					return -1;
					break;
			}
		}
		retry_nr++;
	}

	if (stmt) {
		*stmt = malloc(sizeof(**stmt));
		if (!*stmt) {
			edg_wll_SetError(ctx,ENOMEM,NULL);
			return -1;
		}
		memset(*stmt,0,sizeof(**stmt));
		(**stmt).ctx = ctx;
		(**stmt).result = mysql_store_result((MYSQL *) ctx->mysql);
		if (!(**stmt).result) {
			if (mysql_errno((MYSQL *) ctx->mysql)) {
				my_err();
				return -1;
			}
		}
	} else {
		MYSQL_RES	*r = mysql_store_result((MYSQL *) ctx->mysql);
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
	
	return mysql_affected_rows((MYSQL *) ctx->mysql);
}

int edg_wll_FetchRow(edg_wll_Stmt stmt,char **res)
{
	MYSQL_ROW	row;
	edg_wll_Context	ctx = stmt->ctx;
	int 		nr,i;
	unsigned long	*len;

	edg_wll_ResetError(ctx);

	if (!stmt->result) return 0;

	if (!(row = mysql_fetch_row(stmt->result))) {
		if (mysql_errno((MYSQL *) ctx->mysql)) {
			my_err();
			return -1;
		} else return 0;
	}

	nr = mysql_num_fields(stmt->result);
	len = mysql_fetch_lengths(stmt->result);
	for (i=0; i<nr; i++) res[i] = len[i] ? strdup(row[i]) : strdup("");

	return nr;
}

int edg_wll_QueryColumns(edg_wll_Stmt stmt,char **cols)
{
	int	i = 0;
	MYSQL_FIELD 	*f;

	while ((f = mysql_fetch_field(stmt->result))) cols[i++] = f->name;
	return i == 0;
}

void edg_wll_FreeStmt(edg_wll_Stmt *stmt)
{
	if (*stmt) {
		if ((**stmt).result) mysql_free_result((**stmt).result);
		free(*stmt);
		*stmt = NULL;
	}
}

int edg_wll_DBCheckVersion(edg_wll_Context ctx)
{
	MYSQL	*m = (MYSQL *) ctx->mysql;
	const   char *ver_s = mysql_get_server_info(m);
	int	major,minor,sub,version;

	if (!ver_s || 3 != sscanf(ver_s,"%d.%d.%d",&major,&minor,&sub))
		return edg_wll_SetError(ctx,EINVAL,"retreiving MySQL version");

	version = 10000*major + 100*minor + sub;

	if (version < EDG_WLL_MYSQL_VERSION) {
		char	msg[300];

		snprintf(msg,sizeof msg,"Your MySQL version is %d. At least %d required.",version,EDG_WLL_MYSQL_VERSION);
		return edg_wll_SetError(ctx,EINVAL,msg);
	}

	return edg_wll_ResetError(ctx);
}


int edg_wll_Transaction(edg_wll_Context ctx) {
	int err = 0;

	if (ctx->use_transactions) {
		err = edg_wll_ExecStmt(ctx, "set autocommit=0", NULL);
		if (!err)
			return edg_wll_ExecStmt(ctx, "begin", NULL);
	}

	return err;
}


int edg_wll_Commit(edg_wll_Context ctx) {
	int err = 0;

	if (ctx->use_transactions) {
		err = edg_wll_ExecStmt(ctx, "commit", NULL);
		if (!err)
			return edg_wll_ExecStmt(ctx, "set autocommit=1", NULL);
	}

	return err;
}


int edg_wll_Rollback(edg_wll_Context ctx) {
	int err = 0;

	if (ctx->use_transactions) { 
		err = edg_wll_ExecStmt(ctx, "rollback", NULL);
		if (!err)
			return edg_wll_ExecStmt(ctx, "set autocommit=1", NULL);
	}

	return err;
}


int edg_wll_bufferedInsertInit(edg_wll_Context ctx, edg_wll_bufInsert *bi, void *mysql, char *table_name, long size_limit, long record_limit, int num_cols, ...)
{
	va_list l;
	long	i;


	assert(num_cols != 1);	// XXX: I do not know one column multi-row insert :(
	va_start(l, num_cols);
	
	bi->ctx = ctx;
	bi->table_name = strdup(table_name);
	bi->num_cols = num_cols;
	bi->columns = calloc(num_cols, sizeof(*(bi->columns)) );
	bi->rec_num = 0;
	bi->rec_size = 0;
	bi->row_len = calloc(num_cols, sizeof(*(bi->row_len)) );
	bi->row_alloc = calloc(num_cols, sizeof(*(bi->row_alloc)) );
	bi->values = calloc(num_cols, sizeof(*(bi->values)) );
	bi->size_limit = size_limit;
	bi->record_limit = record_limit;

	for (i=0; i < num_cols; i++) {
		bi->columns[i] = strdup(va_arg(l,char *));	
		bi->row_len[i] = 0;
		bi->row_alloc[i] = 0;
	}

	va_end(l);
	return 0;
}



static int string_add(char *what, long *used_size, long *alloc_size, char **where)
{
	long	what_len = strlen(what);
	int	reall = 0;

	while (*used_size + what_len >= *alloc_size) {
		*alloc_size += BUF_INSERT_ROW_ALLOC_BLOCK;
		reall = 1;
	}

	if (reall) 
		*where = realloc(*where, *alloc_size * sizeof(char));

	what_len = sprintf(*where + *used_size, "%s", what);
	if (what_len < 0) /* ENOMEM? */ return -1;

	*used_size += what_len;

	return 0;
}


static int flush_bufferd_insert(edg_wll_bufInsert *bi)
{
	char *stmt, *cols, *vals, *temp;
	long i;

	asprintf(&cols,"%s", bi->columns[0]);
	asprintf(&vals,"%s", bi->values[0]);
	for (i=1; i < bi->num_cols; i++) {
		asprintf(&temp,"%s,%s", cols, bi->columns[i]);
		free(cols); cols = temp; temp = NULL;
		asprintf(&temp,"%s),(%s", vals, bi->values[i]);
		free(vals); vals = temp; temp = NULL;
		
	}
	
	trio_asprintf(&stmt, "insert into %|Ss(%|Ss) values (%|Ss);",
		bi->table_name, cols, vals);

	// XXX: remove after testing
	fprintf(stderr,"\n%s\n",stmt);

	if (edg_wll_ExecStmt(bi->ctx,stmt,NULL) < 0) {
                if (edg_wll_Error(bi->ctx,NULL,NULL) == EEXIST)
                        edg_wll_ResetError(bi->ctx);
        }

	/* reset bi counters */
	bi->rec_size = 0;
	bi->rec_num = 0;
	for (i=0; i < bi->num_cols; i++) 
		bi->row_len[i] = 0;
	
	free(cols);
	free(vals);
	free(stmt);

	return edg_wll_Error(bi->ctx,NULL,NULL);
}


/*
 * adds row of n values into n columns into an insert buffer
 * if num. of rows or size of data oversteps the limits, real
 * multi-row insert is done
 */
edg_wll_ErrorCode edg_wll_bufferedInsert(edg_wll_bufInsert *bi, ...)
{
	va_list l;
	long	i;


	va_start(l, bi);

	bi->rec_size = 0;
	for (i=0; i < bi->num_cols; i++) {
		if (bi->rec_num) { 
			if (string_add(",", &bi->row_len[i], &bi->row_alloc[i], &bi->values[i]))
				return edg_wll_SetError(bi->ctx,ENOMEM,NULL);;
		}
		if (string_add(va_arg(l,char *), &bi->row_len[i], &bi->row_alloc[i], &bi->values[i]))
			return edg_wll_SetError(bi->ctx,ENOMEM,NULL);;
		bi->rec_size += bi->row_len[i];
	}
	bi->rec_num++;

	if ((bi->size_limit && bi->rec_size >= bi->size_limit) ||
		(bi->record_limit && bi->rec_num >= bi->record_limit))
	{
		if (flush_bufferd_insert(bi))
			return edg_wll_Error(bi->ctx,NULL,NULL);
	}

	va_end(l);
	return edg_wll_ResetError(bi->ctx);
}

static void free_buffered_insert(edg_wll_bufInsert *bi) {
	long i;
	
	free(bi->table_name);
	for (i=0; i < bi->num_cols; i++) {
		free(bi->columns[i]);
		free(bi->values[i]);
	}
	free(bi->columns);
	free(bi->values);
	free(bi->row_len);
	free(bi->row_alloc);
}

edg_wll_ErrorCode edg_wll_bufferedInsertClose(edg_wll_bufInsert *bi)
{
	if (flush_bufferd_insert(bi))
		return edg_wll_Error(bi->ctx,NULL,NULL);
	free_buffered_insert(bi);

	return edg_wll_ResetError(bi->ctx);
}

