#include <iostream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

extern "C" {
#define DEFAULT_SOCKET "/tmp/interlogger.sock"
char *socket_path = DEFAULT_SOCKET;
}

class LLTest: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(LLTest);
	CPPUNIT_TEST(testProtoServer);
	CPPUNIT_TEST_SUITE_END();

public:

  void setUp() {
    pipe(pd);
  }

  void tearDown() {
    close(pd[0]);
    close(pd[1]);
  }

  void testProtoServer() {
  }

private:
  int  pd[2];
};


CPPUNIT_TEST_SUITE_REGISTRATION( LLTest );

int 
main (int ac,const char *av[])
{
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
	CppUnit::TextUi::TestRunner runner;
	
	runner.addTest(suite);
	return runner.run() ? 0 : 1;
}
