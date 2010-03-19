#ifndef GLITE_LB_SERVER_NOTIFICATION_H
#define GLITE_LB_SERVER_NOTIFICATION_H

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

int edg_wll_NotifNewServer(edg_wll_Context,
				edg_wll_QueryRec const * const *, int flags, char const *,
				const edg_wll_NotifId, time_t *);
int edg_wll_NotifBindServer(edg_wll_Context,
				const edg_wll_NotifId, const char *, time_t *);
int edg_wll_NotifChangeServer(edg_wll_Context,
				const edg_wll_NotifId, edg_wll_QueryRec const * const *,
				edg_wll_NotifChangeOp);
int edg_wll_NotifRefreshServer(edg_wll_Context,
				const edg_wll_NotifId, time_t *);
int edg_wll_NotifDropServer(edg_wll_Context, edg_wll_NotifId);

#endif
