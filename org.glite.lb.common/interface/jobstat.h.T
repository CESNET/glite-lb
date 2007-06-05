#ifndef __GLITE_LB_JOBSTAT_H__
#define __GLITE_LB_JOBSTAT_H__

/*!
 * \file jobstat.h  
 * \brief edg_wll_JobStat definition and related stuff
 */

#ident "$Header$"
/* 
@@@AUTO
*/
@@@LANG: C

#include "glite/wmsutils/jobid/cjobid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup jobstatus Job Status
 * \brief Job status structure definition and related definitions.
 *@{
 */

/**
 * Miscelaneous job status numeric codes
 */

@@@{
	for my $n ($status->getAllFieldsOrdered) {
# XXX: we've got only _common_ in jobStatus, no clash possible
		my $f = selectField $status $n;
		if ($f->{codes}) {
			my $n = ucfirst getName $f;
			gen qq{
!enum edg_wll_Stat$n \{
};
			for (@{$f->{codes}}) {
				my $uc = uc $_->{name};
				gen qq{
!	EDG_WLL_STAT_$uc,		/**< $_->{comment} */
};
			}
			my $un = uc $n;
			gen qq{
!	EDG_WLL_NUMBER_OF_${un}S	/**< Number of ${n}s */
};
			gen qq{
!\};
};
		}
	}
@@@}


/*!
 *
 * Job status numeric code
 */

typedef enum _edg_wll_JobStatCode {
/** Indicates invalid edg_wll_JobStat structure */
	EDG_WLL_JOB_UNDEF = 0,
@@@{
	for my $stat ($status->getTypesOrdered) {
		my $u = uc $stat;
		my $c = getTypeComment $status $stat;
		gen qq{
!	EDG_WLL_JOB_$u,	/**< $c */
};
	}
@@@}
	EDG_WLL_NUMBER_OF_STATCODES /**< Number of meaningful status codes */
} edg_wll_JobStatCode;

/*!
 *
 * Pair tag = value.
 */
typedef struct _edg_wll_TagValue {
        char *  tag;    /**< User-specified information tag */
        char *  value;  /**< Value assigned to user-specified information tag */
} edg_wll_TagValue;


/*!
 *
 * Description of the job status.
 * Returned by the edg_wll_JobStatus() function
 */
typedef struct _edg_wll_JobStat {
	edg_wll_JobStatCode	state;	        /**< status code */
@@@{
	for my $n (getAllFieldsOrdered $status) {
		selectField $status $n;
		my $f = getField $status;
		my $type = getType $f;
		my $name = getName $f;
		my $fucname = ucfirst $name;
		$type = "enum edg_wll_Stat$fucname" if $f->{codes};
		my $comment = getComment $f;
		gen qq{
!	$type	$name;	/**< $comment */
};
	}
@@@}

} edg_wll_JobStat;

/**
 * \name edg_wll_JobStat manipulation
 */


/**
 * Bitmasks for edg_wll_JobStatus() flags argument.
 * Settings these flags causes the status calls to retrieve additional
 * information.
 */
#define EDG_WLL_STAT_CLASSADS	1	/**< various job description fields */
#define EDG_WLL_STAT_CHILDREN	2	/**< list of subjob JobId's */
#define EDG_WLL_STAT_CHILDSTAT	4	/**< apply the flags recursively to subjobs */
#define EDG_WLL_STAT_CHILDHIST_FAST 8		/**< partially complete histogram of child job states */
#define EDG_WLL_STAT_CHILDHIST_THOROUGH 16	/**< full and up-to date histogram of child job states */
/* starting from bit 10 private flags begins - do not add 1024 and more! */


/**
 * Initialize empty status structure.
 * Fills in the stucture with NULL's or values with no meaning
 */
 
extern int edg_wll_InitStatus(edg_wll_JobStat *);

/**
 * Initialize dest structure and copy source status to this destination
 */
 
extern edg_wll_JobStat *edg_wll_CpyStatus(const edg_wll_JobStat *,edg_wll_JobStat *);

/** 
 * Free status structure contents.
 * \warning The structure itself is not freed.
 */
extern void edg_wll_FreeStatus(edg_wll_JobStat *);

/**
 * Convert string job status to numeric code.
 */
extern edg_wll_JobStatCode edg_wll_StringToStat(const char *);

/** 
 * Convert numeric job status code to string representation
 */
extern char *edg_wll_StatToString(edg_wll_JobStatCode);

/*@}*/

/*
 *@} end of group
 */

#ifdef __cplusplus
}
#endif

#endif /* __GLITE_LB_JOBSTAT_H__ */
