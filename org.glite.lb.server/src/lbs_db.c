#ident "$Header$"

#include "mysql/mysql.h"	// MySql header file
#include "mysql/mysqld_error.h"
#include "mysql/errmsg.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>

#include "lbs_db.h"
#include "glite/lb/context-int.h"


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

	if (!mysql_real_connect((MYSQL *) ctx->mysql,host,user,pw,db,0,NULL,0)) {
		free(buf);
		return my_err();
	}

	free(buf);
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

	edg_wll_ResetError(ctx);

	if (stmt) {
		*stmt = NULL;
	}

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


char *edg_wll_TimeToDB(time_t t)
{
	struct tm	*tm = gmtime(&t);
	char	tbuf[256];

	sprintf(tbuf,"'%4d-%02d-%02d %02d:%02d:%02d'",tm->tm_year+1900,tm->tm_mon+1,
		tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	
	return strdup(tbuf);
}

time_t edg_wll_DBToTime(char *t)
{
	struct tm	tm;

	memset(&tm,0,sizeof(tm));
	setenv("TZ","UTC",1); tzset();
	sscanf(t,"%4d-%02d-%02d %02d:%02d:%02d",
		&tm.tm_year,&tm.tm_mon,&tm.tm_mday,
		&tm.tm_hour,&tm.tm_min,&tm.tm_sec);
	tm.tm_year -= 1900;
	tm.tm_mon--;

	return mktime(&tm);
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
