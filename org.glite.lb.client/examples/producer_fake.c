/* 
 * fake implementation of the producer API
 */

#include "producer.h"
#include "glite/lb/context-int.h"
#include "producer_fake.h"

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
