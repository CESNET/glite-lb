#ifndef GLITE_LB_WS_FAULT_H
#define GLITE_LB_WS_FAULT_H

#ident "$Header$"
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


#include "glite/lb/context.h"

extern void edg_wll_ErrToFault(const edg_wll_Context, struct soap *);
extern void edg_wll_FaultToErr(const struct soap *, edg_wll_Context);

#endif /* GLITE_LB_WS_FAULT_H */
