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


#ifndef GLITE_LB_LBS_JOBSTAT_H
#define GLITE_LB_LBS_JOBSTAT_H

#include "glite/lb/jobstat.h"
#include "glite/lb/intjobstat.h"
#include "glite/lb/intjobstat_supp.h"
#include "glite/lbu/db.h"

#define HISTORY_EMPTY "[]"
#define HISTORY_HEADER "[\n"
#define HISTORY_HEADER_SIZE 2
#define HISTORY_FOOTER "\n]"
#define HISTORY_FOOTER_SIZE 2
#define HISTORY_SEPARATOR ",\n"
#define HISTORY_SEPARATOR_SIZE 2

int edg_wll_AreThereZombies(edg_wll_Context ctx, const edg_wll_QueryRec **conditions);
int edg_wll_JobStatusServer(edg_wll_Context, glite_jobid_const_t, int, edg_wll_JobStat *);


int edg_wll_intJobStatus( edg_wll_Context, glite_jobid_const_t, int, intJobStat *, int, int);
edg_wll_ErrorCode edg_wll_StoreIntState(edg_wll_Context, intJobStat *, int);
edg_wll_ErrorCode edg_wll_StoreIntStateEmbryonic(edg_wll_Context, char *icnames, char *values);
edg_wll_ErrorCode edg_wll_LoadIntState(edg_wll_Context , glite_jobid_const_t , int, int, intJobStat **);
edg_wll_ErrorCode edg_wll_RestoreSubjobState(edg_wll_Context , glite_jobid_const_t , intJobStat *);


/* update stored job state according to new event */
edg_wll_ErrorCode edg_wll_StepIntState(edg_wll_Context ctx, glite_jobid_const_t job, edg_wll_Event *e, int seq, edg_wll_JobStat *old_stat, edg_wll_JobStat *stat_out);

edg_wll_ErrorCode edg_wll_StepIntStateParent(edg_wll_Context,glite_jobid_const_t,edg_wll_Event *,int,intJobStat *, edg_wll_JobStat *old_stat, edg_wll_JobStat *);

/* create embriotic job state for DAGs' subjob */

edg_wll_ErrorCode edg_wll_StepIntStateEmbriotic(
	edg_wll_Context ctx,	/* INOUT */
        edg_wll_Event *e	/* IN */
);



intJobStat* dec_intJobStat(char *, char **);
char *enc_intJobStat(char *, intJobStat* );

void write2rgma_status(intJobStat *);
void write2rgma_chgstatus(intJobStat *, char *);
char* write2rgma_statline(intJobStat *);

int add_stringlist(char ***, const char *);

edg_wll_ErrorCode edg_wll_GetSubjobHistogram(edg_wll_Context, glite_jobid_const_t parent_jobid, int *hist);
edg_wll_ErrorCode edg_wll_StoreSubjobHistogram(edg_wll_Context, glite_jobid_const_t parent_jobid, intJobStat *ijs);

edg_wll_Event* fetch_history(edg_wll_Context ctx, edg_wll_JobStat *stat);
int collate_history(edg_wll_Context ctx, edg_wll_JobStat *stat, edg_wll_Event* events, int authz_flags);
//int clear_history();
edg_wll_ErrorCode edg_wll_getConnectedJobs(edg_wll_Context ctx, glite_jobid_const_t job, edg_wll_JobConnectionType connection, edg_wll_RelationshipRecord **connections);

#endif /* GLITE_LB_LBS_JOBSTAT_H*/
