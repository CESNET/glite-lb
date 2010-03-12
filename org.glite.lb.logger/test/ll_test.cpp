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
#define DEFAULT_SOCKET "/tmp/interlogger.sock"
char *socket_path = DEFAULT_SOCKET;
int edg_wll_log_proto_server(int *,char *,char *,int,int);
void edg_wll_ll_log_init(int);
}

class LLTest: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(LLTest);
	CPPUNIT_TEST(testProtoServer);
	CPPUNIT_TEST_SUITE_END();

public:

  void setUp() {
    char *msg = "DATE=20040831150159.702224 HOST=\"some.host\" PROG=edg-wms LVL=USAGE DG.PRIORITY=0 DG.SOURCE=\"UserInterface\" DG.SRC_INSTANCE=\"\" DG.EVNT=\"RegJob\" DG.JOBID=\"https://some.host:1234/x67qr549qc\" DG.SEQCODE=\"UI=2:NS=0:WM=0:BH=1:JSS=0:LM=0:LRMS=0:APP=0\" DG.REGJOB.JDL=\"\" DG.REGJOB.NS=\"ns address\" DG.REGJOB.PARENT=\"\" DG.REGJOB.JOBTYPE=\"SIMPLE\" DG.REGJOB.NSUBJOBS=\"0\" DG.REGJOB.SEED=\"\"";
    pipe(pd);
    log_proto_client(pd[1], msg);
    input_queue_attach();
  }

  void tearDown() {
    close(pd[0]);
    close(pd[1]);
    input_queue_detach();
  }

  void testProtoServer() {
    int ret;
    edg_wll_ll_log_init(255);
    ret = edg_wll_log_proto_server(&pd[0], 
				       "michal",
				       "/tmp/dglogd.log",
				       0,
				       0);
    CPPUNIT_ASSERT( ret == 0 );
  }

private:
  int  pd[2];

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

 int sock;
 int accepted;

int 
input_queue_attach()
{ 
  struct sockaddr_un saddr;

  CPPUNIT_ASSERT((sock=socket(PF_UNIX, SOCK_STREAM, 0)) >= 0);

  memset(&saddr, 0, sizeof(saddr));
  saddr.sun_family = AF_UNIX;
  strcpy(saddr.sun_path, socket_path);

  CPPUNIT_ASSERT(bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) >= 0); 
  CPPUNIT_ASSERT(listen(sock, 5) >= 0 );
  return(0);
}

void input_queue_detach()
{
  if (sock >= 0)
    close(sock);
  unlink(socket_path);
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
