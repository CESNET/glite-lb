#include <cppunit/extensions/HelperMacros.h>
 
#include "IlTestBase.h"

#include <string.h>

using namespace std;

class server_msgTest: public CppUnit::TestFixture 
{
	CPPUNIT_TEST_SUITE(server_msgTest);
	CPPUNIT_TEST( server_msg_createTest );
	CPPUNIT_TEST( server_msg_copyTest );
	CPPUNIT_TEST_SUITE_END();

public:

	void setUp() {
		msg = server_msg_create((char *)IlTestBase::msg);
	}

	void tearDown() {
		server_msg_free(msg);
	}

	void server_msg_createTest() {
		CPPUNIT_ASSERT( msg != NULL );
		CPPUNIT_ASSERT_EQUAL( string(msg->job_id_s), string(IlTestBase::smsg.job_id_s) );
		CPPUNIT_ASSERT_EQUAL( string(msg->msg), string(IlTestBase::smsg.msg) );
		CPPUNIT_ASSERT_EQUAL( msg->len, IlTestBase::smsg.len );
		CPPUNIT_ASSERT_EQUAL( msg->ev_len, IlTestBase::smsg.ev_len );
		CPPUNIT_ASSERT_EQUAL( msg->es, IlTestBase::smsg.es );
		CPPUNIT_ASSERT( !server_msg_is_priority(msg) );
	}

	void server_msg_copyTest() {
		struct server_msg *msg2;

		msg2 = server_msg_copy(msg);
		CPPUNIT_ASSERT( msg2 != NULL );
		CPPUNIT_ASSERT( msg2 != msg );
		CPPUNIT_ASSERT_EQUAL( string(msg->job_id_s), string(msg2->job_id_s) );
		CPPUNIT_ASSERT( msg->job_id_s != msg2->job_id_s);
		CPPUNIT_ASSERT_EQUAL( string(msg->msg), string(msg2->msg) );
		CPPUNIT_ASSERT( msg->msg != msg2->msg );
		CPPUNIT_ASSERT_EQUAL( msg->len, msg2->len );
		CPPUNIT_ASSERT_EQUAL( msg->ev_len, msg2->ev_len );
		CPPUNIT_ASSERT_EQUAL( msg->es, msg2->es );
		server_msg_free(msg2);
	}

private:
	struct server_msg *msg;
};


CPPUNIT_TEST_SUITE_REGISTRATION(server_msgTest);
