#include <iostream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "trio.h"

class TrioTest: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TrioTest);
	CPPUNIT_TEST(escapeULM);
	CPPUNIT_TEST(escapeXML);
	CPPUNIT_TEST(escapeSQL);
	CPPUNIT_TEST_SUITE_END();

public:
	void escapeULM();
	void escapeXML();
	void escapeSQL();
};

void TrioTest::escapeULM()
{
	char	*e, *r = "START we have =, \\\", and \\n in the string END";

	trio_asprintf(&e,"START %|Us END", "we have =, \", and \n in the string"),
	std::cerr << e << std::endl;

	CPPUNIT_ASSERT_MESSAGE("escape ULM failed",!strcmp(e,r));
}

void TrioTest::escapeXML()
{
	char	*e, *r = "START there is a &lt;tag&gt; containing &amp;something; &lt;/tag&gt; END";

	trio_asprintf(&e,"START %|Xs END", "there is a <tag> containing &something; </tag>"),
	std::cerr << e << std::endl;

	CPPUNIT_ASSERT_MESSAGE("escape XML failed",!strcmp(e,r));
}

void TrioTest::escapeSQL()
{
	char	*e, *r = "START SQL doesn''t like '' END";

	trio_asprintf(&e,"START %|Ss END", "SQL doesn't like '"),
	std::cerr << e << std::endl;

	CPPUNIT_ASSERT_MESSAGE("escape SQL failed",!strcmp(e,r));
}

CPPUNIT_TEST_SUITE_REGISTRATION( TrioTest );

#include <assert.h>
#include <fstream>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>

int main (int argc,const char *argv[])
{
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

	assert(argc == 2);
	std::ofstream	xml(argv[1]);

	CppUnit::TestResult controller;
	CppUnit::TestResultCollector result;
	controller.addListener( &result );

	CppUnit::TestRunner runner;
	runner.addTest(suite);
	runner.run(controller);

	CppUnit::XmlOutputter xout( &result, xml );
	CppUnit::CompilerOutputter tout( &result, std::cout);
	xout.write();
	tout.write();

	return result.wasSuccessful() ? 0 : 1 ;
}
