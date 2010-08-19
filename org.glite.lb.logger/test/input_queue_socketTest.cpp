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
