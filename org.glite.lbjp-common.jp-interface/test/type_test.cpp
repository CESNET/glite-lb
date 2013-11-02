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

#include <assert.h>
#include <fstream>
#include <cstdlib>
#include <cstring>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>

#include "types.h"
#include "attr.h"
#include "context.h"
#include "backend.h"


class TypePluginTest: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TypePluginTest);
	CPPUNIT_TEST(simple);
	CPPUNIT_TEST(binary);
	CPPUNIT_TEST(origin);
	CPPUNIT_TEST(origin2);
	CPPUNIT_TEST(index);
	CPPUNIT_TEST_SUITE_END();
public:
	void simple();
	void binary();
	void origin();
	void origin2();
	void index();
};

void TypePluginTest::simple()
{	
	glite_jp_context_t	ctx;

	glite_jp_attrval_t	attr = {
		const_cast<char *>("myattr"),
		const_cast<char *>("short string"),
		0,0,
		GLITE_JP_ATTR_ORIG_USER,
		NULL,
		0
	},attr2;

	char	*db;

	glite_jp_init_context(&ctx);
	attr.timestamp = time(NULL);

	db = glite_jp_attrval_to_db_full(ctx,&attr);

	CPPUNIT_ASSERT_MESSAGE(std::string("glite_jp_attrval_to_db_full()"),db);
	std::cerr << db << std::endl; 

	glite_jp_attrval_from_db(ctx,db,&attr2);
	CPPUNIT_ASSERT_MESSAGE(std::string("value"),!strcmp(attr.value,attr2.value));
	CPPUNIT_ASSERT_MESSAGE(std::string("origin"),attr.origin == attr2.origin);
	CPPUNIT_ASSERT_MESSAGE(std::string("timestamp"),attr.timestamp == attr2.timestamp);
}

void TypePluginTest::binary()
{
	glite_jp_context_t	ctx;

	glite_jp_attrval_t	attr = {
		const_cast<char *>("myattr"),
		NULL,
		1,1000,
		GLITE_JP_ATTR_ORIG_USER,
		NULL,
		0
	},attr2;

	char	*db;

	glite_jp_init_context(&ctx);
	attr.timestamp = time(NULL);
	attr.value = (char *) malloc(attr.size);

	db = glite_jp_attrval_to_db_full(ctx,&attr);

	CPPUNIT_ASSERT_MESSAGE(std::string("glite_jp_attrval_to_db_full()"),db);
	std::cerr << db << std::endl; 

	glite_jp_attrval_from_db(ctx,db,&attr2);
	CPPUNIT_ASSERT_MESSAGE(std::string("size"),attr.size == attr2.size);
	CPPUNIT_ASSERT_MESSAGE(std::string("value"),!memcmp(attr.value,attr2.value,attr.size));
}

void TypePluginTest::origin()
{
	glite_jp_context_t	ctx;

	glite_jp_attrval_t	attr = {
		const_cast<char *>("myattr"),
		NULL,
		0,0,
		GLITE_JP_ATTR_ORIG_USER,
		NULL,
		0
	},attr2;

	char	*db;

	glite_jp_init_context(&ctx);
	attr.timestamp = time(NULL);
	attr.value = const_cast<char *>("origin test");
	attr.origin_detail = const_cast<char *>("simple origin");

	db = glite_jp_attrval_to_db_full(ctx,&attr);

	CPPUNIT_ASSERT_MESSAGE(std::string("glite_jp_attrval_to_db_full()"),db);
	std::cerr << db << std::endl; 

	glite_jp_attrval_from_db(ctx,db,&attr2);
	CPPUNIT_ASSERT_MESSAGE(std::string("origin detail"),!strcmp(attr.origin_detail,attr2.origin_detail));
}

void TypePluginTest::origin2()
{
	glite_jp_context_t	ctx;

	glite_jp_attrval_t	attr = {
		const_cast<char *>("myattr"),
		NULL,
		0,0,
		GLITE_JP_ATTR_ORIG_USER,
		NULL,
		0
	},attr2;

	char	*db;

	glite_jp_init_context(&ctx);
	attr.timestamp = time(NULL);
	attr.value = const_cast<char *>("origin:test");
	attr.origin_detail = const_cast<char *>("ftp://some.server:1234/ugly \\file");

	db = glite_jp_attrval_to_db_full(ctx,&attr);

	CPPUNIT_ASSERT_MESSAGE(std::string("glite_jp_attrval_to_db_full()"),db);
	std::cerr << db << std::endl; 

	glite_jp_attrval_from_db(ctx,db,&attr2);
	CPPUNIT_ASSERT_MESSAGE(std::string("origin detail"),!strcmp(attr.origin_detail,attr2.origin_detail));
	CPPUNIT_ASSERT_MESSAGE(std::string("value"),!strcmp(attr.value,attr2.value));
}

void TypePluginTest::index()
{
	/* TODO: check monotonity */
}

CPPUNIT_TEST_SUITE_REGISTRATION(TypePluginTest);


int main (int argc,const char *argv[])
{
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

	assert(argc == 2);
	std::ofstream	xml(argv[1]);

	CppUnit::TestResult controller;
	CppUnit::TestResultCollector result;
	controller.addListener( &result );

	CppUnit::TestRunner runner;
	runner.addTest(suite);
	runner.run(controller);

	CppUnit::XmlOutputter xout( &result, xml );
	CppUnit::CompilerOutputter tout( &result, std::cout);
	xout.write();
	tout.write();

	return result.wasSuccessful() ? 0 : 1 ;
}




/* fake to link */
int glite_jppsbe_pread(
        glite_jp_context_t ctx,
        void *handle,
        void *buf,
        size_t nbytes,
        off_t offset,
        ssize_t *nbytes_ret
)
{
	abort();
}


