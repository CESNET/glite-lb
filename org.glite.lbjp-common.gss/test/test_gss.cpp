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
	edg_wll_GssCred	my_cred;
	int		sock, port;
	struct timeval	timeout;
	
	void replier();
};


void GSSTest::replier() {
	edg_wll_GssConnection	conn;
	edg_wll_GssStatus	stat;
	struct sockaddr_storage	a;
	socklen_t		alen = sizeof(a);
	int                     s, len;
	char 			buf[8*BUFSIZ];
	
	std::cerr << "replier " << getpid() << std::endl;
	
	if ( (s = accept(sock, (struct sockaddr *) &a, &alen)) < 0 ) exit(1);
	
	if ( edg_wll_gss_accept(my_cred, s, &timeout, &conn, &stat) ) exit(1);

	while ( (len = edg_wll_gss_read(&conn, buf, sizeof(buf), &timeout, &stat)) >= 0 ) {
		if ( edg_wll_gss_write(&conn, buf, len, &timeout, &stat) ) exit(1);
	}	

	edg_wll_gss_close(&conn, &timeout);

	exit(0);
}


void GSSTest::setUp(void) {
	pid_t pid;
	edg_wll_GssStatus stat;
	struct sockaddr_storage a;
	socklen_t 		alen = sizeof(a);
	char *			cred_file = NULL;
	char *			key_file = NULL;
	char * 			to = getenv("GSS_TEST_TIMEOUT");
	struct addrinfo	*ai;
	struct addrinfo	hints;
	char		servname[16];
	int		ret;


	timeout.tv_sec = to ? atoi(to) : 10 ;
	timeout.tv_usec = 0;
	my_cred = NULL;
	
	key_file = cred_file = getenv("X509_USER_PROXY");
	CPPUNIT_ASSERT_MESSAGE("credential file", cred_file);
	
	if (edg_wll_gss_acquire_cred_gsi(cred_file, key_file, &my_cred, &stat))
		CPPUNIT_ASSERT_MESSAGE("gss_acquire_cred", 0);
	
	memset (&hints, '\0', sizeof (hints));
	hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE | AI_ADDRCONFIG;
	hints.ai_socktype = SOCK_STREAM;

	snprintf(servname, sizeof servname, "%d", port);
	ret = getaddrinfo (NULL, servname, &hints, &ai);
	CPPUNIT_ASSERT_MESSAGE("getaddrinfo()", ret == 0 && ai != NULL);

	sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	CPPUNIT_ASSERT_MESSAGE("socket()", sock >= 0);

	ret = bind(sock, ai->ai_addr, ai->ai_addrlen);
	CPPUNIT_ASSERT_MESSAGE("bind()", ret == 0);

	ret = listen(sock, 1);
	CPPUNIT_ASSERT_MESSAGE("listen()", ret == 0);

	getsockname(sock,(struct sockaddr *) &a,&alen);
	ret = getnameinfo ((struct sockaddr *) &a, alen,
                NULL, 0, servname, sizeof(servname), NI_NUMERICSERV);
        CPPUNIT_ASSERT_MESSAGE("getnameinfo()", ret == 0);

	port = atoi(servname);

	if ( !(pid = fork()) ) replier();
	else close(sock);
}


void GSSTest::tearDown(void) {
	edg_wll_gss_release_cred(&my_cred, NULL);
}


void GSSTest::echo()
{
	edg_wll_GssConnection	conn;
	edg_wll_GssStatus	stat;
	size_t			total;
	int			err;
	char 			buf[] = "f843fejwfanczn nc4*&686%$$&^(*)*#$@WSH";	
	char			buf2[100];	

	std::cerr << "echo " << getpid() << std::endl;

	err = edg_wll_gss_connect(my_cred, "localhost", port, &timeout, &conn, &stat);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_connect()", !err);
	
	err = edg_wll_gss_write(&conn, buf, strlen(buf)+1, &timeout, &stat);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_write()", !err);
	
	err = edg_wll_gss_read_full(&conn, buf2, strlen(buf)+1, &timeout, &total, &stat);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_read_full()", !err);

	CPPUNIT_ASSERT(strlen(buf)+1 == total && !strcmp(buf,buf2) );

	edg_wll_gss_close(&conn, &timeout);
		
}

void GSSTest::bigecho()
{
	edg_wll_GssConnection	conn;
	edg_wll_GssStatus	stat;
	size_t			total;
	int			err;
	char 			buf[7*BUFSIZ];
	char			buf2[7*BUFSIZ];	

	std::cerr << "bigecho " << getpid() << std::endl;

	err = edg_wll_gss_connect(my_cred, "localhost", port, &timeout, &conn, &stat);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_connect()", !err);
	
	err = edg_wll_gss_write(&conn, buf, sizeof buf, &timeout, &stat);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_write()", !err);
	
	err = edg_wll_gss_read_full(&conn, buf2, sizeof buf2, &timeout, &total, &stat);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_read_full()", !err);

	CPPUNIT_ASSERT(sizeof buf == total && !memcmp(buf,buf2,sizeof buf) );

	edg_wll_gss_close(&conn, &timeout);
		
}


void GSSTest::errorTest()
{
	edg_wll_GssConnection	conn;
	edg_wll_GssStatus	stat;
	int			err;
	char *			msg = NULL;


	err = edg_wll_gss_connect(my_cred, "xxx.example.net", port, &timeout, &conn, &stat);
	if (err) edg_wll_gss_get_error(&stat, "gss_connect()", &msg);
	CPPUNIT_ASSERT_MESSAGE("edg_wll_gss_get_error()", msg);
}


CPPUNIT_TEST_SUITE_REGISTRATION( GSSTest );

int main (int ac,const char *av[])
{
	assert(ac == 2);
	std::ofstream	xml(av[1]);

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
