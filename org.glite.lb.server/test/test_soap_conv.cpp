#include <iostream>
#include <stdsoap2.h>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <glite/lb/query_rec.h>

#include "bk_ws_H.h"
#include "ws_typeref.h"

using namespace std;

class SoapConvTest: public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(SoapConvTest);
	CPPUNIT_TEST(Conditions);
	CPPUNIT_TEST(States);
	CPPUNIT_TEST_SUITE_END();

private:
	struct soap						   *soap;
	edg_wll_QueryRec				  **stdConds;
	edg_wll_JobStat						stdStat;

	int stdRecCmp(edg_wll_QueryRec &, edg_wll_QueryRec &);
	int stdCondsCmp(edg_wll_QueryRec **, edg_wll_QueryRec **);
	int soapRecCmp(struct edgwll__QueryRec &, struct edgwll__QueryRec &);
	int soapCondsCmp(struct edgwll__QueryConditions &, struct edgwll__QueryConditions &);

public:
	void setUp();

	void Conditions();
	void States();
};

void SoapConvTest::setUp()
{
	soap = soap_new();

	stdConds = (edg_wll_QueryRec **)calloc(17, sizeof(edg_wll_QueryRec *));

	stdConds[0] = (edg_wll_QueryRec *)calloc(4, sizeof(edg_wll_QueryRec));
	stdConds[0][0].attr = EDG_WLL_QUERY_ATTR_STATUS;
	stdConds[0][0].op = EDG_WLL_QUERY_OP_EQUAL;
	stdConds[0][0].value.i = EDG_WLL_JOB_DONE;
	stdConds[0][1].attr = EDG_WLL_QUERY_ATTR_STATUS;
	stdConds[0][1].op = EDG_WLL_QUERY_OP_EQUAL;
	stdConds[0][1].value.i = EDG_WLL_JOB_RUNNING;
	stdConds[0][2].attr = EDG_WLL_QUERY_ATTR_STATUS;
	stdConds[0][2].op = EDG_WLL_QUERY_OP_EQUAL;
	stdConds[0][2].value.i = EDG_WLL_JOB_CANCELLED;

	stdConds[1] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[1][0].attr = EDG_WLL_QUERY_ATTR_OWNER;
	stdConds[1][0].op = EDG_WLL_QUERY_OP_EQUAL;
	stdConds[1][0].value.c = NULL;

	stdConds[2] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[2][0].attr = EDG_WLL_QUERY_ATTR_JOBID;
	stdConds[2][0].op = EDG_WLL_QUERY_OP_EQUAL;
	edg_wlc_JobIdCreate("my.server.org", 9000, &(stdConds[2][0].value.j));

	stdConds[3] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[3][0].attr = EDG_WLL_QUERY_ATTR_LOCATION;
	stdConds[3][0].op = EDG_WLL_QUERY_OP_EQUAL;
	stdConds[3][0].value.c = strdup("my_location");

	stdConds[4] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[4][0].attr = EDG_WLL_QUERY_ATTR_DESTINATION;
	stdConds[4][0].op = EDG_WLL_QUERY_OP_EQUAL;
	stdConds[4][0].value.c = strdup("my_destination");

	stdConds[5] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[5][0].attr = EDG_WLL_QUERY_ATTR_DONECODE;
	stdConds[5][0].op = EDG_WLL_QUERY_OP_GREATER;
	stdConds[5][0].value.i = 1;

	stdConds[6] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[6][0].attr = EDG_WLL_QUERY_ATTR_USERTAG;
	stdConds[6][0].op = EDG_WLL_QUERY_OP_EQUAL;
	stdConds[6][0].attr_id.tag = strdup("color");
	stdConds[6][0].value.c = strdup("red");

	stdConds[7] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[7][0].attr = EDG_WLL_QUERY_ATTR_TIME;
	stdConds[7][0].op = EDG_WLL_QUERY_OP_WITHIN;
	stdConds[7][0].value.t = (struct timeval){10, 1};
	stdConds[7][0].value2.t = (struct timeval){20, 1};

	stdConds[8] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[8][0].attr = EDG_WLL_QUERY_ATTR_LEVEL;
	stdConds[8][0].op = EDG_WLL_QUERY_OP_WITHIN;
	stdConds[8][0].value.i = 10;
	stdConds[8][0].value2.i = 20;

	stdConds[9] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[9][0].attr = EDG_WLL_QUERY_ATTR_HOST;
	stdConds[9][0].op = EDG_WLL_QUERY_OP_EQUAL;
	stdConds[9][0].value.c = strdup("any.host");

	stdConds[10] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[10][0].attr = EDG_WLL_QUERY_ATTR_SOURCE;
	stdConds[10][0].op = EDG_WLL_QUERY_OP_EQUAL;
	stdConds[10][0].value.i = 2;

	stdConds[11] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[11][0].attr = EDG_WLL_QUERY_ATTR_INSTANCE;
	stdConds[11][0].op = EDG_WLL_QUERY_OP_EQUAL;
	stdConds[11][0].value.c = strdup("any.instance");

	stdConds[12] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[12][0].attr = EDG_WLL_QUERY_ATTR_EVENT_TYPE;
	stdConds[12][0].op = EDG_WLL_QUERY_OP_EQUAL;
	stdConds[12][0].value.i = 1;

	stdConds[13] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[13][0].attr = EDG_WLL_QUERY_ATTR_RESUBMITTED;
	stdConds[13][0].op = EDG_WLL_QUERY_OP_EQUAL;
	stdConds[13][0].value.c = strdup("where");

	stdConds[14] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[14][0].attr = EDG_WLL_QUERY_ATTR_PARENT;
	stdConds[14][0].op = EDG_WLL_QUERY_OP_EQUAL;
	edg_wlc_JobIdCreate("my.server.org", 8000, &(stdConds[14][0].value.j));

	stdConds[15] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[15][0].attr = EDG_WLL_QUERY_ATTR_EXITCODE;
	stdConds[15][0].op = EDG_WLL_QUERY_OP_LESS;
	stdConds[15][0].value.i = 255;
/*
 * XXX: what is that for?
	stdConds[13] = (edg_wll_QueryRec *)calloc(2, sizeof(edg_wll_QueryRec));
	stdConds[13][0].attr = EDG_WLL_QUERY_ATTR_CHKPT_TAG;
	stdConds[13][0].op = EDG_WLL_QUERY_OP_EQUAL;
	stdConds[13][0].value.i = 1;
*/
}

