/* $Header$ */

/*
 * Internal representation of job state
 * (includes edg_wll_JobStat API structure)
 */

#define INTSTAT_VERSION "release-2.0"

typedef struct _sr_container {		// shallow resubmission container
	int	branch;
	char	*destination;
	char	*ce_node;
	char	*jdl;
} sr_container;


typedef struct _intJobStat {
		edg_wll_JobStat	pub;
		int		resubmit_type;
		char		*last_seqcode;
		char		*last_cancel_seqcode;
		char		*branch_tag_seqcode;		
		char		*last_branch_seqcode;
		char		*deep_resubmit_seqcode;
		sr_container	*branch_states;		// branch zero terminated array

/*		int		expect_mask; */
	} intJobStat;

void destroy_intJobStat(intJobStat *);

edg_wll_ErrorCode edg_wll_IColumnsSQLPart(edg_wll_Context, void *, intJobStat *, int , char **, char **);
edg_wll_ErrorCode edg_wll_RefreshIColumns(edg_wll_Context, void *);
int edg_wll_intJobStatus( edg_wll_Context, const edg_wlc_JobId, int, intJobStat *, int);

intJobStat* dec_intJobStat(char *, char **);
char *enc_intJobStat(char *, intJobStat* );

void write2rgma_status(edg_wll_JobStat *);

int before_deep_resubmission(const char *, const char *);
int same_branch(const char *, const char *);
int component_seqcode(const char *a, edg_wll_Source index);
