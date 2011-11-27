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

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include "db.h"

#define zoneCheck(tmp) {\
	(tmp) = getenv("TZ"); \
	if (tz == NULL) CPPUNIT_ASSERT((tmp) == NULL);\
	else CPPUNIT_ASSERT((tmp) != NULL && strcmp(tz, (tmp)) == 0);\
}

static struct {
	time_t t;
	const char *db;
} data[] = {
	// year of tiger (and day +, day -)
	{t:1266142830, db:"2010-02-14 10:20:30"},
	{t:1266142830-24*3600, db:"2010-02-13 10:20:30"},
	{t:1266142830+24*3600, db:"2010-02-15 10:20:30"},

	// two months later (and day +, day -)
	{t:1271240430, db:"2010-04-14 10:20:30"},
	{t:1271240430-24*3600, db:"2010-04-13 10:20:30"},
	{t:1271240430+24*3600, db:"2010-04-15 10:20:30"},
};

class ZoneTest: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(ZoneTest);
	CPPUNIT_TEST(testInitialZoneCheck);
	CPPUNIT_TEST(testTimeZone3);
	CPPUNIT_TEST(testTimeZoneNULL);
	CPPUNIT_TEST(testTimeZoneUTC);
	CPPUNIT_TEST_SUITE_END();

public:

	void setUp() {
		tz = tz0 = getenv("TZ");
	}

	void tearDown() {
		if (tz0) setenv("TZ", tz0, 1);
		else unsetenv("TZ");
	}

	void testInitialZoneCheck() {
		char *tz2;

		zoneCheck(tz2);
	}

	void testTimeZone3() {
		tz = "Tst 3:00";
		setenv("TZ", tz, 1);
		tzset();
		testInitialZoneCheck();
		convertCheck();
	}

	void testTimeZoneNULL() {
		tz = NULL;
		unsetenv("TZ");
		tzset();
		testInitialZoneCheck();
		convertCheck();
	}

	void testTimeZoneUTC() {
		tz = "UTC";
		setenv("TZ", tz, 1);
		tzset();
		testInitialZoneCheck();
		convertCheck();
	}

private:

const char *tz0, *tz;

void convertCheck() {
	char *tz2;

	time_t tt;
	double st_d;
	char *st_str, *st_dbstr, *t_dbstr;

	char *s;
	time_t t;
	double d;
	size_t i;

	for (i = 0; i < sizeof(data)/sizeof(data[0]); i++) {
		tt = data[i].t;
		st_d = tt + 0.013;
		asprintf(&t_dbstr, "'%s'", data[i].db);
		asprintf(&st_str, "%s.013", data[i].db);
		asprintf(&st_dbstr, "'%s.013'", data[i].db);

		glite_lbu_TimeToStr(tt, &s);
		CPPUNIT_ASSERT(s != NULL);
		CPPUNIT_ASSERT(strcmp(s, t_dbstr) == 0);
		free(s);
		zoneCheck(tz2);

		glite_lbu_TimestampToStr(st_d, &s);
		CPPUNIT_ASSERT(s != NULL);
		CPPUNIT_ASSERT(strncmp(s, st_dbstr, 24) == 0);
		free(s);
		zoneCheck(tz2);

		t = glite_lbu_StrToTime(data[i].db);
		CPPUNIT_ASSERT(t == tt);
		zoneCheck(tz2);

		d = glite_lbu_StrToTimestamp(st_str);
		CPPUNIT_ASSERT(round(1000 * d) == round(1000 * st_d));
		zoneCheck(tz2);

		free(st_str);
		free(st_dbstr);
		free(t_dbstr);
	}
}

};

CPPUNIT_TEST_SUITE_REGISTRATION( ZoneTest );

int 
main ()
{
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
	CppUnit::TextUi::TestRunner runner;
	
	runner.addTest(suite);
	return runner.run() ? 0 : 1;
}
