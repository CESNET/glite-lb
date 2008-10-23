#include <assert.h>
#include <fstream>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>

#include <glite/lb/connpool.h>

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

	edg_wll_poolFree();

	return result.wasSuccessful() ? 0 : 1 ;
}
