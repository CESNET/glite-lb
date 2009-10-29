
#define dprintf(CTX, FMT...) if ((CTX)->caps & GLITE_LBU_DB_CAP_ERRORS) fprintf(stderr, ##FMT)

struct glite_lbu_DBContext_s {
	int backend;
	struct {
		int code;
		char *desc;
	} err;
	int caps;
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
void glite_lbu_TimeToStr(time_t t, char **str);
void glite_lbu_TimestampToStr(double t, char **str);
time_t glite_lbu_StrToTime(const char *str);
double glite_lbu_StrToTimestamp(const char *str);
