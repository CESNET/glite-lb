#include <cppunit/extensions/HelperMacros.h>

#include "Singleton.H"

class one : public Singleton<one> {
	friend class Singleton<one>;
};

class two : public Singleton<two> {
	friend class Singleton<two>;
};

class SingletonTest: public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(SingletonTest);
	CPPUNIT_TEST(getInstance);
	CPPUNIT_TEST(pair);
	CPPUNIT_TEST(noCreate);
	CPPUNIT_TEST_SUITE_END();
public:
	
	void setUp() {
	}

	void tearDown() {
	}

	void getInstance() {
		one *p;
		one *q;

		p = one::instance();
		q = one::instance();
		CPPUNIT_ASSERT(p != NULL);
		CPPUNIT_ASSERT(q != NULL);
		CPPUNIT_ASSERT(p == q);
	}

	void pair() {
		one *p;
		two *q;

		p = one::instance();
		q = two::instance();
		CPPUNIT_ASSERT(p != NULL);
		CPPUNIT_ASSERT(q != NULL);
		CPPUNIT_ASSERT((void*)p != (void*)q);
	}

	void noCreate() {
		one *p = new one;

	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( SingletonTest );
