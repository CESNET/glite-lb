/* 
 * fake implementation of the consumer API 
 */

#ifndef WORKLOAD_LOGGING_CLIENT_CONSUMER_FAKE_H
#define WORKLOAD_LOGGING_CLIENT_CONSUMER_FAKE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "glite/lb/context-int.h"

typedef int (edg_wll_QueryEvents_cb_f)(edg_wll_Context context, edg_wll_Event **events);
typedef int (edg_wll_QueryListener_cb_f)(edg_wll_Context context, char **host, uint16_t *port);

int edg_wll_RegisterTestQueryEvents(edg_wll_QueryEvents_cb_f *cb);
int edg_wll_RegisterTestQueryListener(edg_wll_QueryListener_cb_f *cb);
void edg_wll_UnregisterTestQueryEvents();
void edg_wll_UnregisterTestQueryListener();

#ifdef __cplusplus
}
#endif

#endif /* WORKLOAD_LOGGING_CLIENT_CONSUMER_FAKE_H */
