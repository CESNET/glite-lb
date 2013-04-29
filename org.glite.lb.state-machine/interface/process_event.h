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

#ifndef GLITE_LB_PROCESS_EVENT_H
#define GLITE_LB_PROCESS_EVENT_H

enum {
	GLITE_LB_EVENT_FILTER_STORE=0,
	GLITE_LB_EVENT_FILTER_REPLACE,
	GLITE_LB_EVENT_FILTER_DROP,
};

int filterEvent(intJobStat *js, edg_wll_Event *e, int ev_seq, edg_wll_Event *prev_ev, int prev_seq);
int processEvent(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring);
int add_stringlist(char ***lptr, const char *new_item);

#endif
