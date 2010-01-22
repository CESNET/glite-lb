#ifndef GLITE_LB_CONSUMER_H
#define GLITE_LB_CONSUMER_H

/*!
 * \file consumer.h
 * \brief L&B consumer API
 */

#ident "$Header$"

#include "glite/jobid/cjobid.h"
#include "glite/lb/context.h"
#include "glite/lb/events.h"
#include "glite/lb/jobstat.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "glite/lb/query_rec.h"

/**
 * \defgroup Functions for Server querying
 * \brief The core part of the LB querying API.
 * 
 * The functions in this part of the API are responsible for
 * transforming the user query to the LB protocol, contacting the server,
 * receiving back the response and transforming back the results to the
 * API data structures. 
 *
 * General rules:
 * - functions return 0 on success, nonzero on error, errror details can 
 *   be found via edg_wll_ErrorCode()
 * - OUT are ** types, functions malloc()-ate objects and fill in the pointer 
 *   pointed to by the OUT argument
 * - returned lists of pointers are NULL-terminated malloc()-ed arrays
 * - edg_wll_Query + wrapper terminate arrays with EDG_WLL_EVENT_UNDEF event
 * - OUT is NULL if the list is empty
 *@{
 */

/**
 * General query on events.
 * Return events satisfying all conditions 
 * query records represent conditions in the form 
 * \a attr \a op \a value eg. time > 87654321.
 * \see edg_wll_QueryRec
 *
 * \param[in] context 		context to work with
 * \param[in] job_conditions 	query conditions (ANDed) on current job status, null (i.e. ATTR_UNDEF) terminated list. NULL means empty list, i.e. always TRUE
 * \param[in] event_conditions 	conditions on events, null terminated list, NULL means empty list, i.e. always TRUE
 * \param[out] events 		list of matching events
 */
int edg_wll_QueryEvents(
	edg_wll_Context			context,
	const edg_wll_QueryRec *	job_conditions,
	const edg_wll_QueryRec *	event_conditions,
	edg_wll_Event **		events
);

/**
 * Extended event query interface.
 * Similar to \ref edg_wll_QueryEvents but the conditions are nested lists.
 * Elements of the inner lists have to refer to the same attribute and they
 * are logically ORed. 
 * The inner lists themselves are logically ANDed then.
 */

int edg_wll_QueryEventsExt(
	edg_wll_Context			context,
	const edg_wll_QueryRec **	job_conditions,
	const edg_wll_QueryRec **	event_conditions,
	edg_wll_Event **		events
);


/**
 * Query LBProxy and use plain communication
 * \warning edg_wll_*Proxy() functions are not implemented in release 1.
 */
int edg_wll_QueryEventsProxy(
	edg_wll_Context			context,
	const edg_wll_QueryRec *	job_conditions,
	const edg_wll_QueryRec *	event_conditions,
	edg_wll_Event **		events
);

/**
 * \warning edg_wll_*Proxy() functions are not implemented in release 1.
 */

int edg_wll_QueryEventsExtProxy(
	edg_wll_Context			context,
	const edg_wll_QueryRec **	job_conditions,
	const edg_wll_QueryRec **	event_conditions,
	edg_wll_Event **		events
);

/** 
 * General query on jobs.
 * Return jobs (and possibly their states) for which an event satisfying the conditions
 * exists.
 * \see edg_wll_QueryEvents
 * \param[in] context 		context to work with
 * \param[in] conditions 	query records (ANDed), null (i.e. EDG_WLL_ATTR_UNDEF) terminated list
 * \param[in] flags 		additional status fields to retrieve (\see edg_wll_JobStatus)
 * \param[out] jobs 		list of job ids. May be NULL.
 * \param[out] states 		list of corresponding states (returned only if not NULL)
 */
int edg_wll_QueryJobs(
	edg_wll_Context			context,
	const edg_wll_QueryRec *	conditions,
	int				flags,
	glite_jobid_t **		jobs,
	edg_wll_JobStat **		states
);

/**
 * Extended job query interface.
 * Similar to \ref edg_wll_QueryJobs but the conditions are nested lists.
 * Elements of the inner lists have to refer to the same attribute and they
 * are logically ORed. 
 * The inner lists themselves are logically ANDed then.
 */


int edg_wll_QueryJobsExt(
	edg_wll_Context			context,
	const edg_wll_QueryRec **	conditions,
	int				flags,
	glite_jobid_t **		jobs,
	edg_wll_JobStat **		states
);


/**
 * Query LBProxy and use plain communication
 * \warning edg_wll_*Proxy() functions are not implemented in release 1.
 */
int edg_wll_QueryJobsProxy(
	edg_wll_Context			context,
	const edg_wll_QueryRec *	conditions,
	int				flags,
	glite_jobid_t **		jobs,
	edg_wll_JobStat **		states
);

/**
 * \warning edg_wll_*Proxy() functions are not implemented in release 1.
 */

