#include <cppunit/extensions/HelperMacros.h>


extern "C" {
#ifdef BUILDING_LB_COMMON
#include "il_string.h"
#else
#include "glite/lb/il_string.h"
#endif
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
		CPPUNIT_ASSERT(!strncmp(buffer, "17\n", 3));
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

