#ifndef GLITE_LB_LB_CALLS_H
#define GLITE_LB_LB_CALLS_H

#ident "$Header:"

#define DB_PROXY_JOB    1
#define DB_SERVER_JOB   2

int edg_wll_jobMembership(edg_wll_Context ctx, glite_jobid_const_t job);

#define edg_wll_LockJobRowInShareMode(X,Y) edg_wll_LockJobRow(X,Y,0)
#define edg_wll_LockJobRowForUpdate(X,Y) edg_wll_LockJobRow(X,Y,1)
int edg_wll_LockJobRow(edg_wll_Context ctx, glite_jobid_const_t job, int lock_mode);


#endif /* GLITE_LB_LB_CALLS_H */
