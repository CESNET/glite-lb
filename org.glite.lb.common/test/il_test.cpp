#include <assert.h>

#include <fstream>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
// #include <cppunit/ui/text/TextTestRunner.h>

#include <cppunit/XmlOutputter.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>

int main(int argc, char *argv[])
{
	CppUnit::Test  *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

#if 0
	CppUnit::TextTestRunner runner;

	runner.addTest(suite);
	runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(&runner.result(), std::cerr));

	return runner.run() ? 0 : 1;
#endif

	assert(argc == 2);

	std::ofstream	xml(argv[1]);
	
	CppUnit::TestResult controller;
	CppUnit::TestResultCollector result;
	controller.addListener( &result );

	CppUnit::TestRunner	runner;
	runner.addTest(suite);
	runner.run(controller);

	CppUnit::XmlOutputter xout( &result, xml );
	CppUnit::CompilerOutputter tout( &result, std::cout);
	xout.write();
	tout.write();

	return result.wasSuccessful() ? 0 : 1 ;
}