int edg_wll_QueryJobsExtProxy(
	edg_wll_Context			context,
	const edg_wll_QueryRec **	conditions,
	int				flags,
	glite_jobid_t **		jobs,
	edg_wll_JobStat **		states
);

/** Return status of a single job.
 * \param[in] context 		context to operate on
 * \param[in] jobid 		query this job
 * \param[in] flags 		specifies optional status fields to retrieve,
 * 	\see EDG_WLL_STAT_CLASSADS, EDG_WLL_STAT_CHILDREN, EDG_WLL_STAT_CHILDSTAT
 * \param[out] status		status
 */

int edg_wll_JobStatus(
	edg_wll_Context		context,
	glite_jobid_const_t		jobid,
	int			flags,
	edg_wll_JobStat		*status
);

/**
 * Query LBProxy and use plain communication
 * \param[in] context 		context to operate on
 * \param[in] jobid 		query this job
 * \param[in] flags 		specifies optional status fields to retrieve,
 *     \see EDG_WLL_STAT_CLASSADS, EDG_WLL_STAT_CHILDREN, EDG_WLL_STAT_CHILDSTAT
 * \param[out] status 		the status of the job

 * \warning edg_wll_*Proxy() functions are not implemented in release 1.
 */
int edg_wll_JobStatusProxy(
	edg_wll_Context		context,
	glite_jobid_const_t	jobid,
	int			flags,
	edg_wll_JobStat		*status
);

/**
 * Return all events related to a single job.
 * Convenience wrapper around edg_wll_Query()
 * \param[in] context 		context to work with
 * \param[in] jobId 		job to query
 * \param[out] events 		list of events 
 */

int edg_wll_JobLog(
	edg_wll_Context		context,
	glite_jobid_const_t	jobId,
	edg_wll_Event **	events
);


/**
 * Query LBProxy and use plain communication
 * \warning edg_wll_*Proxy() functions are not implemented in release 1.

 */
int edg_wll_JobLogProxy(
	edg_wll_Context		context,
	glite_jobid_const_t	jobId,
	edg_wll_Event **	events
);

/**
 * All current user's jobs.
 * \param[in] context 		context to work with
 * \param[out] jobs 		list of the user's jobs
 * \param[out] states 		list of the jobs' states
 */
int edg_wll_UserJobs(
	edg_wll_Context		context,
	glite_jobid_t **	jobs,
	edg_wll_JobStat	**	states
);


/**
 * Query LBProxy and use plain communication
 * \warning edg_wll_*Proxy() functions are not implemented in release 1.

 */
int edg_wll_UserJobsProxy(
	edg_wll_Context		context,
	glite_jobid_t **	jobs,
	edg_wll_JobStat	**	states
);

/**
 * Server supported indexed attributes
 * \see DataGrid-01-TEN-0125
 * \param[in] context 		context to work with
 * \param[out] attrs 		configured indices (each index is an UNDEF-terminated
 * 		array of QueryRec's from which only attr (and attr_id 
 * 		eventually) are meaningful
 */
int edg_wll_GetIndexedAttrs(
	edg_wll_Context		context,
	edg_wll_QueryRec	***attrs
);

/**
 * Retrieve limit on query result size (no. of events or jobs).
 * \warning not implemented.
 * \see DataGrid-01-TEN-0125
 * \param[in] context 		context to work with
 * \param[out] limit 		server imposed limit
 */
int edg_wll_GetServerLimit(
	edg_wll_Context	context,
	int		*limit
);

/**
 * UI port for intactive jobs. Used internally by WMS.
 * \param[in] context 		context to work with
 * \param[in] jobId 		job to query
 * \param[in] name 		name of the UI-port
 * \param[out] host 		hostname of port
 * \param[out] port 		port number
 */
int edg_wll_QueryListener(
	edg_wll_Context	context,
	glite_jobid_const_t		jobId,
	const char *	name,
	char **		host,
	uint16_t *	port
);


/**
 * Query LBProxy and use plain communication
 */
int edg_wll_QueryListenerProxy(
	edg_wll_Context	context,
	glite_jobid_const_t		jobId,
	const char *	name,
	char **		host,
	uint16_t *	port
);

/**
 * Ask LB Proxy server for sequence number
 * \param[in] context 		context to work with
 * \param[in] jobId 		job to query
 * \param[out] code 		sequence code
 */


int edg_wll_QuerySequenceCodeProxy(
	edg_wll_Context	context,
	glite_jobid_const_t	jobId,
	char **		code
);
		
/*
 *@} end of group
 */

#ifdef CLIENT_SBIN_PROG
extern int edg_wll_http_send_recv(
	edg_wll_Context,
	char *, const char * const *, char *,
	char **,char ***,char **
);

extern int http_check_status(
	edg_wll_Context,
	char *
);

extern int set_server_name_and_port(
	edg_wll_Context,
	const edg_wll_QueryRec **
);

#endif

#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_CONSUMER_H */
