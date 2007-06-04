/* 
 * fake implementation of the producer API
 */

#ifndef WORKLOAD_LOGGING_CLIENT_PRODUCER_FAKE_H
#define WORKLOAD_LOGGING_CLIENT_PRODUCER_FAKE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "glite/lb/context.h"

typedef int (edg_wll_Logging_cb_f)(edg_wll_Context context);

int edg_wll_RegisterTestLogging(edg_wll_Logging_cb_f *cb);
int edg_wll_RegisterTestLoggingProxy(edg_wll_Logging_cb_f *cb);
void edg_wll_UnregisterTestLogging();
void edg_wll_UnregisterTestLoggingProxy();

#ifdef __cplusplus
}
#endif

#endif /* WORKLOAD_LOGGING_CLIENT_PRODUCER_FAKE_H */
