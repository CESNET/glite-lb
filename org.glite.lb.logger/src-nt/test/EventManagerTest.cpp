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