int SoapConvTest::stdCondsCmp(edg_wll_QueryRec **c1, edg_wll_QueryRec **c2)
{
	int		i, j;


	if ( (c1 && !c2) || (!c1 && c2) ) return 1;
	if ( c1 ) for ( i = 0; c1[i]; i++ ) {
		if ( !c2[i] ) return 2;
		for ( j = 0; c1[i][j].attr; j++ ) {
			if ( !c2[i][j].attr ) return 3;
			if ( stdRecCmp(c1[i][j], c2[i][j]) ) return 4;
		}
		if ( c2[i][j].attr ) return 3;
	}
	if ( c2[i] ) return 2;

	return 0;
}

int SoapConvTest::stdRecCmp(edg_wll_QueryRec &qr1, edg_wll_QueryRec &qr2)
{
	if ( qr1.attr != qr2.attr ) return 1;
	if ( qr1.op != qr2.op ) return 1;
	switch ( qr1.attr) {
	case EDG_WLL_QUERY_ATTR_USERTAG:
		if ( strcmp(qr1.attr_id.tag, qr2.attr_id.tag) ) return 1;
	case EDG_WLL_QUERY_ATTR_OWNER:
	case EDG_WLL_QUERY_ATTR_LOCATION:
	case EDG_WLL_QUERY_ATTR_DESTINATION:
	case EDG_WLL_QUERY_ATTR_HOST:
	case EDG_WLL_QUERY_ATTR_INSTANCE:
		if ( (qr1.value.c && !qr2.value.c) || (!qr1.value.c && qr2.value.c) ) return 1;
		if ( qr1.value.c && qr2.value.c && strcmp(qr1.value.c, qr2.value.c) ) return 1;
		break;
	case EDG_WLL_QUERY_ATTR_JOBID:
	case EDG_WLL_QUERY_ATTR_PARENT: {
		char *s1, *s2;
		int	rv;

		s1 = edg_wlc_JobIdUnparse(qr1.value.j);
		s2 = edg_wlc_JobIdUnparse(qr2.value.j);
		if ( !s1 || !s2 ) rv = 1;
		else rv = strcmp(s1, s2);
		free(s1); free(s2);
		return rv;
		}
		break;
	case EDG_WLL_QUERY_ATTR_STATUS:
	case EDG_WLL_QUERY_ATTR_DONECODE:
	case EDG_WLL_QUERY_ATTR_LEVEL:
	case EDG_WLL_QUERY_ATTR_SOURCE:
	case EDG_WLL_QUERY_ATTR_EVENT_TYPE:
	case EDG_WLL_QUERY_ATTR_RESUBMITTED:
	case EDG_WLL_QUERY_ATTR_EXITCODE:
		if (   (qr1.value.i != qr2.value.i)
			|| (qr1.op == EDG_WLL_QUERY_OP_WITHIN && qr1.value2.i != qr2.value2.i) )
			return 1;
		break;
	case EDG_WLL_QUERY_ATTR_TIME:
		if (   (qr1.value.t.tv_sec != qr2.value.t.tv_sec
				|| qr1.value.t.tv_usec != qr2.value.t.tv_usec)
			|| (qr1.op == EDG_WLL_QUERY_OP_WITHIN
				&& (qr1.value2.t.tv_sec != qr2.value2.t.tv_sec
					|| qr1.value2.t.tv_usec != qr2.value2.t.tv_usec)) )
			return 1;
		break;
	/*
	 *	XXX: what about EDG_WLL_QUERY_ATTR_CHKPT_TAG   ???
	 */
	default:
		return 1;
	}

	return 0;
}

