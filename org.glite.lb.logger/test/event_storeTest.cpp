#include <cppunit/extensions/HelperMacros.h>
 
#include "IlTestBase.h"

class event_storeTest: public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( event_storeTest );
	CPPUNIT_TEST( event_store_recoverTest );
	CPPUNIT_TEST( event_store_syncTest );
	CPPUNIT_TEST( event_store_nextTest );
	CPPUNIT_TEST( event_store_commitTest );
	CPPUNIT_TEST( event_store_cleanTest );
	CPPUNIT_TEST( event_store_findTest );
	CPPUNIT_TEST( event_store_releaseTest );
	CPPUNIT_TEST( event_store_initTest );
	CPPUNIT_TEST( event_store_recover_allTest );
	CPPUNIT_TEST( event_store_cleanupTest );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {
	}

	void tearDown() {
	}

	void event_store_recoverTest() {
	}

	void event_store_syncTest() {
	}

	void event_store_nextTest() {
	}

	void event_store_commitTest() {
	}

	void event_store_cleanTest() {
	}

	void event_store_findTest() {
	}

	void event_store_releaseTest() {
	}

	void event_store_initTest() {
	}

	void event_store_recover_allTest() {
	}

	void event_store_cleanupTest() {
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( event_storeTest );
