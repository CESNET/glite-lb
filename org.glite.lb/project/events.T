@type _common_
	timeval	timestamp	timestamp of event generation
	_alias_	date		ULM
	timeval	arrived		timestamp of event store
	_alias_	arr_date	ULM
	_optional_
	string	host		hostname of the machine where the event was generated
	_alias_	host		ULM
	int	level		logging level (system, debug, ...)
	_alias_	lvl		ULM
	_code_	EMERGENCY	emergency
	_code_	ALERT		alert
	_code_	ERROR		error
	_code_	WARNING		warning
	_code_	AUTH		authentication
	_code_	SECURITY	security
	_code_	USAGE		usage
	_code_	SYSTEM		system
	_code_	IMPORTANT	important
	_code_	DEBUG		debug
	int	priority	message priority (yet 0 for asynchronous and 1 for synchronous transfers)
	_null_	-1
	jobid	jobId		DataGrid job id of the source job
	string	seqcode		sequence code assigned to the event
	string	user		identity (cert. subj.) of the generator
	logsrc	source		source (WMS component) which generated this event
#	string	prog		name of program ("EDG WMS" of name of the application)
	string	src_instance	instance of WMS component (e.g. service communication endpoint)
	_optional_

@type Transfer		Start, success, or failure of job transfer to another component
	logsrc	destination	destination where the job is being transfered to
	string	dest_host	destination hostname
	string	dest_instance	destination instance
	_optional_
	string	job		job description in receiver language
	int     result          result of the attempt
	_code_  START           the sending component has started or is about to start the transfer
	_code_  OK              job was sent successfully
	_code_  REFUSED         job was refused by the other component
	_code_  FAIL            transfer failed for other reason than explicit refusal (eg. network timeout)
	string	reason		detailed description of transfer, especially reason of failure
	_optional_
	string	dest_jobid	destination internal jobid
	_optional_

@type Accepted		Accepting job (successful couterpart to Transfer)
	logsrc	from		where was the job received from
	string	from_host	sending component hostname
	string	from_instance	sending component instance
	_optional_
	string	local_jobid	new jobId (Condor, Globus ...) assigned by the receiving component

@type Refused		Refusing job (unsuccessful couterpart to Transfer)
	logsrc	from		where was the job received from
	string	from_host	sending component hostname
	string	from_instance	sending component instance
	_optional_
	string	reason		reason of refusal

@type EnQueued		The job has been enqueued in an inter-component queue
	string	queue		destination queue
	string	job		job description in receiver language
	int     result          result of the attempt
	_code_  START           the sending component has started or is about to start the transfer
	_code_  OK              job was sent successfully
	_code_  REFUSED         job was refused by the other component
	_code_  FAIL            transfer failed for other reason than explicit refusal (eg. network timeout)
	string	reason		detailed description of transfer, especially reason of failure

@type DeQueued		The job has been dequeued from an inter-component queue
	string	queue		queue name
	string	local_jobid	new jobId assigned by the receiving component

@type HelperCall	Helper component is called
	string	helper_name	name of the called component
	string	helper_params	parameters of the call
	int	src_role	whether the logging component is called or calling one
	_code_	CALLING		the logging component is caller
	_code_	CALLED		the logging component is callee

@type HelperReturn	Helper component is returning the control
	string	helper_name	name of the called component
	string	retval		returned data
	int	src_role	whether the logging component is called or calling one
	_code_	CALLING		the logging component is caller
	_code_	CALLED		the logging component is callee

@type Running		Executable started
	string	node		worker node where the executable is run

@type Resubmission	Result of resubmission decision
	int	result		result code
	_code_	WILLRESUB	will be resubmitted
	_code_	WONTRESUB	will not be resubmitted
	string	reason		reason for the decision
	string	tag		value of the attribute on which the decision is based

@type Done		Execution terminated (normally or abnormally)
	int	status_code	way of termination
	_code_	OK		terminated by itself
	_code_	FAILED		disappeared from LRMS
	_code_	CANCELLED	cancelled by user request
	string	reason		reason for the change
	int	exit_code	process exit code
	_null_	-1

@type Cancel		Cancel operation has been attempted on the job
	int	status_code	classification of the cancel
	_code_	REQ		request acknowledged
	_code_	REFUSE		request declined by this component
	_code_	DONE		request completed by whole WMS
	_code_	ABORT		request refused by whole WMS
	string	reason	detailed description

@type Abort		Job aborted by system
	string	reason		reason of abort

@type Clear		Job cleared, output sandbox removed
	int	reason		why the job was cleared
	_code_	USER		user retrieved output sandbox
	_code_	TIMEOUT		timed out, resource purge forced
	_code_	NOOUTPUT	no output was generated

@type Purge		Job is purged from bookkepping server

@type Match		Matching CE found
	string	dest_id		Id of the destination CE/queue
	
@type Pending		No match found yet
	string	reason		why matching CE cannot be found

@type RegJob		New job registration
	string	jdl		job description
	string	ns		NetworkServer handling the job
	jobid	parent		jobid of parent job
	_optional_

	int	jobtype		job type
	_code_	SIMPLE		simple job
	_code_	DAG		dag (containing static set of subjobs)
	_code_	PARTITIONABLE	partitionable (may become partitioned)
	_code_	PARTITIONED	partitioned (dynamically created dag)
	
	int	nsubjobs	number of subjobs
	_optional_
	string	seed		seed for subjob id generation
	_optional_

@type Chkpt		Application-specific checkpoint record
	string	tag		checkpoint tag
	string	classad		checkpoint value

@type Listener 		Listening network port for interactive control
	string	svc_name	port instance name
	string	svc_host	hostname
	port	svc_port	port number

@type CurDescr		current state of job processing (optional event)
	string	descr		description of current job transformation (output of helper)

@type UserTag		user tag -- arbitrary name=value pair
	string	name	tag name
	string	value	tag value

@type ChangeACL		Management of ACL stored on bookkepping server
	string	user_id		DN or VOMS parameter (in format VO:group)
	int	user_id_type	type of information given in user_id (DN or VOMS)
	_null_	-1
	int	permission	ACL permission to change (currently only READ)
	_null_	-1
	int	permission_type	type of permission requested ('allow', 'deny')
	_null_	-1
	int	operation	operation requested to perform with ACL (add, remove)
	_null_	-1

@type Notification	Management of notification service
	notifid	notifId		notification id
	string	owner		owner
	string	dest_host	destination host
	port	dest_port	destination port
	string	jobstat		job status

