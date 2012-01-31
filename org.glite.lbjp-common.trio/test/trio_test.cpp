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

#include <iostream>
#include <cstring>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "trio.h"

class TrioTest: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TrioTest);
	CPPUNIT_TEST(escapeULM);
	CPPUNIT_TEST(escapeXML);
	CPPUNIT_TEST(escapeSQL);
	CPPUNIT_TEST(escapeJSON);
	CPPUNIT_TEST_SUITE_END();

public:
	void escapeULM();
	void escapeXML();
	void escapeSQL();
	void escapeJSON();
};

void TrioTest::escapeULM()
{
	char	*e, *r = "START we have =, \\\", and \\n in the string END";

	trio_asprintf(&e,"START %|Us END", "we have =, \", and \n in the string"),
	std::cerr << e << std::endl;

	CPPUNIT_ASSERT_MESSAGE("escape ULM failed",!strcmp(e,r));

	free(e);
}

void TrioTest::escapeXML()
{
	char	*e, *r = "START there is a &lt;tag&gt; containing &amp;something; &lt;/tag&gt; END";

	trio_asprintf(&e,"START %|Xs END", "there is a <tag> containing &something; </tag>"),
	std::cerr << e << std::endl;

	CPPUNIT_ASSERT_MESSAGE("escape XML failed",!strcmp(e,r));

	free(e);
}

void TrioTest::escapeSQL()
{
	char	*e, *r = "START SQL doesn''t like '' END";

	trio_asprintf(&e,"START %|Ss END", "SQL doesn't like '"),
	std::cerr << e << std::endl;

	CPPUNIT_ASSERT_MESSAGE("escape SQL failed",!strcmp(e,r));

	free(e);
}

void TrioTest::escapeJSON() {
	char	*e, *r = "START Jason doesn't like: \\\\\\n\\r\\b\\r\\t\\f and \\u001b END";
	int	ret;

	ret = trio_asprintf(&e, "START %|Js END", "Jason doesn't like: \\\n\r\b\r\t\f and \x1B");
	std::cerr << e << std::endl;

	CPPUNIT_ASSERT_MESSAGE("escape JSON failed",ret > 0 && strcmp(e,r) == 0);

	free(e);
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
