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

#ifndef GLITE_LBU_DB_H
#define GLITE_LBU_DB_H

#include <time.h>
#include <stdarg.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * \file db.h
 * \defgroup database Database module
 *
 * Database modul module API (LB & JP Utils).
 *
 * There are two ways to access DB here:
 *
 * - simple:
 * SQL commands as single string. All values are incorporated in the SQL command strings. Proper escaping is required.
 *
 * - enhanced:
 * Prepared SQL commands with separated parameters, functions PrepareStmt() and ExecPreparedStmt(). All values are delivered in separated buffers. Its faster for multiple using and more secure.
 *
 * @{
 */


/**
 * Enable transaction support if available.
 *
 * With disabled transaction can be used transaction functions, they are just ignored.
 */
#define GLITE_LBU_DB_CAP_TRANSACTIONS 1

/**
 * Check prepared parameters support.
 */
#define GLITE_LBU_DB_CAP_PREPARED 2

/**
 * Check for getting indexes support.
 *
 * Needed for QueryIndices call.
 */
#define GLITE_LBU_DB_CAP_INDEX 4


/**
 * Print all errors.
 *
 * Not returned from detection of capabilities.
 */
#define GLITE_LBU_DB_CAP_ERRORS 8


/**
 * Database connection context.
 */
typedef struct glite_lbu_DBContext_s *glite_lbu_DBContext;


/**
 * Prepared statement, used for SQL statement with parameters.
 */
typedef struct glite_lbu_Statement_s *glite_lbu_Statement;


/**
 * Structure holds date for multi-rows insert.
 */
typedef struct glite_lbu_bufInsert_s *glite_lbu_bufInsert;



/**
 * All types of parameteres, they match to the SQL types.
 */
typedef enum {
	GLITE_LBU_DB_TYPE_NULL = 0,
	GLITE_LBU_DB_TYPE_TINYINT = 1,
	GLITE_LBU_DB_TYPE_INT = 2,
	GLITE_LBU_DB_TYPE_TINYBLOB = 3,
	GLITE_LBU_DB_TYPE_TINYTEXT = 4,
	GLITE_LBU_DB_TYPE_BLOB = 5,
	GLITE_LBU_DB_TYPE_TEXT = 6,
	GLITE_LBU_DB_TYPE_MEDIUMBLOB = 7,
	GLITE_LBU_DB_TYPE_MEDIUMTEXT = 8,
	GLITE_LBU_DB_TYPE_LONGBLOB = 9,
	GLITE_LBU_DB_TYPE_LONGTEXT = 10,
	GLITE_LBU_DB_TYPE_VARCHAR = 11,
	GLITE_LBU_DB_TYPE_CHAR = 12,
	GLITE_LBU_DB_TYPE_DATE = 13,
	GLITE_LBU_DB_TYPE_TIME = 14,
	GLITE_LBU_DB_TYPE_DATETIME = 15,
	GLITE_LBU_DB_TYPE_TIMESTAMP = 16,
	GLITE_LBU_DB_TYPE_BOOLEAN = 17,
	GLITE_LBU_DB_TYPE_LAST = 18
} glite_lbu_DBType;



/**
 * Supported DB backends.
 */
typedef enum {
	GLITE_LBU_DB_BACKEND_MYSQL = 0,
	GLITE_LBU_DB_BACKEND_PSQL,
	GLITE_LBU_DB_BACKEND_LAST
} glite_lbu_DBBackendNo;


/**
 * Get error state from DB context.
 *
 * \param[in]  ctx   context to work with
 * \param[out] text  error name
 * \param[out] desc  error description
 */
int glite_lbu_DBError(glite_lbu_DBContext ctx, char **text, char **desc);


/**
 * Clear the error from DB context.
 *
 * \param[in] ctx  context to work with
 */
int glite_lbu_DBClearError(glite_lbu_DBContext ctx);


/**
 * Initialize the database context.
 *
 * \param[out] ctx   result context
 * \param[in]  backend  required database backend
 * \return     error code
 */
int glite_lbu_InitDBContext(glite_lbu_DBContext *ctx, int backend, char *log_category);


/**
 * Free database context.
 */
void glite_lbu_FreeDBContext(glite_lbu_DBContext ctx);


/**
 * Connect to the given database.
 *
 * \param[out] ctx     context to work with
 * \param[in] cs       connect string user/password\@host:database
 *
 * \return             error code, 0 = OK
 */
int glite_lbu_DBConnect(glite_lbu_DBContext ctx, const char *cs);


/**
 * Close the connection.
 *
 * \param[in,out] ctx	context to work with
 */
void glite_lbu_DBClose(glite_lbu_DBContext ctx);


/**
 * Check database version and capabilities.
 *
 * \param[in,out] ctx  context to work with
 *
 * \return             capabilities
 * \retval -1          error occured
 */
int glite_lbu_DBQueryCaps(glite_lbu_DBContext ctx);


/**
 * Set the database capabilities on already initialized context.
 *
 * It should be find out by DBQueryCaps() first.
 *
 * \param[in,out] ctx	context to work with
 * \param[in] caps     capabilities to use, should be found out by QueryCaps()
 */
void glite_lbu_DBSetCaps(glite_lbu_DBContext ctx, int caps);


/**
 * Start transaction.
 */
int glite_lbu_Transaction(glite_lbu_DBContext ctx);


/**
 * Commit (end) transaction.
 */
int glite_lbu_Commit(glite_lbu_DBContext ctx);


/**
 * Cancel transaction.
 */
int glite_lbu_Rollback(glite_lbu_DBContext ctx);


/**
 * \param[in,out] stmt  executed SQL statement
 * \param[in] n         number of items for sure there is enough space in lengths and results
 * \param[out] lengths  array with lengths (good for data blobs), may be NULL
 * \param[out] results  array with results, all items are allocated
 *
 * \retval >0	number of fields of the retrieved row
 * \retval 0	no more rows
 * \retval -1	error
 */
int glite_lbu_FetchRow(glite_lbu_Statement stmt, unsigned int n, unsigned long *lengths, char **results);


/**
 * Free the statement structure and destroy its parameters.
 *
 * Statement will be set to NULL and multiple calls are allowed.
 *
 * \param[in,out] stmt  statement
 */
void glite_lbu_FreeStmt(glite_lbu_Statement *stmt);


/**
 * Parse and execute one simple SQL statement.
 * All values are incorporated int the SQL command string.
 *
 * \param[in,out] ctx     context to work with
 * \param[in] cmd         SQL command
 * \param[out] stmt       statement handle with results (makes sense for selects only)
 *
 * \return                number of rows selected, created or affected by update, -1 on error
 */
int glite_lbu_ExecSQL(glite_lbu_DBContext ctx, const char *cmd, glite_lbu_Statement *stmt);


/**
 * Query for column names of the statement.
 *
 * It work only for simple API, so only after ExecSQL().
 *
 * \param[in,out] stmt  the statement handle
 * \param[out] cols     result array of names
 *
 * \return              error code
 */
int glite_lbu_QueryColumns(glite_lbu_Statement stmt, char **cols);


/**
 * Retrieve column names of a query simple SQL statement.
 *
 * \param[in,out] ctx        context to work with
 * \param[in] table          table name
 * \param[out] key_names     one-dimensional index names array
 * \param[out] column_names  two-dimensional column names array
 *
 * \return                0 if OK, nonzero on error
 */
int glite_lbu_QueryIndices(glite_lbu_DBContext ctx, const char *table, char ***key_names, char ****column_names);


/** 
 * Convert time_t into database-specific time string.
 *
 * The result string can be used directly in SQL commands.
 *
 * \param[in]   t    the converted time
 * \param[out]  str  result allocated string
 */
void glite_lbu_TimeToDB(glite_lbu_DBContext ctx, time_t t, char **str);


/** 
 * Convert double into database-specific time string.
 *
 * The result string can be used directly in SQL commands.
 *
 * \param[in]   t    the converted time
 * \param[out]  str  result allocated string
 */
void glite_lbu_TimestampToDB(glite_lbu_DBContext ctx, double t, char **str);


/**
 * Convert database-specific time string to time_t.
 *
 * String is expected in database for (ISO format).
 *
 * \param[in] ctx  context to work with
 * \param[in] str  the converted string
 * \return         result time
 */
time_t glite_lbu_DBToTime(glite_lbu_DBContext ctx, const char *str);


/**
 * Convert database-specific time string to time (double).
 *
 * \param[in] ctx  context to work with
 * \param[in] str  the converted string
 * \return         result time
 * */
double glite_lbu_DBToTimestamp(glite_lbu_DBContext ctx, const char *str);


/**
 * Get the connection string into the database
 *
 * \return newly allocated connection string
 * */
char *glite_lbu_DBGetConnectionString(glite_lbu_DBContext ctx);


/**
 * Get the hostname of the database
 *
 * \return newly allocated host
* */
char *glite_lbu_DBGetHost(glite_lbu_DBContext ctx);


/**
 * Get the name of the database
 *
 * \return newly allocated name
* */
char *glite_lbu_DBGetName(glite_lbu_DBContext ctx);


/* Generic helper time convert functions. */
void glite_lbu_TimeToStr(time_t t, char **str);
void glite_lbu_TimestampToStr(double t, char **str);
time_t glite_lbu_StrToTime(const char *str);
double glite_lbu_StrToTimestamp(const char *str);


/**
 * Init data structure for buffered insert
 *
 * takes table_name and columns string for future multirow insert
 * when insert string oversize size_limit or number of rows to be inserted
 * overcome record_limit, the real insert is triggered
 */
int glite_lbu_bufferedInsertInit(glite_lbu_DBContext ctx, glite_lbu_bufInsert *bi, const char *table_name, long size_limit, long record_limit, const char * columns);


/**
 * adds row of n values into n columns into an insert buffer
 * if num. of rows or size of data oversteps the limits, real
 * multi-row insert is done
 */
int glite_lbu_bufferedInsert(glite_lbu_bufInsert bi, const char *row);


/**
 * Flush buffered data and free bi structure.
 */
int glite_lbu_bufferedInsertClose(glite_lbu_bufInsert bi);


/**
 * Prepare the SQL statement. Use glite_lbu_FreeStmt() to free it.
 *
 * \param[in,out] ctx     context to work with
 * \param[in] sql         SQL command
 * \param[out] stmt       returned SQL statement
 *
 * \return                error code
 */
int glite_lbu_PrepareStmt(glite_lbu_DBContext ctx, const char *sql, glite_lbu_Statement *stmt);


/**
 * Bind input parameters and execute prepared SQL statement.
 * Results can be fetched via glite_lbu_FetchRow.
 *
 * \param[in,out] stmt  SQL statement
 * \param[in]  n                     number of items
 *
 * Variable parameters (n-times):
 *
 * always:
 *
 *   \param type     DB item type
 *
 * then one of them:
 *
 *   \param GLITE_LBU_DB_TYPE_TINYINT             int c
 *   \param GLITE_LBU_DB_TYPE_INT                 long int i
 *   \param GLITE_LBU_DB_TYPE_...BLOB/TEXT        void *b, unsigned long len
 *   \param GLITE_LBU_DB_TYPE_[VAR]CHAR           char *str
 *   \param GLITE_LBU_DB_TYPE_DATE/TIME/DATETIME  time_t t
 *   \param GLITE_LBU_DB_TYPE_TIMESTAMP           double t
 *   \param GLITE_LBU_DB_TYPE_BOOLEAN             int b
 *   \param GLITE_LBU_DB_TYPE_NULL       -
 *
 * \return              number of affected rows, -1 on error
 */
int glite_lbu_ExecPreparedStmt(glite_lbu_Statement stmt, int n, ...);


/**
 * "va_list version" of glite_lbu_ExecPreparedStmt().
 */
int glite_lbu_ExecPreparedStmt_v(glite_lbu_Statement stmt, int n, va_list ap);


/**
 * Returns the last automatically generated id, if any.
 */
long int glite_lbu_Lastid(glite_lbu_Statement stmt);


/**
 * @} database group
 */

#ifdef __cplusplus
}
#endif

#endif
