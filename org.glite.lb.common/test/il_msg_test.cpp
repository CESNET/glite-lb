#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>

extern "C" {
#include "il_string.h"
#include "il_msg.h"
}

class IlMsgTest : public CppUnit::TestFixture 
{
	CPPUNIT_TEST_SUITE ( IlMsgTest );
	CPPUNIT_TEST( testEncodeMsg );
	CPPUNIT_TEST( testDecodeMsg );
	CPPUNIT_TEST( testEncodeReply );
	CPPUNIT_TEST( testDecodeReply );
	CPPUNIT_TEST( testReadData );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {
		len_msg = encode_il_msg(&buffer_msg, "zprava");
		len_rep = encode_il_reply(&buffer_rep, 10, 20, "chyba");
	}

	void tearDown() {
		free(buffer_msg);
		free(buffer_rep);
	}

	void testEncodeMsg() {
		CPPUNIT_ASSERT_EQUAL(len_msg, 26);
		CPPUNIT_ASSERT(buffer_msg != NULL);
		CPPUNIT_ASSERT(!strncmp(buffer_msg, msg, len_msg));
	}

	void testDecodeMsg() {
		char *s;
		int  l;

		l = decode_il_msg(&s, buffer_msg + 17);
		CPPUNIT_ASSERT_EQUAL(l, len_msg - 17);
		CPPUNIT_ASSERT(s != NULL);
		CPPUNIT_ASSERT( !strcmp(s, "zprava") );
		free(s);
	}

	void testEncodeReply() {
	}

	void testDecodeReply() {
		char *s;
		int m, n, l;

		l = decode_il_reply(&m, &n, &s, buffer_rep + 17);
		CPPUNIT_ASSERT_EQUAL(l, len_rep - 17);
		CPPUNIT_ASSERT_EQUAL(m, 10);
		CPPUNIT_ASSERT_EQUAL(n, 20);
		CPPUNIT_ASSERT( s != NULL );
		CPPUNIT_ASSERT( !strcmp(s, "chyba") );
		free(s);
	}

	void testReadData() {
		int l;
		char *s;

		l = read_il_data(&s, test_reader);
		CPPUNIT_ASSERT_EQUAL(l, 9);
		CPPUNIT_ASSERT(s != NULL);
		CPPUNIT_ASSERT(!strcmp(s, "6 zprava\n"));
		free(s);
	}

private:
	int len_msg, len_rep;
	char *buffer_msg, *buffer_rep;
	static const char *msg, *rep;

	static int pos;
	static int test_reader(char *buf, int len) {
		strncpy(buf, msg+pos, len);
		pos += len;
		return(len);
	}
};

const char *IlMsgTest::msg = "               9\n6 zprava\n";
const char *IlMsgTest::rep = "";
int IlMsgTest::pos;

CPPUNIT_TEST_SUITE_REGISTRATION( IlMsgTest );
