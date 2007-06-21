#include <cppunit/extensions/HelperMacros.h>


extern "C" {
#include "glite/lb/il_string.h"
}

class IlIntTest: public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( IlIntTest );
	CPPUNIT_TEST( testPutInt );
	CPPUNIT_TEST( testGetInt );
	CPPUNIT_TEST( testLenInt );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {
	}

	void tearDown() {
	}

	void testPutInt() {
		put_int(buffer, 17);
		CPPUNIT_ASSERT(!strcmp(buffer, "17\n"));
	}

	void testGetInt() {
		int d;
		get_int("17\n", &d);
		CPPUNIT_ASSERT(d == 17);
		CPPUNIT_ASSERT(get_int("17 \n", &d) == NULL);
	}

	void testLenInt() {
		CPPUNIT_ASSERT(3 == len_int(17));
	}

protected:
	char buffer[255];
};

CPPUNIT_TEST_SUITE_REGISTRATION( IlIntTest ) ;

