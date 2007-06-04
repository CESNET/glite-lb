#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_CONSUMER_H__
#define __EDG_WORKLOAD_LOGGING_CLIENT_CONSUMER_H__

/*!
 * \file consumer.h
 * \brief L&B consumer API
 */

#ident "$Header$"

#include "glite/wmsutils/jobid/cjobid.h"
#include "context.h"
#include "events.h"
#include "jobstat.h"

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
		char *			tag;	/**< user tag name */
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
		edg_wlc_JobId	j;	/**< JobId query attribute value */
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

#ifdef __cplusplus
}
#endif

#endif /* __EDG_WORKLOAD_LOGGING_CLIENT_CONSUMER_H__ */
