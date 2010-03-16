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
