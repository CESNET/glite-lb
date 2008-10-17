#ident "$Header$"

int component_seqcode(const char *a, edg_wll_Source index);

char * set_component_seqcode(char *a,edg_wll_Source index,int val);

int before_deep_resubmission(const char *a, const char *b);

int same_branch(const char *a, const char *b) ;

int edg_wll_compare_pbs_seq(const char *a,const char *b);
#define edg_wll_compare_condor_seq edg_wll_compare_pbs_seq

edg_wll_PBSEventSource get_pbs_event_source(const char *pbs_seq_num) ;

edg_wll_CondorEventSource get_condor_event_source(const char *condor_seq_num) ;

int edg_wll_compare_seq(const char *a, const char *b);

int compare_events_by_seq(const void *a, const void *b);

int add_taglist(const char *new_item, const char *new_item2, const char *seq_code, intJobStat *js);
