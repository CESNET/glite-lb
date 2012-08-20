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

#ifndef GLITE_LB_TEXT_H
#define GLITE_LB_TEXT_H

#include "glite/lb/context.h"
#include "glite/lb/events.h"
#include "glite/lb/jobstat.h"
#include "lb_proto.h"

int edg_wll_QueryToText(edg_wll_Context,edg_wll_Event *,char **);
int edg_wll_UserNotifsToText(edg_wll_Context ctx, char **notifids, char **message);
char *edg_wll_ErrorToText(edg_wll_Context,int);

#endif /* GLITE_LB_TEXT */
