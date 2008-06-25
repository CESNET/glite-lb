#ifndef GLITE_LB_PROCESS_EVENT_H
#define GLITE_LB_PROCESS_EVENT_H

int processEvent(intJobStat *js, edg_wll_Event *e, int ev_seq, int strict, char **errstring);
int add_stringlist(char ***lptr, const char *new_item);

#endif
