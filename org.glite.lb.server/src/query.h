int convert_event_head(edg_wll_Context,char **,edg_wll_Event *);
int check_strict_jobid(edg_wll_Context, const glite_lbu_JobId);
int match_status(edg_wll_Context, const edg_wll_JobStat *stat,const edg_wll_QueryRec **conditions);

#define NOTIF_ALL_JOBS	"all_jobs"
