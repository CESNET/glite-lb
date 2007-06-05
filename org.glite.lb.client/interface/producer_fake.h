#ifndef __GLITE_LB_PRODUCER_FAKE_H__
#define __GLITE_LB_PRODUCER_FAKE_H

/* 
 * fake implementation of the producer API
 */

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

#endif /* __GLITE_LB_PRODUCER_FAKE_H__ */
