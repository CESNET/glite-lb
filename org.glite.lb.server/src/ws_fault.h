#ifndef __EDG_WORKLOAD_LOGGING_LBSERVER_WS_FAULT_H__
#define __EDG_WORKLOAD_LOGGING_LBSERVER_WS_FAULT_H__

extern void edg_wll_ErrToFault(const edg_wll_Context, struct soap *);
extern void edg_wll_FaultToErr(const struct soap *, edg_wll_Context);

#endif /* __EDG_WORKLOAD_LOGGING_LBSERVER_WS_FAULT_H__ */
