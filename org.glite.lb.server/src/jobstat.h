/* $Header$ */

/*
 * Internal representation of job state
 * (includes edg_wll_JobStat API structure)
 */

#define INTSTAT_VERSION "release-2.0"


typedef struct _intJobStat {
		edg_wll_JobStat	pub;
		int		wontresub;
		char		*last_seqcode;
		char		*last_cancel_seqcode;

/*		int		expect_mask; */
	} intJobStat;

void destroy_intJobStat(intJobStat *);

edg_wll_ErrorCode edg_wll_IColumnsSQLPart(edg_wll_Context, void *, intJobStat *, int , char **, char **);
edg_wll_ErrorCode edg_wll_RefreshIColumns(edg_wll_Context, void *);
int edg_wll_intJobStatus( edg_wll_Context, const edg_wlc_JobId, int, intJobStat *, int);

intJobStat* dec_intJobStat(char *, char **);
char *enc_intJobStat(char *, intJobStat* );

void write2rgma_status(edg_wll_JobStat *);