int SoapConvTest::soapCondsCmp(struct edgwll__QueryConditions &qc1, struct edgwll__QueryConditions &qc2)
{
	int		i, j;


	if ( qc1.__sizecondition != qc2.__sizecondition ) return 1;
	if ( (qc1.condition && !qc2.condition) || (!qc1.condition && qc2.condition) )
	for ( i = 0; i < qc1.__sizecondition; i++ ) {
		if ( qc1.condition[i]->attr != qc2.condition[i]->attr ) return 2;
		if ( qc1.condition[i]->__sizerecords != qc2.condition[i]->__sizerecords ) return 3;
		for ( j = 0; j < qc1.condition[i]->__sizerecords; j++ )
			if ( soapRecCmp(*(qc1.condition[i]->records[j]),
							*(qc2.condition[i]->records[j])) ) return 4;
	}

	return 0;
}

int SoapConvTest::soapRecCmp(struct edgwll__QueryRec &qr1, struct edgwll__QueryRec &qr2)
{
	if ( qr1.op != qr2.op ) return 1;
	if (   (qr1.attrid->tag && !qr2.attrid->tag)
		|| (!qr1.attrid->tag && qr2.attrid->tag)
		|| (qr1.attrid->tag && strcmp(qr1.attrid->tag, qr2.attrid->tag)) ) return 2;
	if (   (qr1.attrid->state && !qr2.attrid->state)
		|| (!qr1.attrid->state && qr2.attrid->state)
		|| (qr1.attrid->state && (qr1.attrid->state != qr2.attrid->state)) ) return 3;

	if (   (qr1.value1 && !qr2.value1)
		|| (!qr1.value1 && qr2.value1) ) return 3;
	if ( qr1.value1 ) {
		if (   (qr1.value1->i && !qr2.value1->i)
			|| (!qr1.value1->i && qr2.value1->i)
			|| (qr1.value1->i && qr1.value1->i != qr2.value1->i) )
			return 4;
		if (   (qr1.value1->c && !qr2.value1->c)
			|| (!qr1.value1->c && qr2.value1->c)
			|| (qr1.value1->c && strcmp(qr1.value1->c,qr2.value1->c)) )
			return 4;
		if (   (qr1.value1->t && !qr2.value1->t)
			|| (!qr1.value1->t && qr2.value1->t)
			|| (qr1.value1->t && memcmp(qr1.value1->t,qr2.value1->t,sizeof(*qr2.value1->t))) )
			return 4;
	}

	if (   (qr1.value2 && !qr2.value2)
		|| (!qr1.value2 && qr2.value2) ) return 3;
	if ( qr1.value2 ) {
		if (   (qr1.value2->i && !qr2.value2->i)
			|| (!qr1.value2->i && qr2.value2->i)
			|| (qr1.value2->i && qr1.value2->i != qr2.value2->i) )
			return 4;
		if (   (qr1.value2->c && !qr2.value2->c)
			|| (!qr1.value2->c && qr2.value2->c)
			|| (qr1.value2->c && strcmp(qr1.value2->c,qr2.value2->c)) )
			return 4;
		if (   (qr1.value2->t && !qr2.value2->t)
			|| (!qr1.value2->t && qr2.value2->t)
			|| (qr1.value2->t && memcmp(qr1.value2->t,qr2.value2->t,sizeof(*qr2.value2->t))) )
			return 4;
	}


	return 0;
}

void SoapConvTest::Conditions()
{
	struct edgwll__QueryConditions	   *soapConds, *soapConds2;
	edg_wll_QueryRec				  **stdConds2;
	int									ret;
	int size;
	void *tmp1;

	ret = edg_wll_QueryCondsExtToSoap(soap, (const edg_wll_QueryRec**)stdConds, &soapConds);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_QueryCondsExtToSoap()", ret == SOAP_OK);
	ret = edg_wll_SoapToQueryCondsExt(soapConds, &stdConds2, &size, &tmp);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_SoapToQueryCondsExt()", !ret);

	CPPUNIT_ASSERT_MESSAGE("Converted std results differs", !stdCondsCmp(stdConds, stdConds2));

	ret = edg_wll_QueryCondsExtToSoap(soap, (const edg_wll_QueryRec**)stdConds2, &soapConds2);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_QueryCondsExtToSoap()", ret == SOAP_OK);

	CPPUNIT_ASSERT_MESSAGE("Converted soap results differs", !soapCondsCmp(*soapConds, *soapConds2));
}

void SoapConvTest::States()
{
	struct edgwll__JobStat	   *soapStat;
	edg_wll_JobStat				stdStat2;
}

CPPUNIT_TEST_SUITE_REGISTRATION(SoapConvTest);

int main (int ac,const char *av[])
{
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
	CppUnit::TextUi::TestRunner runner;

	runner.addTest(suite);
	return runner.run() ? 0 : 1;
}
