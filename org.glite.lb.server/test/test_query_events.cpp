#include <iostream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <glite/lb/consumer.h>
#include <glite/lb/context-int.h>

#include "lbs_db.h"
#include "get_events.h"

using namespace std;

class QueryEventsTest: public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(QueryEventsTest);
	CPPUNIT_TEST(oneJob);
	CPPUNIT_TEST_SUITE_END();

private:
	edg_wll_Context	ctx;
	vector<pair<string,vector<string>>>	expQueries;
	int					queryIdx;

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
	edg_wlc_JobIdParse("https://fake.server/fake_job",&job[0].value.j);
	job[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	expQueries.clear();
	/*
	 *	XXX: ...
	 */
	expQueries.push_back();
	CPPUNIT_ASSERT(!edg_wll_QueryEventsServer(ctx,1,jobs,NULL,&events));
}

int QueryEventsTest::ExecStmt(const char *, edg_wll_Stmt *)
{
	return 0;
}

extern "C" {

int edg_wll_ExecStmt(edg_wll_Context ctx,char *qry,edg_wll_Stmt *stmt)
{
	cout << qry << endl;

	class QueryEventsTest *tst = (class QueryEventsTest *)(ctx->mysql);
	return tst->ExecStmt(qry, stmt);
}

int edg_wll_FetchRow(edg_wll_Stmt stmt, char **cols)
{
	return 0;
}

void edg_wll_FreeStmt(edg_wll_Stmt *) {}

int debug;

int edg_wll_QueryColumns(edg_wll_Stmt stmt, char**cols) {}
char *edg_wll_TimeToDB(long t) {}

time_t edg_wll_DBToTime(char *c) {}
edg_wll_ErrorCode  edg_wll_DBConnect(edg_wll_Context ctx, char*str) {}



	
}

CPPUNIT_TEST_SUITE_REGISTRATION(QueryEventsTest);

int main (int ac,const char *av[])
{
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
	CppUnit::TextUi::TestRunner runner;

	runner.addTest(suite);
	return runner.run() ? 0 : 1;
}
