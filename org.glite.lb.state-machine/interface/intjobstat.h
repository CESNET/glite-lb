#ifndef GLITE_LB_INTJOBSTAT_H
#define GLITE_LB_INTJOBSTAT_H

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


#include "glite/lb/jobstat.h"

/*
 * Internal representation of job state
 * (includes edg_wll_JobStat API structure)
 */

/* convention: revision Y.XX - DESCRIPTION 			*/
/* where Z.XX is version from indent + 1 (version after commit), Y = Z+1 */
/* and DESCRIPTION is short hit why version changed		*/

#define INTSTAT_VERSION "revision 2.11 - payload owner"
//                      ".... MAX LENGTH 32 BYTES !! ...."

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
#define RET_UNREG	10
#define RET_INTERNAL    100


// shallow resubmission container - holds state of each branch
// (useful when state restore is needed after ReallyRunning event)
//
typedef struct _branch_state {
	int	branch;
	char	*destination;
	char	*ce_node;
	char	*jdl;
	/*!! if adding new field, modify also free_branch_state() */
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

		struct timeval	last_pbs_event_timestamp;
		int		pbs_reruning;		// true if rerun event arrived
		char		**tag_seq_codes;
		char 		*payload_owner_pending;
		char 		*payload_owner_unconfirmed;

		/*!! if adding new field, modify also destroy_intJobStat_extension() 	*
		 *!! update dec/enc_intJobStat and increase INTSTAT_VERSION		*/
	} intJobStat;

typedef enum _edg_wll_PBSEventSource {
	EDG_WLL_PBS_EVENT_SOURCE_UNDEF = 0,
	EDG_WLL_PBS_EVENT_SOURCE_SCHEDULER,
	EDG_WLL_PBS_EVENT_SOURCE_SERVER,
	EDG_WLL_PBS_EVENT_SOURCE_MOM,
	EDG_WLL_PBS_EVENT_SOURCE_ACCOUNTING,
	EDG_WLL_PBS_EVENT_SOURCE__LAST
} edg_wll_PBSEventSource;

typedef enum _edg_wll_CondorEventSource {
	EDG_WLL_CONDOR_EVENT_SOURCE_UNDEF = 0,
	EDG_WLL_CONDOR_EVENT_SOURCE_COLLECTOR,
	EDG_WLL_CONDOR_EVENT_SOURCE_MASTER,
	EDG_WLL_CONDOR_EVENT_SOURCE_MATCH,
	EDG_WLL_CONDOR_EVENT_SOURCE_NEGOTIATOR,
	EDG_WLL_CONDOR_EVENT_SOURCE_SCHED,
	EDG_WLL_CONDOR_EVENT_SOURCE_SHADOW,
	EDG_WLL_CONDOR_EVENT_SOURCE_STARTER,
	EDG_WLL_CONDOR_EVENT_SOURCE_START,
	EDG_WLL_CONDOR_EVENT_SOURCE_JOBQUEUE,
	EDG_WLL_CONDOR_EVENT_SOURCE__LAST
} edg_wll_CondorEventSource;

typedef enum _subjobClassCodes {
	SUBJOB_CLASS_UNDEF = 0,
	SUBJOB_CLASS_RUNNING,
	SUBJOB_CLASS_DONE,
	SUBJOB_CLASS_ABORTED,
	SUBJOB_CLASS_CLEARED,
	SUBJOB_CLASS_REST
} subjobClassCodes;

void destroy_intJobStat(intJobStat *);
void destroy_intJobStat_extension(intJobStat *p);


void init_intJobStat(intJobStat *p);


#endif /* GLITE_LB_INTJOBSTAT_H */
