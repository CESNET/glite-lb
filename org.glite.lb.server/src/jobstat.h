/* $Header$ */

/*
 * Internal representation of job state
 * (includes edg_wll_JobStat API structure)
 */

#define INTSTAT_VERSION "release-3.0_shallow"


// Internal error codes 

#define RET_FAIL        0
#define RET_OK          1
#define RET_FATAL       RET_FAIL
#define RET_SOON        2
#define RET_LATE        3
#define RET_BADSEQ      4
#define RET_SUSPECT     5
#define RET_IGNORE      6
#define RET_BADBRANCH   7
#define RET_GOODBRANCH  8
#define RET_TOOOLD      9
#define RET_INTERNAL    100


// shallow resubmission container - holds state of each branch
// (useful when state restore is needed after ReallyRunning event)
//
typedef struct _branch_state {
	int	branch;
	char	*destination;
	char	*ce_node;
	char	*jdl;
} branch_state;


typedef struct _intJobStat {
		edg_wll_JobStat	pub;
		int		resubmit_type;
		char		*last_seqcode;
		char		*last_cancel_seqcode;
		char		*branch_tag_seqcode;		
		char		*last_branch_seqcode;
		char		*deep_resubmit_seqcode;
		branch_state	*branch_states;		// branch zero terminated array

/*		int		expect_mask; */
	} intJobStat;

void destroy_intJobStat(intJobStat *);
void destroy_intJobStat_extension(intJobStat *p);


edg_wll_ErrorCode edg_wll_IColumnsSQLPart(edg_wll_Context, void *, intJobStat *, int , char **, char **);
edg_wll_ErrorCode edg_wll_RefreshIColumns(edg_wll_Context, void *);
int edg_wll_intJobStatus( edg_wll_Context, const edg_wlc_JobId, int, intJobStat *, int);
edg_wll_ErrorCode edg_wll_StoreIntState(edg_wll_Context, intJobStat *, int);
edg_wll_ErrorCode edg_wll_StoreIntStateEmbryonic(edg_wll_Context, edg_wlc_JobId, const edg_wll_RegJobEvent *e);
edg_wll_ErrorCode edg_wll_LoadIntState(edg_wll_Context , edg_wlc_JobId , int, intJobStat **);


intJobStat* dec_intJobStat(char *, char **);
char *enc_intJobStat(char *, intJobStat* );

void write2rgma_status(edg_wll_JobStat *);
void write2rgma_chgstatus(edg_wll_JobStat *, char *);
char* write2rgma_statline(edg_wll_JobStat *);

int before_deep_resubmission(const char *, const char *);
int same_branch(const char *, const char *);
int component_seqcode(const char *a, edg_wll_Source index);
char * set_component_seqcode(char *s,edg_wll_Source index,int val);
int processEvent(intJobStat *, edg_wll_Event *, int, int, char **);

int add_stringlist(char ***, const char *);
int edg_wll_compare_seq(const char *, const char *);

void init_intJobStat(intJobStat *p);

