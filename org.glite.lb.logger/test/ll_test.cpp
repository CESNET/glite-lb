#include <iostream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>


class LLTest: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(LLTest);
	CPPUNIT_TEST(testProtoServer);
	CPPUNIT_TEST_SUITE_END();

public:

  void setUp() {
  }

  void tearDown() {
  }

  void testProtoServer() {
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION( LLTest );

int main (int ac,const char *av[])
{
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
	CppUnit::TextUi::TestRunner runner;
	
	runner.addTest(suite);
	return runner.run() ? 0 : 1;
}
