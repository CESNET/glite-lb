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

#ifndef GLITE_LB_LB_CALLS_H
#define GLITE_LB_LB_CALLS_H

#ident "$Header:"

#define DB_PROXY_JOB    1
#define DB_SERVER_JOB   2

int edg_wll_jobMembership(edg_wll_Context ctx, glite_jobid_const_t job);

#define edg_wll_LockJobRowInShareMode(X,Y) edg_wll_LockJobRow(X,Y,0)
#define edg_wll_LockJobRowForUpdate(X,Y) edg_wll_LockJobRow(X,Y,1)
int edg_wll_LockJobRow(edg_wll_Context ctx, const char *job, int lock_mode);


#endif /* GLITE_LB_LB_CALLS_H */
