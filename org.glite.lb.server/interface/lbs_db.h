#ifndef _LBS_DB_H
#define _LBS_DB_H

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
#include <sys/time.h>

#include "glite/lb/consumer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EDG_WLL_MYSQL_VERSION		40001
#define BUF_INSERT_ROW_ALLOC_BLOCK	1000


typedef struct _edg_wll_Stmt *edg_wll_Stmt;

edg_wll_ErrorCode edg_wll_DBConnect(
	edg_wll_Context,	/* INOUT: */
	const char *		/* IN: connect string user/password@host:database */
);


/*
 * structure holds date for multi-rows insert
 */
typedef struct _edg_wll_bufInsert {
	edg_wll_Context ctx;
	char	*table_name;
	char	*columns;	/* names of columns to be inserted into 
				 * (values separated with commas) */
	char	**rows;		/* each row hold string of one row to be inserted
				 * (values separated with commas) */
	long	rec_num, 	/* actual number of rows in structure */
		rec_size;	/* approx. size of a real insert string */
	long	size_limit, 	/* size and # of records limit which trigger */
		record_limit;	/* real insert; zero means unlimitted */
} edg_wll_bufInsert;


void edg_wll_DBClose(edg_wll_Context);


/* Parse and execute SQL statement. Returns number of rows selected, created 
 * or affected by update, or -1 on error */

int edg_wll_ExecStmt(
	edg_wll_Context,	/* INOUT: */
	char *,		/* IN: SQL statement */
	edg_wll_Stmt *	/* OUT: statement handle. Usable for select only */
);


/* Fetch next row of select statement. 
 * All columns are returned as fresh allocated strings 
 *
 * return values:
 * 	>0 - number of fields of the retrieved row
 * 	 0 - no more rows
 * 	-1 - error
 *
 * Errors are stored in context passed to previous edg_wll_ExecStmt() */

int edg_wll_FetchRow(
	edg_wll_Stmt,	/* IN: statement */
	char **		/* OUT: array of fetched values. 
			 *      As number of columns is fixed and known,
			 *      expects allocated array of pointers here */
);

/* Retrieve column names of a query statement */

int edg_wll_QueryColumns(
	edg_wll_Stmt,	/* IN: statement */
	char **		/* OUT: result set column names. Expects allocated array. */
);

/* Free the statement structure */

void edg_wll_FreeStmt(
	edg_wll_Stmt *    /* INOUT: statement */
);


/**
 * convert time_t into database-specific time string
 *
 * returns pointer to dynamic area which should be freed 
 */
char *edg_wll_TimeToDB(time_t);
time_t edg_wll_DBToTime(char *);

extern edg_wll_ErrorCode edg_wll_Open(edg_wll_Context ctx, char *cs);

/**
 * Check database version.
 */
int edg_wll_DBCheckVersion(edg_wll_Context, const char *);

int edg_wll_Transaction(edg_wll_Context ctx);
int edg_wll_Commit(edg_wll_Context ctx);
int edg_wll_Rollback(edg_wll_Context ctx);

/**
 * Init data structure for buffered insert
 *
 * takes table_name and columns string for future multirow insert
 * when insert string oversize size_limit or number of rows to be inserted
 * overcome record_limit, the real insert is triggered
 */
edg_wll_ErrorCode edg_wll_bufferedInsertInit(edg_wll_Context ctx, edg_wll_bufInsert *bi, void *mysql, char *table_name, long size_limit, long record_limit, char * columns);

/**
 * adds row of n values into n columns into an insert buffer
 * if num. of rows or size of data oversteps the limits, real
 * multi-row insert is done
 */
edg_wll_ErrorCode edg_wll_bufferedInsert(edg_wll_bufInsert *bi, char *row);

/**
 * flush buffered data and free bi structure
 */
edg_wll_ErrorCode edg_wll_bufferedInsertClose(edg_wll_bufInsert *bi);



#ifdef __cplusplus
}
#endif

#endif
