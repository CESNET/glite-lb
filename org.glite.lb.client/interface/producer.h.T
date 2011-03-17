#ifndef GLITE_LB_PRODUCER_H
#define GLITE_LB_PRODUCER_H

/**
 * \file producer.h
 * \brief client API for storing data into L&B service
 */

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

/*
@@@AUTO
*/
@@@LANG: C

#ifdef __cplusplus
extern "C" {
#endif

#include "glite/lb/context.h"
#include "glite/lb/events.h"

/* Event sources: */

#if 0 /* obsolete */
#define EDG_WLL_SOURCE_UI           "UserInterface"
#define EDG_WLL_SOURCE_RB           "ResourceBroker"
#define EDG_WLL_SOURCE_JSS          "JobSubmissionService"  /* aka Condor-G */
#define EDG_WLL_SOURCE_JOBMGR       "GlobusJobmanager"
#define EDG_WLL_SOURCE_LRMS         "LocalResourceManager"
#define EDG_WLL_SOURCE_APP          "Application"

#define EDG_WLL_SOURCE_NS	"NetworkServer"
#define EDG_WLL_SOURCE_WM	"WorkloadManager"
#define EDG_WLL_SOURCE_BH	"BigHelper"
#define EDG_WLL_SOURCE_LM	"LogMonitor"

#endif

/* edg_wll_LogEvent shortcuts */
@@@{
$PRINTPROTOTYPESONLY = 1;
$PRINTDOC = 1;

require './uiwrap.T';

@@@}


/**
 * Formats a logging message and sends it asynchronously to local-logger
 * (the connection is through GSS)
 * \brief generic asynchronous logging function
 * \param[in,out] context	context to work with,
 * \param[in] event		type of the event,
 * \param[in] fmt		printf()-like format string,
 * \param[in] ...		event specific values/data according to fmt,
 * \retval 0		successful completition,
 * \retval EINVAL	bad jobId, unknown event code, or the format string together with the remaining arguments does not form a valid event,
 * \retval ENOSPC	L&B infrastructure failed to accept the event due to lack of disk space etc.,
 * \retval ENOMEM	failed to allocate memory,
 * \retval ECONNREFUSED	cannot connect to the specified local logger,
 * \retval EAGAIN	non blocking return from the call, the event may or may not get logged,
 * \retval EDG_WLL_ERROR_NOJOBID logging call attempted without assigning jobId to the context.
 */
extern int edg_wll_LogEvent(
	edg_wll_Context context,
	edg_wll_EventCode event,
	char *fmt, ...);

/**
 * Formats a logging message and sends it synchronously to local-logger
 * (the connection is through GSS)
 * \brief generic synchronous logging function
 * \param[in,out] context	context to work with,
 * \param[in] event		type of the event,
 * \param[in] fmt		printf()-like format string,
 * \param[in] ...		event specific values/data according to fmt,
 * \retval 0		successful completition,
 * \retval EINVAL	bad jobId, unknown event code, or the format string together with the remaining arguments does not form a valid event,
 * \retval ENOSPC	L&B infrastructure failed to accept the event due to lack of disk space etc.,
 * \retval ENOMEM	failed to allocate memory,
 * \retval ECONNREFUSED	cannot connect to the specified local logger,
 * \retval EAGAIN	non blocking return from the call, the event may or may not get logged,
 * \retval EDG_WLL_ERROR_NOJOBID logging call attempted without assigning jobId to the context,
 * \retval EPERM	the user is not authorized to add events to this job,
 * \retval EDG_WLL_ERROR_DB_DUP_KEY exactly the same event has been already stored.
 */
extern int edg_wll_LogEventSync(
	edg_wll_Context context,
	edg_wll_EventCode event,
	char *fmt, ...);

/**
 * Formats a logging message and sends it synchronously to L&B Proxy
 * (the connection is through UNIX socket)
 * \brief generic synchronous logging function
 * \param[in,out] context	context to work with,
 * \param[in] event		type of the event,
 * \param[in] fmt		printf()-like format string,
 * \param[in] ...		event specific values/data according to fmt,
 * \retval 0            successful completition,
 * \retval EINVAL       bad jobId, unknown event code, or the format string together with the remaining arguments does not form a valid event,             
 * \retval ENOSPC       L&B infrastructure failed to accept the event due to lack of disk space etc.,
 * \retval ENOMEM       failed to allocate memory,
 * \retval ECONNREFUSED cannot connect to the specified L&B Proxy
 * \retval EAGAIN       non blocking return from the call, the event may or may not get logged,
 * \retval EDG_WLL_ERROR_NOJOBID logging call attempted without assigning jobId to the context.
 */             
extern int edg_wll_LogEventProxy(
        edg_wll_Context context, 
        edg_wll_EventCode event,
        char *fmt, ...);        

/**
 * Instructs interlogger to to deliver all pending events related to current job
 * \brief flush events from interlogger
 * \note sort of status query more than a command
 * \param[in,out] context	context to work with,
 * \param[in,out] timeout	wait at most this much time for completition, remaining time on return,
 * \retval 0		successful completition,
 * \retval EDG_WLL_ERROR_INTERLOG_TIMEOUT the inter-logger did not respond within the timeout,
 * \retval EDG_WLL_ERROR_INTERLOG_CONLOST inter-logger lost connection to one or more servers,
 * \retval EDG_WLL_ERROR_INTERLOG_AGAIN   not all pending events were delivered within the timeout.
 */
extern int edg_wll_LogFlush(
	edg_wll_Context context,
	struct timeval *timeout);


/**
 * Instructs interlogger to to deliver all pending events
 * \brief flush all events from interlogger
 * \note same as edg_wll_LogFlush() for all jobs known to interlogger
 * \see edg_wll_LogFlush()
 */
extern int edg_wll_LogFlushAll(
	edg_wll_Context context,
	struct timeval *timeout);

/**
 * Set a current job for given context.
 * \note Should be called before any logging call.
 * \param[in,out] context 	context to work with
 * \param[in] job 		further logging calls are related to this job
 * \param[in] code 		sequence code as obtained from previous component
 * \param[in] flags 		flags on code handling (\see API documentation)
 */
extern int edg_wll_SetLoggingJob(
	edg_wll_Context	context,
	glite_jobid_const_t	job,
	const char *		code,
	int			flags
);

/**
 * Set a current job for given context.
 * \note Should be called before any call logging to lbproxy.
 * \param[in,out] context 	context to work with
 * \param[in] job 		further logging calls are related to this job
 * \param[in] code 		sequence code as obtained from previous component
 * \param[in] user 		user credentials
 * \param[in] flags 		flags on code handling (\see API documentation)
 */
extern int edg_wll_SetLoggingJobProxy(
	edg_wll_Context	context,
	glite_jobid_const_t	job,
	const char *		code,
	const char *		user,
	int			flags
);

/**
 * Register job with L&B service.
 * Done via logging REGJOB event directly to LB server (the connection is asynchronous through GSS)
 * - may generate subjob id's and create the parent-children associations,
 * - set the job as current for the context and initialize sequence code,
 * - partitionable jobs should set num_subjobs=0 initially, 
 *   and re-register when the number of subjobs becomes known.
 *
 * \brief asynchronous job registration
 * \param[in,out] context 	context to work with
 * \param[in] job 		jobId
 * \param[in] type 		EDG_WLL_JOB_SIMPLE,  EDG_WLL_JOB_DAG, or EDG_WLL_JOB_PARTITIONABLE
 * \param[in] jdl 		user-specified JDL
 * \param[in] ns 		network server contact
 * \param[in] num_subjobs 	number of subjobs to create
 * \param[in] seed 		seed used for subjob id's generator.
 * 	Use non-NULL value to be able to regenerate the set of jobid's
 * \param[out] subjobs 		returned subjob id's
 */

/* backward compatibility */
#define EDG_WLL_JOB_SIMPLE	EDG_WLL_REGJOB_SIMPLE
#define edg_wll_RegisterJobAsync	edg_wll_RegisterJob 

extern int edg_wll_RegisterJob(
	edg_wll_Context		context,
	glite_jobid_const_t	job,
	enum edg_wll_RegJobJobtype	type,
	const char *		jdl,
	const char *		ns,
	int			num_subjobs,
	const char *		seed,
	glite_jobid_t **	subjobs
);

#if 0
DEPRECATED?
/** 
 * Synchronous variant of edg_wll_RegisterJob
 */

extern int edg_wll_RegisterJobSync(
	edg_wll_Context		context,
	glite_jobid_const_t	job,
	enum edg_wll_RegJobJobtype	type,
	const char *		jdl,
	const char *		ns,
	int			num_subjobs,
	const char *		seed,
	glite_jobid_t **	subjobs
);
#endif
#define edg_wll_RegisterJobSync edg_wll_RegisterJob

/**
 * Register job with L&B Proxy service.
 * Done via logging REGJOB event in parallel both 
 * to LB proxy (through UNIX socket) and 
 * to LB server (through GSS)
 * - may generate subjob id's and create the parent-children associations,
 * - set the job as current for the context and initialize sequence code,
 * - partitionable jobs should set num_subjobs=0 initially,
 *   and re-register when number of subjobs becomes known.
 *
 * \param[in,out] context	context to work with
 * \param[in] job		jobId
 * \param[in] type 		EDG_WLL_JOB_SIMPLE, EDG_WLL_JOB_DAG, or EDG_WLL_JOB_PARTITIONABLE
 * \param[in] jdl 		user-specified JDL
 * \param[in] ns 		network server contact
 * \param[in] num_subjobs 	number of subjobs to create
 * \param[in] seed 		seed used for subjob id's generator.
 *      Use non-NULL value to be able to regenerate the set of jobid's
 * \param[out] subjobs 		returned subjob id's
 */

extern int edg_wll_RegisterJobProxy(
	edg_wll_Context		context,
	glite_jobid_const_t	job,
	enum edg_wll_RegJobJobtype	type,
	const char *		jdl,
	const char *		ns,
	int			num_subjobs,
	const char *		seed,
	glite_jobid_t **	subjobs
);

/** 
 * Job registration with an extra ACL specifying WMS to acces the job
 * (the connection to LB server is asynchronous through GSS)
 * \param[in] wms_dn 		DN of WMS handling the job
 */

extern int edg_wll_RegisterJobExt(
	edg_wll_Context		context,
	glite_jobid_const_t	job,
	enum edg_wll_RegJobJobtype	type,
	const char *		jdl,
	const char *		ns,
	int			num_subjobs,
	const char *		seed,
	glite_jobid_t **	subjobs,
	char **			wms_dn,
	int			flags
);

extern int edg_wll_RegisterJobProxyExt(
	edg_wll_Context		context,
	glite_jobid_const_t	job,
	enum edg_wll_RegJobJobtype	type,
	const char *		jdl,
	const char *		ns,
	int			num_subjobs,
	const char *		seed,
	glite_jobid_t **	subjobs,
	char **			wms_dn,
	int			flags
);

#ifdef LB_PERF
/* register only to LBProxy 		*/
/* useful for performance measurements	*/

extern int edg_wll_RegisterJobProxyOnly(
	edg_wll_Context		context,
	glite_jobid_const_t	job,
	enum edg_wll_RegJobJobtype	type,
	const char *		jdl,
	const char *		ns,
	int			num_subjobs,
	const char *		seed,
	glite_jobid_t **	subjobs
);
#endif



/**
 * Register subjobs in a batch.
 * Mainly used to provide JDL's of individual subjobs in a more efficient
 * way than logging them one by one.
 * \param[in,out] context	context to work with
 * \param[in] parent		parent's jobId
 * \param[in] jdls		array of JDL's
 * \param[in] ns 		network server contact
 * \param[in] subjobs 		array of jobid's in the same order
 */

extern int edg_wll_RegisterSubjobs(
	edg_wll_Context		context,
	glite_jobid_const_t	parent,
	char const * const *	jdls,
	const char *		ns,
	glite_jobid_t const *	subjobs
);


/**
 * Register subjobs to LB Proxyin a batch.
 * Mainly used to provide JDL's of individual subjobs in a more efficient
 * way than logging them one by one.
 * \param[in,out] context	context to work with
 * \param[in] parent		parent's jobId
 * \param[in] jdls		array of JDL's
 * \param[in] ns 		network server contact
 * \param[in] subjobs 		array of jobid's in the same order
 */

extern int edg_wll_RegisterSubjobsProxy(
	edg_wll_Context		context,
	glite_jobid_const_t	parent,
	char const * const *	jdls,
	const char *		ns,
	glite_jobid_t const *	subjobs
);

/**
 * Register file transfer subjobs in a batch.
 * \param[in,out] context       context to work with
 * \param[in] parent            parent's jobId
 * \param[in] ns                network server contact
 * \param[in] nsubjobs		number of subjobs
 * \param[in] subjobs           array of jobid's in the same order
 */

extern int edg_wll_RegisterFTSubjobs(
        edg_wll_Context         ctx,
        glite_jobid_const_t     parent,
        const char *            ns,
        int                     nsubjobs,
        edg_wlc_JobId const *   subjobs
);

/**
 * Change ACL for given job.
 * \param[in,out] context 	context to work with
 * \param[in] job 		jobId
 * \param[in] user_id 		specification of user's credential
 * \param[in] user_id_type 	type of user_id,
 *    for EDG_WLL_USER_SUBJECT the user_id parameter is expected to be user's subject name
 *    for EDG_WLL_USER_VOMS_GROUP the user_id is expected to be of the form VO:group specifying required group membersip as managed by VOMS
 * \param[in] permission 	ACL permission to change
 * \param[in] permission_type 	type of given permission (allow or deny operation) 
 * \param[in] operation 	operation to perform with ACL (add or remove record)
 */
 
extern int edg_wll_ChangeACL(
	edg_wll_Context		context,
	glite_jobid_const_t	job,
	const char *		user_id,
	enum edg_wll_ChangeACLUser_id_type	user_id_type,
	enum edg_wll_ChangeACLPermission	permission,
	enum edg_wll_ChangeACLPermission_type	permission_type,
	enum edg_wll_ChangeACLOperation		operation
);


#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_PRODUCER_H */
