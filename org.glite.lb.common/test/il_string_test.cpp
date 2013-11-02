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
#include <cstdlib>

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
		s.data = const_cast<char *>("ahoj");
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

		s.data = const_cast<char *>("ahoj");
		s.len = strlen(s.data);
		d = len_string(&s);
		CPPUNIT_ASSERT( d == 7);
	}

private:
	char buffer[255];
};

CPPUNIT_TEST_SUITE_REGISTRATION( IlStringTest );
