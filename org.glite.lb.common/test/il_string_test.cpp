#include <cppunit/extensions/HelperMacros.h>

extern "C" {
#include "il_string.h"
}

class IlStringTest : public CppUnit::TestFixture 
{
	CPPUNIT_TEST_SUITE( IlStringTest );
	CPPUNIT_TEST( testPutString );
	CPPUNIT_TEST( testGetString );
	CPPUNIT_TEST( testLenString );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {
	}

	void tearDown() {
	}

	void testPutString() {
		put_string(buffer, "ahoj");
		CPPUNIT_ASSERT( !strncmp(buffer,"4 ahoj\n",7) );
	}

	void testGetString() {
		char *s;
		get_string("4 ahoj\n", &s);
		CPPUNIT_ASSERT( s != NULL );
		CPPUNIT_ASSERT( !strcmp(s, "ahoj") );
		CPPUNIT_ASSERT( s[4] == 0 );
		free(s);
	}

	void testLenString() {
		int d = len_string("ahoj");
		CPPUNIT_ASSERT( d == 7);
	}

private:
	char buffer[255];
};

CPPUNIT_TEST_SUITE_REGISTRATION( IlStringTest );
