#include <fstream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <glite/lb/consumer.h>
#include <glite/lb/context-int.h>

#include "get_events.h"

using namespace std;

static const char *test_dir;

class QueryEventsTest: public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(QueryEventsTest);
	CPPUNIT_TEST(oneJob);
	CPPUNIT_TEST_SUITE_END();

private:
	edg_wll_Context	ctx;
	
	ifstream	qry_file;

	vector<pair<string,vector<string> > >	queries;

public:
	void oneJob();
	int ExecStmt(const char *, glite_lbu_Statement *);

	void setUp() {
		edg_wll_InitContext(&ctx);
		ctx->dbctx = (glite_lbu_DBContext) this; /* XXX */
	}

	void tearDown() {
		edg_wll_FreeContext(ctx);
	}

};

void QueryEventsTest::oneJob()
{
	edg_wll_QueryRec	job[2];
	const edg_wll_QueryRec	*jobs[2] = { job,NULL} ;
	edg_wll_Event		*events;
	int                      i;

	job[0].attr = EDG_WLL_QUERY_ATTR_JOBID;
	job[0].op = EDG_WLL_QUERY_OP_EQUAL ;
	edg_wlc_JobIdParse("https://lhun.ics.muni.cz:4850/WrCEKje9QTXFiSOZuPMLtw",
		&job[0].value.j);
	job[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;
	
	string file(test_dir);
	file += "/oneJob.qry";
	qry_file.open(file.c_str());
	
	while (!qry_file.eof()) {
		string	query,line;
		vector<string>	rows;

		getline(qry_file,query);
		cout << "read: " << query <<endl;
		rows.clear();

		while (!qry_file.eof()) {
			getline(qry_file,line);
			if (line == "") break;
	
			rows.push_back(line);
		}
		rows.push_back("END");
		queries.push_back(pair<string,vector<string> >(query,rows));
	}

	qry_file.close();

	CPPUNIT_ASSERT(!edg_wll_QueryEventsServer(ctx,1,jobs,NULL,&events));

	edg_wlc_JobIdFree(job[0].value.j);
	for (i = 0; events[i].type; i++) edg_wll_FreeEvent(&events[i]);
	free(events);
}

int QueryEventsTest::ExecStmt(const char *qry, glite_lbu_Statement *stmt_out)
{
	vector<pair<string,vector<string> > >::iterator	stmt = queries.begin();

	for (; stmt != queries.end(); stmt++) {
		const char	*q = stmt->first.c_str();

		/* XXX: there some spaces at the end of qry */
		if (!strncmp(q,qry,strlen(q))) break;
	}

	if (stmt == queries.end()) {
		cerr << "query not found" << endl;
		CPPUNIT_ASSERT(0);
	}
	vector<string>::iterator	*rows = new vector<string>::iterator(stmt->second.begin());

	*stmt_out = (glite_lbu_Statement) rows;
	return stmt->second.size()-1;
}

extern "C" {

int glite_lbu_ExecSQL(glite_lbu_DBContext ctx,const char *qry,glite_lbu_Statement *stmt)
{
	cout << "glite_lbu_ExecSQL: " << qry << endl;

	class QueryEventsTest *tst = (class QueryEventsTest *)ctx;
	return tst->ExecStmt(qry, stmt);
}

int glite_lbu_FetchRow(glite_lbu_Statement stmt, unsigned int n, unsigned long int *lengths, char **cols)
{
	vector<string>::iterator	*rows = (vector<string>::iterator *) stmt;
	char	*row,*p,i=0;

	if (**rows == "END") return 0;
	row = strdup((*rows)->c_str());
	(*rows)++;
	for (p = strtok(row,"\t"); p; p = strtok(NULL,"\t"))
		cols[i++] = strdup(p);
	free(row);

	return i;
}

void glite_lbu_FreeStmt(glite_lbu_Statement *stmt) {
	vector<string>::iterator *rows = (vector<string>::iterator *) *stmt;	

	delete rows;
	*stmt = NULL;
}

int debug;

int glite_lbu_QueryColumns(glite_lbu_Statement stmt, char**cols) { return 0; }
void glite_lbu_TimeToDB(long t, char **s) { *s = NULL; }
time_t glite_lbu_DBToTime(const char *c) { return (time_t)-1; }
int glite_lbu_DBConnect(glite_lbu_DBContext *ctx, const char*str) { return 0; }

int glite_lbu_Transaction(glite_lbu_DBContext ctx) { return 0; }
int glite_lbu_Commit(glite_lbu_DBContext ctx) { return 0; }
int glite_lbu_Rollback(glite_lbu_DBContext ctx) { return 0; }

int glite_lbu_bufferedInsert(glite_lbu_bufInsert bi, const char *row)  { return 0; }

int glite_lbu_QueryIndices(glite_lbu_DBContext ctx, const char *table, char ***key_names, char ****column_names) { return 0; }
int glite_lbu_DBError(glite_lbu_DBContext ctx, char **s1, char **s2) { return 0; }

}

CPPUNIT_TEST_SUITE_REGISTRATION(QueryEventsTest);

int main (int ac,const char *av[])
{
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
	CppUnit::TextUi::TestRunner runner;

	test_dir = ac >= 2 ? av[1] : "../test";

	runner.addTest(suite);
	return runner.run() ? 0 : 1;
}
