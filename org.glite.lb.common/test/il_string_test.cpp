#include <cppunit/extensions/HelperMacros.h>

extern "C" {
#include <string.h>
#ifdef BUILDING_LB_COMMON
#include "il_string.h"
#else
#include "glite/lb/il_string.h"
#endif
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
		il_octet_string_t s;
		s.data = "ahoj";
		s.len = strlen(s.data);
		put_string(buffer, &s);
		CPPUNIT_ASSERT( !strncmp(buffer,"4 ahoj\n",7) );
	}

	void testGetString() {
		il_octet_string_t s;
		get_string("4 ahoj\n", &s);
		CPPUNIT_ASSERT( s.data != NULL );
		CPPUNIT_ASSERT( !strcmp(s.data, "ahoj") );
		CPPUNIT_ASSERT( s.data[4] == 0 );
		free(s.data);
	}

	void testLenString() {
		il_octet_string_t s;
		int d;

		s.data = "ahoj";
		s.len = strlen(s.data);
		d = len_string(&s);
		CPPUNIT_ASSERT( d == 7);
	}

private:
	char buffer[255];
};

CPPUNIT_TEST_SUITE_REGISTRATION( IlStringTest );
