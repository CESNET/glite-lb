#ifndef GLITE_LB_QUERY_REC_H
#define GLITE_LB_QUERY_REC_H

/*!
 * \file consumer.h
 * \brief L&B consumer API
 */

#ident "$Header$"

#include <glite/jobid/cjobid.h>

#ifdef BUILDING_LB_COMMON
#include "context.h"
#include "events.h"
#include "jobstat.h"
#else
#include "glite/lb/context.h"
#include "glite/lb/events.h"
#include "glite/lb/jobstat.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup Structures for Server querying
 * \brief Structures for Server querying
 * 
 *@{
 */

/**
 * Predefined types for query attributes
 */
typedef enum _edg_wll_QueryAttr{
	EDG_WLL_QUERY_ATTR_UNDEF=0,	/**< Not-defined value, used to terminate lists etc. */
	EDG_WLL_QUERY_ATTR_JOBID,	/**< Job Id \see _edg_wll_QueryRec */
	EDG_WLL_QUERY_ATTR_OWNER,	/**< Job owner \see _edg_wll_QueryRec */
	EDG_WLL_QUERY_ATTR_STATUS,	/**< Current job status */
	EDG_WLL_QUERY_ATTR_LOCATION,	/**< Where is the job processed */
	EDG_WLL_QUERY_ATTR_DESTINATION,	/**< Destination CE */
	EDG_WLL_QUERY_ATTR_DONECODE,	/**< Minor done status (OK,fail,cancel) */
	EDG_WLL_QUERY_ATTR_USERTAG,	/**< User tag */
	EDG_WLL_QUERY_ATTR_TIME,	/**< Timestamp \see _edg_wll_QueryRec */
	EDG_WLL_QUERY_ATTR_LEVEL,	/**< Logging level (see "dglog.h") * \see _edg_wll_QueryRec */
	EDG_WLL_QUERY_ATTR_HOST,	/**< Where the event was generated */
	EDG_WLL_QUERY_ATTR_SOURCE,	/**< Source component */
	EDG_WLL_QUERY_ATTR_INSTANCE,	/**< Instance of the source component */
	EDG_WLL_QUERY_ATTR_EVENT_TYPE,	/**< Event type \see _edg_wll_QueryRec */
	EDG_WLL_QUERY_ATTR_CHKPT_TAG,	/**< Checkpoint tag */
	EDG_WLL_QUERY_ATTR_RESUBMITTED,	/**< Job was resubmitted */
	EDG_WLL_QUERY_ATTR_PARENT,	/**< Job was resubmitted */
	EDG_WLL_QUERY_ATTR_EXITCODE,	/**< Unix exit code */
	EDG_WLL_QUERY_ATTR_JDL_ATTR,	/**< Arbitrary JDL attribute */
	EDG_WLL_QUERY_ATTR_STATEENTERTIME,	/**< When entered current status */
	EDG_WLL_QUERY_ATTR_LASTUPDATETIME,	/**< Time of the last known event of the job */
	EDG_WLL_QUERY_ATTR_NETWORK_SERVER,	/**< Network server aka RB aka WMproxy endpoint */
	EDG_WLL_QUERY_ATTR__LAST
/*	if adding new attribute, add conversion string to common/xml_conversions.c too !! */
} edg_wll_QueryAttr;


/**
 * Predefined types for query operands
 */
typedef enum _edg_wll_QueryOp{
	EDG_WLL_QUERY_OP_EQUAL,		/**< attribute is equal to the operand value \see _edg_wll_QueryRec */
	EDG_WLL_QUERY_OP_LESS,		/**< attribute is grater than the operand value \see _edg_wll_QueryRec */
	EDG_WLL_QUERY_OP_GREATER,	/**< attribute is less than the operand value \see _edg_wll_QueryRec */
	EDG_WLL_QUERY_OP_WITHIN,	/**< attribute is in given interval \see _edg_wll_QueryRec */
	EDG_WLL_QUERY_OP_UNEQUAL,	/**< attribute is not equal to the operand value \see _edg_wll_QueryRec */
} edg_wll_QueryOp;


/**
 * Single query condition for edg_wll_Query().
 * Those records are composed to form an SQL \a where clause
 * when processed at the L&B server
 */
typedef struct _edg_wll_QueryRec {
	edg_wll_QueryAttr	attr;	/**< attribute to query */
	edg_wll_QueryOp		op;	/**< query operation */

/**
 * Specification of attribute to query
 */
	union {
		char *			tag;	/**< user tag name / JDL attribute "path"*/
		edg_wll_JobStatCode	state;	/**< job status code */	
	} attr_id;
/**
 * Query operand.
 * The appropriate type is uniquely defined by the attr member
 */
	union edg_wll_QueryVal {
		int	i;	/**< integer query attribute value */
		char	*c;	/**< character query attribute value */
		struct timeval	t;	/**< time query attribute value */
		glite_jobid_t	j;	/**< JobId query attribute value */
	} value, value2;
} edg_wll_QueryRec;

/*
 * edg_wll_QueryRec manipulation
 */

/** Free edg_wll_QueryRec internals, not the structure itself */
void edg_wll_QueryRecFree(edg_wll_QueryRec *);

/*
 *@} end of group
 */

/**
 * \defgroup Structures for Server purge, dump and load
 * \brief Structures for Server  purge, dump and load
 * 
 *@{
 */

/** Purge request */
typedef struct _edg_wll_PurgeRequest {
	char	**jobs;		/**< list of jobid's to work on */ 

/** Purge jobs that are in the given states and "untouched" at least for the
  * specified interval.
  * Currently applicable for CLEARED, ABORTED, CANCELLED and OTHER (catchall).
  * The other array members are for future extensions.
  * Negative values stand for server defaults.
  */
	time_t	timeout[EDG_WLL_NUMBER_OF_STATCODES];
#define EDG_WLL_PURGE_JOBSTAT_OTHER	EDG_WLL_JOB_UNDEF

/**
 * Actions to be taken and information required.
 */
	int	flags;

/** no dry run */
#define EDG_WLL_PURGE_REALLY_PURGE	1
/** return list of jobid matching the purge/dump criteria */
#define EDG_WLL_PURGE_LIST_JOBS		2
/** dump to a file on the sever */
#define EDG_WLL_PURGE_SERVER_DUMP	4
/** TODO: stream the dump info to the client */
#define EDG_WLL_PURGE_CLIENT_DUMP	8
/* ! when addning new constant, add it also to common/xml_conversions.c ! */

/** Desired purge estimated time.
 */
	time_t target_runtime;
	
/** private request processing data (for the reentrant functions) */
/* TODO */
	
} edg_wll_PurgeRequest;

/** Output data of a purge */
typedef struct _edg_wll_PurgeResult {
	char	*server_file;	/**< filename of the dump at the server */
	char	**jobs;		/**< affected jobs */
/* TODO: output of the streaming interface */
} edg_wll_PurgeResult;


#define EDG_WLL_DUMP_NOW	-1
#define EDG_WLL_DUMP_LAST_START	-2
#define EDG_WLL_DUMP_LAST_END	-3
/*      if adding new attribute, add conversion string to common/xml_conversions.c too !! */

/** Purge request */
typedef struct {
	time_t	from,to;
} edg_wll_DumpRequest;

/** Output data of a dump */
typedef struct {
	char	*server_file;
	time_t	from,to;
} edg_wll_DumpResult;


/** Load request */
typedef struct {
	char	*server_file;
} edg_wll_LoadRequest;

/** Output data of a load */
typedef struct {
	char	*server_file;
	time_t	from,to;
} edg_wll_LoadResult;

/*
 *@} end of group
 */

#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_QUERY_REC_H */
