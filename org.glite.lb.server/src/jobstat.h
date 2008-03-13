#ifndef GLITE_LB_LBS_JOBSTAT_H
#define GLITE_LB_LBS_JOBSTAT_H

#ident "$Header$"

#include "glite/lb/jobstat.h"
#include "glite/lb/intjobstat.h"

int edg_wll_JobStatusServer(edg_wll_Context, glite_jobid_const_t, int, edg_wll_JobStat *);


int edg_wll_intJobStatus( edg_wll_Context, glite_jobid_const_t, int, intJobStat *, int);
edg_wll_ErrorCode edg_wll_StoreIntState(edg_wll_Context, intJobStat *, int);
edg_wll_ErrorCode edg_wll_StoreIntStateEmbryonic(edg_wll_Context, edg_wlc_JobId, char *icnames, char *values, glite_lbu_bufInsert *bi);
edg_wll_ErrorCode edg_wll_LoadIntState(edg_wll_Context , edg_wlc_JobId , int, int, intJobStat **);

edg_wll_ErrorCode edg_wll_StepIntState(edg_wll_Context ctx, edg_wlc_JobId job, edg_wll_Event *e, int seq, edg_wll_JobStat *stat_out);
edg_wll_ErrorCode edg_wll_StepIntStateParent(edg_wll_Context,edg_wlc_JobId,edg_wll_Event *,int,intJobStat *,edg_wll_JobStat *);



intJobStat* dec_intJobStat(char *, char **);
char *enc_intJobStat(char *, intJobStat* );

void write2rgma_status(edg_wll_JobStat *);
void write2rgma_chgstatus(edg_wll_JobStat *, char *);
char* write2rgma_statline(edg_wll_JobStat *);

int add_stringlist(char ***, const char *);

edg_wll_ErrorCode edg_wll_GetSubjobHistogram(edg_wll_Context, edg_wlc_JobId parent_jobid, int *hist);
edg_wll_ErrorCode edg_wll_StoreSubjobHistogram(edg_wll_Context, edg_wlc_JobId parent_jobid, intJobStat *ijs);

#endif /* GLITE_LB_LBS_JOBSTAT_H*/
