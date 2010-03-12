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

#include "Singleton.H"

class one : public Singleton<one> {
	friend class Singleton<one>;
};

class two : public Singleton<two> {
	friend class Singleton<two>;
};

class SingletonTest: public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(SingletonTest);
	CPPUNIT_TEST(getInstance);
	CPPUNIT_TEST(pair);
	CPPUNIT_TEST(noCreate);
	CPPUNIT_TEST_SUITE_END();
public:
	
	void setUp() {
	}

	void tearDown() {
	}

	void getInstance() {
		one *p;
		one *q;

		p = one::instance();
		q = one::instance();
		CPPUNIT_ASSERT(p != NULL);
		CPPUNIT_ASSERT(q != NULL);
		CPPUNIT_ASSERT(p == q);
	}

	void pair() {
		one *p;
		two *q;

		p = one::instance();
		q = two::instance();
		CPPUNIT_ASSERT(p != NULL);
		CPPUNIT_ASSERT(q != NULL);
		CPPUNIT_ASSERT((void*)p != (void*)q);
	}

	void noCreate() {
		one *p = new one;

	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( SingletonTest );
