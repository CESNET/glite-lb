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

