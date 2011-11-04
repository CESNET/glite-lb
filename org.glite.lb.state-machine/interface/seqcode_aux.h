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


int component_seqcode(const char *a, edg_wll_Source index);

char * set_component_seqcode(char *a,edg_wll_Source index,int val);

int before_deep_resubmission(const char *a, const char *b);

int same_branch(const char *a, const char *b) ;

int edg_wll_compare_pbs_seq(const char *a,const char *b);

int edg_wll_compare_condor_seq(const char *a,const char *b);

edg_wll_CondorEventSource get_condor_event_source(const char *condor_seq_num) ;

int edg_wll_compare_seq(const char *a, const char *b);

int compare_events_by_seq(const void *a, const void *b);

int add_taglist(const char *new_item, const char *new_item2, const char *seq_code, intJobStat *js);
