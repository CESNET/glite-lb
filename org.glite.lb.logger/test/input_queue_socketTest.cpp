#include <cppunit/extensions/HelperMacros.h>
 
#include "IlTestBase.h"

extern "C" {
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "interlogd.h"

	extern char *socket_path;
}

#include <string>
using namespace std;
 
class input_queue_socketTest: public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( input_queue_socketTest );
	CPPUNIT_TEST( input_queue_getTest );
	CPPUNIT_TEST_SUITE_END();

public:

	void setUp() {
		struct sockaddr_un saddr;
		int sock;
		long offset = 0;

		int ret = input_queue_attach();
		CPPUNIT_ASSERT(ret == 0);

		sock=socket(PF_UNIX, SOCK_STREAM, 0);
		CPPUNIT_ASSERT(sock >= 0);
		
		memset(&saddr, 0, sizeof(saddr));
		saddr.sun_family = AF_UNIX;
		strcpy(saddr.sun_path, socket_path);
		ret = connect(sock, (struct sockaddr *)&saddr, sizeof(saddr.sun_path));
		CPPUNIT_ASSERT(ret >= 0);
		
		ret = write(sock, &offset, sizeof(offset));
		CPPUNIT_ASSERT( ret == sizeof(offset) );
		ret = write(sock, IlTestBase::msg, strlen(IlTestBase::msg));
		CPPUNIT_ASSERT( ret == strlen(IlTestBase::msg) );
		ret = write(sock, "\n", 1);
		CPPUNIT_ASSERT( ret == 1 );
	}

	void tearDown() {
		input_queue_detach();
	}


	void input_queue_getTest() {
		char *event;
		long offset;
		int ret;

		ret = input_queue_get(&event, &offset, 10);
		CPPUNIT_ASSERT( ret >= 0 );
		CPPUNIT_ASSERT_EQUAL( 0L, offset );
		CPPUNIT_ASSERT_EQUAL( string(IlTestBase::msg), string(event) );
		free(event);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(input_queue_socketTest);
