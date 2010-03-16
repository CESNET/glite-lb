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

/* 
 * fake implementation of the producer API
 */

#include "glite/lb/producer.h"
#include "glite/lb/context-int.h"
#include "glite/lb/producer_fake.h"

#define FAKE_VERSION 1

#include "../src/producer.c"


static edg_wll_Logging_cb_f *Logging_cb = NULL;
static edg_wll_Logging_cb_f *LoggingProxy_cb = NULL;


/* register the logging callback */
int edg_wll_RegisterTestLogging(edg_wll_Logging_cb_f *cb) {
  if (Logging_cb) return 0;

  Logging_cb = cb;
  return 1;
}


/* register the proxy logging callback */
int edg_wll_RegisterTestLoggingProxy(edg_wll_Logging_cb_f *cb) {
  if (LoggingProxy_cb) return 0;

  LoggingProxy_cb = cb;
  return 1;
}


/* unregister the logging callback */
void edg_wll_UnregisterTestLogging() {
  Logging_cb = NULL;
}


/* unregister the proxy logging callback */
void edg_wll_UnregisterTestLoggingProxy() {
  LoggingProxy_cb = NULL;
}


/* "fake" implementation of function sending formated UML string */
int edg_wll_DoLogEvent(edg_wll_Context context, edg_wll_LogLine logLine) {
  if (Logging_cb)
    return Logging_cb(context);
  else
    return edg_wll_Error(context, NULL, NULL);
}


/* "fake" implementation of function sending formated ULM string */
int edg_wll_DoLogEventProxy(edg_wll_Context context, edg_wll_LogLine logline) {
  if (Logging_cb)
    return Logging_cb(context);
  else
    return edg_wll_Error(context, NULL, NULL);
}
