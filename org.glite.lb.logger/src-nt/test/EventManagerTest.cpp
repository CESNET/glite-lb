#include <cppunit/extensions/HelperMacros.h>

#include "EventManager.H"

class EventManagerTest: public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(EventManagerTest);
	CPPUNIT_TEST_SUITE_END();
public:
	
	void setUp() {
	}

	void tearDown() {
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION( EventManagerTest );
