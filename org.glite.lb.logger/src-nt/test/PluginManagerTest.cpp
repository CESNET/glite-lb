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
		PluginManager::instance()->initialize();
		CPPUNIT_ASSERT(TestPlugin::theTestPlugin.inited);
	}
	
	void testClean() {
		PluginManager::instance()->cleanup();
		CPPUNIT_ASSERT(TestPlugin::theTestPlugin.cleaned);
	}
};


TestPlugin TestPlugin::theTestPlugin;

CPPUNIT_TEST_SUITE_REGISTRATION( PluginManagerTest );
