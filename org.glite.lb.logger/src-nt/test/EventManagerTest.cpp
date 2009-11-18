#include <cppunit/extensions/HelperMacros.h>

#include "EventManager.H"

class EventA : public Event {
};

class EventB : public Event {
};

class EventAA : public EventA {
};


class EventManagerTest: public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(EventManagerTest);
	CPPUNIT_TEST(handleEventTest);
	CPPUNIT_TEST_SUITE_END();
public:
	
	void setUp() {
		handled = false;
		manager.registerHandler(this);
	}

	void tearDown() {
	}

	void handleEventTest() {
		Event *e = new EventAA();
		manager.postEvent(e);
		CPPUNIT_ASSERT(handled);
	}

	int handleEvent(EventA* &e) {
		handled = true;
		return 0;
	}

private:
	EventManager manager;
	bool handled;
};

CPPUNIT_TEST_SUITE_REGISTRATION( EventManagerTest );
