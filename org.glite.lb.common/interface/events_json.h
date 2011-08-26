#ifndef GLITE_LB_EVENTS_JSON_H
#define GLITE_LB_EVENTS_JSON_H

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


#ifdef BUILDING_LB_COMMON
#include "events.h"
#else
#include "glite/lb/events.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Generate a special Notification ULM line from edg_wll_Event structure
 * \param context IN: context to work with
 * \param event IN: event to unparse
 */
extern int edg_wll_UnparseEventJSON(
	edg_wll_Context	context,
	edg_wll_Event *	event,
	char **line
);


#ifdef __cplusplus
}
#endif


#endif /* GLITE_LB_EVENTS_JSON_H */
