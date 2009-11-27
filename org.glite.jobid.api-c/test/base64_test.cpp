#include <assert.h>
#include <fstream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>

#include "strmd5.h"

class Base64Test: public  CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(Base64Test);
	CPPUNIT_TEST(test);
	CPPUNIT_TEST_SUITE_END();
public:
	void test();
};

void Base64Test::test()
{	
	int	i;
	unsigned char	in[2000], b[4000], out[2000];

	srandom(0xDEAD);
	in[0] = 'x';
	for (i=1; i<2000; i++) {
		char	s[20];
		int	len;
		sprintf(s,"%d",i);
		in[i] = random() % 256;

		std::cerr << '.';

		base64_encode(in,i,(char *) b,sizeof b);
		len = base64_decode((const char *) b,(char *) out,sizeof out);

		CPPUNIT_ASSERT_MESSAGE(std::string("len"),i == len);
		CPPUNIT_ASSERT_MESSAGE(std::string(s),!memcmp(in,out,i));
	}
	std::cerr << std::endl;
}

CPPUNIT_TEST_SUITE_REGISTRATION(Base64Test);


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
