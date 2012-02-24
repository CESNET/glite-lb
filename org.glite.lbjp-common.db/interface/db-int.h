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

#ifdef WIN32
#define asprintf(STR, FMT...) trio_asprintf((STR), ##FMT)
#define vasprintf(STR, FMT, VARGS) trio_asprintf((STR), (FMT), (VARGS))
#define strcasestr(H,N) strstr((H), (N))
#endif

#define dprintf(CTX, FMT...) if ((CTX)->caps & GLITE_LBU_DB_CAP_ERRORS) fprintf(stderr, ##FMT)

struct glite_lbu_DBContext_s {
	int backend;
	struct {
		int code;
		char *desc;
	} err;
	int caps;
	char *log_category;
	char *connection_string;
};
typedef struct glite_lbu_DBContext_s glite_lbu_DBContext_t;

struct glite_lbu_Statement_s {
	glite_lbu_DBContext ctx;
};
typedef struct glite_lbu_Statement_s glite_lbu_Statement_t;



typedef struct {
	int backend;

	int (*initContext)(glite_lbu_DBContext *ctx);
	void (*freeContext)(glite_lbu_DBContext ctx);
	int (*connect)(glite_lbu_DBContext ctx, const char *cs);
	void (*close)(glite_lbu_DBContext ctx);
	int (*queryCaps)(glite_lbu_DBContext ctx);
	void (*setCaps)(glite_lbu_DBContext ctx, int caps);

	int (*transaction)(glite_lbu_DBContext ctx);
	int (*commit)(glite_lbu_DBContext ctx);
	int (*rollback)(glite_lbu_DBContext ctx);

	int (*fetchRow)(glite_lbu_Statement stmt, unsigned int n, unsigned long *lengths, char **results);
	void (*freeStmt)(glite_lbu_Statement *stmt);

	int (*queryIndices)(glite_lbu_DBContext ctx, const char *table, char ***key_names, char ****column_names);
	int (*execSQL)(glite_lbu_DBContext ctx, const char *cmd, glite_lbu_Statement *stmt);
	int (*queryColumns)(glite_lbu_Statement stmt_gen, char **cols);

	int (*prepareStmt)(glite_lbu_DBContext ctx, const char *sql, glite_lbu_Statement *stmt);
	int (*execPreparedStmt_v)(glite_lbu_Statement stmt, int n, va_list ap);
	long int (*lastid)(glite_lbu_Statement stmt);

	void (*timeToDB)(time_t, char **str);
	void (*timestampToDB)(double t, char **str);
	time_t (*DBToTime)(const char *str);
	double (*DBToTimestamp)(const char *str);
} glite_lbu_DBBackend_t;

int glite_lbu_DBSetError(glite_lbu_DBContext ctx, int code, const char *func, int line, const char *desc, ...);

void glite_lbu_TimeToStrGeneric(time_t t, char **str, const char *amp);
void glite_lbu_TimestampToStrGeneric(double t, char **str, const char *amp);
