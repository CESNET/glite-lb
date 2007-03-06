#include <cppunit/extensions/HelperMacros.h>

#include "PluginManager.H"

class TestPlugin : public PluginManager::Plugin {
public:
	bool inited, cleaned;

	virtual bool initialize() {
		inited = true;
	}

	virtual bool cleanup() {
		cleaned = true;
	}

	static TestPlugin theTestPlugin;

private:
	TestPlugin() : PluginManager::Plugin("test plugin"),
		       inited(false),
		       cleaned(false) 
		{}



};


class PluginManagerTest : public CppUnit::TestFixture 
{
	CPPUNIT_TEST_SUITE(PluginManagerTest);
	CPPUNIT_TEST(testInit);
	CPPUNIT_TEST(testClean);
	CPPUNIT_TEST_SUITE_END();
	
public:
	void setUp() {
	}

	void tearDown() {
	}

	void testInit() {
		PluginManager::thePluginManager.initialize();
		CPPUNIT_ASSERT(TestPlugin::theTestPlugin.inited);
	}
	
	void testClean() {
		PluginManager::thePluginManager.cleanup();
		CPPUNIT_ASSERT(TestPlugin::theTestPlugin.cleaned);
	}
};


TestPlugin TestPlugin::theTestPlugin;

CPPUNIT_TEST_SUITE_REGISTRATION( PluginManagerTest );
