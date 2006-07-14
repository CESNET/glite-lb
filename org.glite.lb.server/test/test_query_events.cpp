#include <fstream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <glite/lb/consumer.h>
#include <glite/lb/context-int.h>

#include "lbs_db.h"
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
	int ExecStmt(const char *, edg_wll_Stmt *);

	void setUp() {
		edg_wll_InitContext(&ctx);
		ctx->mysql = (void *) this; /* XXX */
	}

};

void QueryEventsTest::oneJob()
{
	edg_wll_QueryRec	job[2];
	const edg_wll_QueryRec	*jobs[2] = { job,NULL} ;
	edg_wll_Event		*events;

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
}

int QueryEventsTest::ExecStmt(const char *qry, edg_wll_Stmt *stmt_out)
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

	*stmt_out = (edg_wll_Stmt) rows;
	return stmt->second.size()-1;
}

extern "C" {

int edg_wll_ExecStmt(edg_wll_Context ctx,char *qry,edg_wll_Stmt *stmt)
{
	cout << "edg_wll_ExecStmt: " << qry << endl;

	class QueryEventsTest *tst = (class QueryEventsTest *)(ctx->mysql);
	return tst->ExecStmt(qry, stmt);
}

int edg_wll_FetchRow(edg_wll_Stmt stmt, char **cols)
{
	vector<string>::iterator	*rows = (vector<string>::iterator *) stmt;
	char	*row,*p,i=0;

	if (**rows == "END") return 0;
	row = strdup((*rows)->c_str());
	(*rows)++;
	for (p = strtok(row,"\t"); p; p = strtok(NULL,"\t"))
		cols[i++] = strdup(p);

	return i;
}

void edg_wll_FreeStmt(edg_wll_Stmt *) {}

int debug;

int edg_wll_QueryColumns(edg_wll_Stmt stmt, char**cols) { return 0; }
char *edg_wll_TimeToDB(long t) { return NULL; }

time_t edg_wll_DBToTime(char *c) { return (time_t)-1; }
edg_wll_ErrorCode edg_wll_DBConnect(edg_wll_Context ctx, const char*str) { 
  return (edg_wll_ErrorCode)0;
}

int edg_wll_Transaction(edg_wll_Context ctx) { return 0; }
int edg_wll_Commit(edg_wll_Context ctx) { return 0; }
int edg_wll_Rollback(edg_wll_Context ctx) { return 0; }

edg_wll_ErrorCode edg_wll_bufferedInsert(edg_wll_bufInsert *bi, char **row)  { return (edg_wll_ErrorCode) 0; };
	
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
