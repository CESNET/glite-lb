#ifndef __GLITE_LB_PRODUCER_H__
#define __GLITE_LB_PRODUCER_H__

/**
 * \file producer.h
 * \brief client API for storing data into L&B service
 */

#ident "$Header$"
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
my $PRINTPROTOTYPESONLY = 1;
my $PRINTDOC = 1;

for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	my $tu = uc $t;
	my $func = "edg_wll_Log$t";		# name of generated function
	my $funcproxy = $func . "Proxy";
	my $funcc = $func . "CODETOREPLACE";	# name of generated CODE function
	my $funccproxy = $funcc . "Proxy";
	my $p = "edg_wll_Context context"; 	# parameters of generated function
	my $pc = $p;				# parameters of generated CODE function
	my $q = "context,EDG_WLL_EVENT_$tu,EDG_WLL_FORMAT_$tu"; # parameters for LogEvent called in generated function
	my $qc = $q;				# parameters for LogEvent called in generated CODE function
	my $text = "";				# whole text for generated function
	my $textproxy = "";			# whole text for generated CODE function
	my $doc = " * \\param[in,out] context\tcontext to work with, \n"; # parameters description for generated function
	my $docc = $doc;			# parameters description for generated CODE function

	my $decl = "\tint ret;\n";		
	my $free = "";

	selectType $event $t;
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->getName;
		my $sfn = $fn;
		my $ft = "const ";
		my $fc = $f->getComment;
		if ($ULMasString{$f->{type}}) {
			$decl .= "\tchar *s_$fn = ".$f->getType()."ToString($fn);\n";
			$free .= "\tif (s_$fn) free(s_$fn);\n";
			$sfn = "s_$fn";
		}
                if ($f->{codes}) {
# XXX: obsolete         $ft = "enum edg_wll\_$t" . ucfirst $fn;
                        $ft = $ft . "char *";
			$qc = $qc . ", CODETOREPLACE";
                } else {
                        $ft = $ft . $f->getType;
			$pc = $pc . ", $ft $fn";
			$qc = $qc . ", $sfn";
			$docc = $docc . " * \\param[in] $fn\t$fc\n";
                }
		$p = $p . ", $ft $fn";
		$q = $q . ", $sfn";
		$doc = $doc . " * \\param[in] $fn\t$fc\n";
	}
	$text =	qq{
/**
 * \\brief $func
 *
 * $func - simple wrapper around edg_wll_LogEvent for event $t
 *
};
	$textproxy = qq{
/**
 * \\brief $funcproxy
 *
 * $funcproxy - simple wrapper around edg_wll_LogEventProxy for event $t
 *
};
	if ($PRINTDOC) {
		$text = $text . $doc . " * \\see edg_wll_LogEvent()\n";
		$textproxy = $textproxy . $doc . "* \\see edg_wll_LogEventProxy()\n"
	}
	$text = $text . " */\n";
	$textproxy = $textproxy . " */\n";
	if ($PRINTPROTOTYPESONLY) {
		$text = $text . "\nextern int $func($p);\n";
		$textproxy = $textproxy . "\nextern int $funcproxy($p);\n";
	} else {
		$text = $text . qq{
int $func($p)
\{
$decl
	ret = edg_wll_LogEvent($q);
$free
	return ret;
\}
};
		$textproxy = $textproxy . qq{
int $funcproxy($p)
\{
$decl
	ret = edg_wll_LogEventProxy($q);
$free
	return ret;
\}
};
	}
	gen "$text";
	gen "$textproxy";

	#
	# generate also CODE functions:
	#
	for ($event->getFieldsOrdered) {
	my $f = selectField $event $_;
	my $fn = $f->getName;
	if ($f->{codes}) {
	for (@{$f->{codes}}) {
		my $code = uc $_->{name};
		my $funccc = $funcc; $funccc =~ s/CODETOREPLACE/$code/g;
		my $funcccproxy = $funccproxy; $funcccproxy =~ s/CODETOREPLACE/$code/g;
		my $qcc = $qc; $qcc =~ s/CODETOREPLACE/"$code"/g;
		my $textc = qq{
/**
 * \\brief $funccc
 *
 * $funccc - simple wrapper around edg_wll_LogEvent for event $t, $fn $code
 *
};
		my $textcproxy = qq{
/**
 * \\brief $funcccproxy
 *
 * $funcccproxy - simple wrapper around edg_wll_LogEventProxy for event $t, $fn $code
 *
};
		if ($PRINTDOC) {
			$textc = $textc . $docc . " * \\see edg_wll_LogEvent()\n";
			$textcproxy = $textcproxy . $docc . " * \\see edg_wll_LogEventProxy()\n";
		}
		$textc = $textc . " */\n";
		$textcproxy = $textcproxy . " */\n";
		if ($PRINTPROTOTYPESONLY) {
			$textc = $textc . "\nextern int $funccc($pc);\n";
			$textcproxy = $textcproxy . "\nextern int $funcccproxy($pc);\n";
		} else {
			$textc = $textc . qq{
int $funccc($pc)
\{
$decl
	ret = edg_wll_LogEvent($qcc);
$free
	return ret;
\}
};
			$textcproxy = $textcproxy . qq{
int $funcccproxy($pc)
\{
$decl
	ret = edg_wll_LogEventProxy($qcc);
$free
	return ret;
\}
};
		}
		gen "$textc";
		gen "$textcproxy";
	} # for codes
	} # if
	} # for getFields
}
@@@}


