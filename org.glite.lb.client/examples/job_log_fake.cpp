/* sample how to use fake testing library instead glite_lb_client */

#include <iostream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include "glite/lb/consumer_fake.h"

class JobLogFakeExample: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JobLogFakeExample);
  CPPUNIT_TEST(testWithFake);
  CPPUNIT_TEST_SUITE_END();

private:
  static int query_events_cb(edg_wll_Context context, edg_wll_Event **events) {
    return edg_wll_SetError(context, ENOENT, "Some error");
  }

public:

  void testWithFake(void) {
    edg_wll_Context ctx;
    edg_wlc_JobId job;
    edg_wll_Event *events;

    edg_wll_InitContext(&ctx);
    CPPUNIT_ASSERT(edg_wlc_JobIdParse("https://localhost:9000/someid", &job) == 0);

    CPPUNIT_ASSERT(edg_wll_JobLog(ctx, job, &events) == 0);
    freeEvents(events);

    edg_wll_RegisterTestQueryEvents(&query_events_cb);
    CPPUNIT_ASSERT(edg_wll_JobLog(ctx, job, &events) != 0);
    // no events disposed here (they are deallocated on error)

    edg_wll_UnregisterTestQueryEvents();
    edg_wlc_JobIdFree(job);
    edg_wll_FreeContext(ctx);
  }

private:

  /* free returned events
   */
  void freeEvents(edg_wll_Event *events) {
    int i;

    i = 0;
    while (events[i].type != EDG_WLL_EVENT_UNDEF) {
      edg_wll_FreeEvent(&events[i]);
      i++;
    }
    free(events);
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION(JobLogFakeExample);

int main(void) {
  CppUnit::Test *suite;
  CppUnit::TextUi::TestRunner runner;

  suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
  runner.addTest(suite);

  return runner.run() ? 0 : 1;
}
