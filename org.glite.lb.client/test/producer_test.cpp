#include <iostream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

extern "C" {
int edg_wll_log_proto_client(int *,char *,char *,int,int);
}

class ProducerTest: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(ProducerTest);
	CPPUNIT_TEST(testProtoClient);
	CPPUNIT_TEST_SUITE_END();

public:

  void setUp() {
    pipe(pd);
  }

  void tearDown() {
    close(pd[0]);
    close(pd[1]);
  }

  void testProtoClient() {
    int ret=0;
    CPPUNIT_ASSERT( ret == 0 );
  }

private:
  int  pd[2];

  int log_proto_server(int con, char *logline) {
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION( ProducerTest );

int 
main (int ac,const char *av[])
{
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
	CppUnit::TextUi::TestRunner runner;
	
	runner.addTest(suite);
	return runner.run() ? 0 : 1;
}