/**
 * Formats a logging message and sends it asynchronously to local-logger
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
	const edg_wlc_JobId	job,
	const char *		code,
	int			flags
);

/**
 * Set a current job for given context.
 * \note Should be called before any logging call.
 * \param[in,out] context 	context to work with
 * \param[in] job 		further logging calls are related to this job
 * \param[in] code 		sequence code as obtained from previous component
 * \param[in] user 		user credentials
 * \param[in] flags 		flags on code handling (\see API documentation)
 */
extern int edg_wll_SetLoggingJobProxy(
	edg_wll_Context	context,
	const edg_wlc_JobId	job,
	const char *		code,
	const char *		user,
	int			flags
);

/**
 * Register job with L&B service.
 * Done via logging REGJOB event, may generate subjob id's and create
 * the parent-children associations.
 * Set the job as current for the context and initialize sequence code.
 *
 * Partitionable jobs should set num_subjobs=0 initially,
 * and re-register when number of subjobs becomes known.
 *
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
 
extern int edg_wll_RegisterJob(
	edg_wll_Context		context,
	const edg_wlc_JobId	job,
	enum edg_wll_RegJobJobtype	type,
	const char *		jdl,
	const char *		ns,
	int			num_subjobs,
	const char *		seed,
	edg_wlc_JobId **	subjobs
);

/** 
 * Synchronous variant of edg_wll_RegisterJob
 */

extern int edg_wll_RegisterJobSync(
	edg_wll_Context		context,
	const edg_wlc_JobId	job,
	enum edg_wll_RegJobJobtype	type,
	const char *		jdl,
	const char *		ns,
	int			num_subjobs,
	const char *		seed,
	edg_wlc_JobId **	subjobs
);

/**
 * Register job with L&B Proxy service.
 * Done via logging REGJOB event, may generate subjob id's and create
 * the parent-children associations.
 * Set the job as current for the context and initialize sequence code.
 *
 * Partitionable jobs should set num_subjobs=0 initially,
 * and re-register when number of subjobs becomes known.
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
	const edg_wlc_JobId	job,
	enum edg_wll_RegJobJobtype	type,
	const char *		jdl,
	const char *		ns,
	int			num_subjobs,
	const char *		seed,
	edg_wlc_JobId **	subjobs
);

#ifdef LB_PERF
/* original register to LBProxy 	*/
extern int edg_wll_RegisterJobProxyOld(
	edg_wll_Context		context,
	const edg_wlc_JobId	job,
	enum edg_wll_RegJobJobtype	type,
	const char *		jdl,
	const char *		ns,
	int			num_subjobs,
	const char *		seed,
	edg_wlc_JobId **	subjobs
);

/* register only to LBProxy 		*/
/* useful for performance measurements	*/

extern int edg_wll_RegisterJobProxyOnly(
	edg_wll_Context		context,
	const edg_wlc_JobId	job,
	enum edg_wll_RegJobJobtype	type,
	const char *		jdl,
	const char *		ns,
	int			num_subjobs,
	const char *		seed,
	edg_wlc_JobId **	subjobs
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
	const edg_wlc_JobId	parent,
	char const * const *	jdls,
	const char *		ns,
	edg_wlc_JobId const *	subjobs
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
	const edg_wlc_JobId	parent,
	char const * const *	jdls,
	const char *		ns,
	edg_wlc_JobId const *	subjobs
);

/**
 * Generate or regenerate set of subjob ID's.
 * Calls the same algorithm used to generate subjob ID's in edg_wll_RegisterJob().
 * Local semantics only, server is not contacted.
 */

extern int edg_wll_GenerateSubjobIds(
	edg_wll_Context		context,
	const edg_wlc_JobId	parent,
	int			num_subjobs,
	const char *		seed,
	edg_wlc_JobId **	subjobs
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
	const edg_wlc_JobId	job,
	const char *		user_id,
	enum edg_wll_UserIdType	user_id_type,
	enum edg_wll_Permission		permission,
	enum edg_wll_PermissionType	permission_type,
	enum edg_wll_ACLOperation	operation
);


#ifdef __cplusplus
}
#endif

#endif /* __GLITE_LB_PRODUCER_H__ */
