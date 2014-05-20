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

#include <cstring>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>

#include "testcore.h"
#include "glite_gss.h"

class GSSTest: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(GSSTest);
	CPPUNIT_TEST(echo);
	CPPUNIT_TEST(echo);
	CPPUNIT_TEST(bigecho);
	CPPUNIT_TEST(errorTest);
	CPPUNIT_TEST_SUITE_END();

public:
	void echo();
	void bigecho();
	void errorTest();

	void setUp();
	void tearDown();

private:
	test_ctx_t ctx;
	void replier();
};


void GSSTest::replier() {
	edg_wll_GssConnection	conn;
	edg_wll_GssStatus	stat;
	struct sockaddr_storage	a;
	socklen_t		alen = sizeof(a);
	int                     s, len;
	char 			buf[8*BUFSIZ];

	ctx.test_name = "replier";
	tprintf(&ctx, "pid %d\n", getpid());
	
	if ( (s = accept(ctx.sock, (struct sockaddr *) &a, &alen)) < 0 ) exit(1);
	
	if ( edg_wll_gss_accept(ctx.server_cred, s, &ctx.timeout, &conn, &stat) ) exit(1);

	while ( (len = edg_wll_gss_read(&conn, buf, sizeof(buf), &ctx.timeout, &stat)) >= 0 ) {
		if ( edg_wll_gss_write(&conn, buf, len, &ctx.timeout, &stat) ) exit(1);
	}	

	edg_wll_gss_close(&conn, &ctx.timeout);
	test_cleanup(&ctx);

	exit(0);
}


void GSSTest::setUp(void) {
	pid_t pid;

	CPPUNIT_ASSERT_MESSAGE("GSSTest setup", test_setup(&ctx, "main") == 0);

	if ( !(pid = fork()) ) replier();
	else {
		ctx.test_name = "main";
		close(ctx.sock);
		ctx.sock = -1;
	}
}


void GSSTest::tearDown(void) {
	test_cleanup(&ctx);
}


void GSSTest::echo()
{
	edg_wll_GssConnection	conn;
	edg_wll_GssStatus	stat;
	size_t			total;
	int			err;
	char 			buf[] = "f843fejwfanczn nc4*&686%$$&^(*)*#$@WSH";	
	char			buf2[100];	

	ctx.test_name = "echo";
	tprintf(&ctx, "pid %d\n", getpid());

	err = edg_wll_gss_connect(ctx.user_cred, hostname, ctx.port, &ctx.timeout, &conn, &stat);
	check_gss(&ctx, err, &stat, "GSS connect failed");
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_connect()", !err);
	
	err = edg_wll_gss_write(&conn, buf, strlen(buf)+1, &ctx.timeout, &stat);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_write()", !err);
	
	err = edg_wll_gss_read_full(&conn, buf2, strlen(buf)+1, &ctx.timeout, &total, &stat);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_read_full()", !err);

	CPPUNIT_ASSERT(strlen(buf)+1 == total && !strcmp(buf,buf2) );

	edg_wll_gss_close(&conn, &ctx.timeout);
}

void GSSTest::bigecho()
{
	edg_wll_GssConnection	conn;
	edg_wll_GssStatus	stat;
	size_t			total;
	int			err;
	char 			buf[7*BUFSIZ];
	char			buf2[7*BUFSIZ];	

	ctx.test_name = "bigecho";
	tprintf(&ctx, "pid %d\n", getpid());

	err = edg_wll_gss_connect(ctx.user_cred, hostname, ctx.port, &ctx.timeout, &conn, &stat);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_connect()", !err);
	
	err = edg_wll_gss_write(&conn, buf, sizeof buf, &ctx.timeout, &stat);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_write()", !err);
	
	err = edg_wll_gss_read_full(&conn, buf2, sizeof buf2, &ctx.timeout, &total, &stat);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_read_full()", !err);

	CPPUNIT_ASSERT(sizeof buf == total && !memcmp(buf,buf2,sizeof buf) );

	edg_wll_gss_close(&conn, &ctx.timeout);
}


void GSSTest::errorTest()
{
	edg_wll_GssConnection	conn;
	edg_wll_GssStatus	stat;
	int			err;
	char *			msg = NULL;

	ctx.test_name = "errtest";
	err = edg_wll_gss_connect(ctx.user_cred, "xxx.example.net", ctx.port, &ctx.timeout, &conn, &stat);
	if (err) edg_wll_gss_get_error(&stat, "gss_connect()", &msg);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_get_error()", msg);
}


CPPUNIT_TEST_SUITE_REGISTRATION( GSSTest );

int main (int ac, char * const av[])
{
	int ret;

	if ((ret = parse_opts(ac, av)) != 0) return ret;
	std::ofstream	xml("out.xml");

	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
	CppUnit::TestRunner runner;

	CppUnit::TestResult controller;
	CppUnit::TestResultCollector result;
	controller.addListener( &result );

	runner.addTest(suite);
	runner.run(controller);


	CppUnit::XmlOutputter xout( &result, xml );
	CppUnit::CompilerOutputter tout( &result, std::cout);
	xout.write();
	tout.write();

	return result.wasSuccessful() ? 0 : 1 ;
}
