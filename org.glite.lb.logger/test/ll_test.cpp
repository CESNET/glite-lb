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
    log_proto_client(pd[1], msg);
  }

  void tearDown() {
    close(pd[0]);
    close(pd[1]);
  }

  void testProtoServer() {
    int ret = edg_wll_log_proto_server(con, 
				       "michal",
				       "/tmp/dglogd.log",
				       0,
				       0);
    CPPUNIT_ASSERT( ret == 0 );
  }

private:
  int  pd[2];
  const char *msg = "";

  int log_proto_client(int con, char *logline) {
    char	header[32];
    int	err;
    int	size;
    u_int8_t size_end[4];

    err = 0;
    size = strlen(logline)+1;
    size_end[0] = size & 0xff; size >>= 8;
    size_end[1] = size & 0xff; size >>= 8;
    size_end[2] = size & 0xff; size >>= 8;
    size_end[3] = size;
    size = strlen(logline)+1;

    err = write(con, "DGLOG", 5);
    CPPUNIT_ASSERT(err == 5);
    err = write(con, size_end, 4);
    CPPUNIT_ASSERT(err == 4);
    err = write(con, logline, size);
    CPPUNIT_ASSERT( err == size );
}

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
