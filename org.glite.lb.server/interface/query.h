#ifndef GLITE_LB_QUERY_H
#define GLITE_LB_QUERY_H

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


#include "glite/jobid/cjobid.h"
#include "glite/lb/context.h"
#include "glite/lb/events.h"
#include "glite/lb/jobstat.h"
#include "glite/lb/query_rec.h"

int convert_event_head(edg_wll_Context,char **,edg_wll_Event *);
int check_strict_jobid(edg_wll_Context, glite_jobid_const_t);
int match_status(edg_wll_Context, const edg_wll_JobStat *oldstat, const edg_wll_JobStat *stat,const edg_wll_QueryRec **conditions);
int check_job_query_index(edg_wll_Context, const edg_wll_QueryRec **);

#define NOTIF_ALL_JOBS	"all_jobs"

#endif /* GLITE_LB_QUERY_H */
