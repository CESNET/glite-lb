#ifndef GLITE_LB_WS_FAULT_H
#define GLITE_LB_WS_FAULT_H

#ident "$Header$"

#include "glite/lb/context.h"

extern void edg_wll_ErrToFault(const edg_wll_Context, struct soap *);
extern void edg_wll_FaultToErr(const struct soap *, edg_wll_Context);

#endif /* GLITE_LB_WS_FAULT_H */
