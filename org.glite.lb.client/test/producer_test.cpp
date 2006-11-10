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

#include "glite/lb/context-int.h"
#include "glite/lb/log_proto.h"

extern "C" {
int edg_wll_log_write(edg_wll_Context, int *,char *);
int edg_wll_log_read(edg_wll_Context, int *);
int edg_wll_log_proxy_write(edg_wll_Context, int *,char *);
int edg_wll_log_proxy_read(edg_wll_Context, int *);
int edg_wll_log_direct_write(edg_wll_Context, int *,char *);
int edg_wll_log_direct_read(edg_wll_Context, int *);
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
    edg_wll_Context context;
    int err;
    static char *tst_msg = "DATE=20040831150159.702224 HOST=\"some.host\" PROG=edg-wms LVL=USAGE DG.PRIORITY=0 DG.SOURCE=\"UserInterface\" DG.SRC_INSTANCE=\"\" DG.EVNT=\"RegJob\" DG.JOBID=\"https://some.host:1234/x67qr549qc\" DG.SEQCODE=\"UI=2:NS=0:WM=0:BH=1:JSS=0:LM=0:LRMS=0:APP=0\" DG.REGJOB.JDL=\"\" DG.REGJOB.NS=\"ns address\" DG.REGJOB.PARENT=\"\" DG.REGJOB.JOBTYPE=\"SIMPLE\" DG.REGJOB.NSUBJOBS=\"0\" DG.REGJOB.SEED=\"\"";
    int size = strlen(tst_msg)+1+EDG_WLL_LOG_SOCKET_HEADER_LENGTH+sizeof(size);

    err = edg_wll_InitContext(&context);
    CPPUNIT_ASSERT(err == 0);
    err = edg_wll_log_write(context, &pd[1], tst_msg);
    CPPUNIT_ASSERT(err == size);
    err = edg_wll_log_read(context, &pd[1]);
    CPPUNIT_ASSERT(err == 0);
    log_proto_server(pd[0], tst_msg);
    edg_wll_FreeContext(context);
  }

private:
  int  pd[2];
  int sock;

  void log_proto_server(int con, char *logline) {
    int i;
    char b[5];
    char *buf;
    ssize_t size, retsize;

    // read DGLOG
    retsize = read(con, b, 5);
    CPPUNIT_ASSERT(retsize == 5);
    CPPUNIT_ASSERT(b[0] == 'D' && b[1] == 'G' && b[2] == 'L' && b[3] == 'O' && b[4] == 'G');

    // read size (including '\0', little endian)
    for (i = 0; i < 4; i++)
      CPPUNIT_ASSERT(read(con, b + i, 1) == 1);
    size = 0;
    for (i = 0; i < 4; i++)
      size = (size << 8) + b[3-i];

    // read the message
    buf = (char *)malloc(size);
    retsize = read(con, buf, size);
    CPPUNIT_ASSERT(size == retsize);

    CPPUNIT_ASSERT(strcmp(buf, logline) == 0);
    free(buf);
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
