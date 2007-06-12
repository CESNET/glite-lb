@type _common_
	timeval	timestamp	Time the event was generated.
	_alias_	date		ULM
	timeval	arrived		Time the event was stored into the bookkeeping server database.
	_alias_	arr_date	ULM
	_optional_
	string	host		Hostname of the machine where the event was generated.
	_alias_	host		ULM
	int	level		Logging level (in the range from DEBUG to EMERGENCY).
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
	int	priority	Message priority (yet 0 for asynchronous and 1 for synchronous transfers).
	_null_	-1
	jobid	jobId		Grid job id of the job the event belongs to.
	string	seqcode		Sequence code assigned to the event.
	string	user		Identity (certificate subject) of the event sender.
	logsrc	source		Source (software component) which generated this event.
#	string	prog		name of program ("EDG WMS" of name of the application).
	string	src_instance	Instance of source component (e.g. service communication endpoint).
	_optional_

@type Transfer		Start, success, or failure of job transfer to another component.
	logsrc	destination	Destination where the job is being transfered to.
	string	dest_host	Hostname of server that takes over control of the job.
	string	dest_instance	Service (instance) that takes over control of the job.
	_optional_
	string	job		Job description in receiver's language.
	int     result          Result code of the transfer attempt (START, OK, REFUSED or FAIL).
	_code_  START           The sending component has started or is about to start the transfer.
	_code_  OK              The job was sent successfully.
	_code_  REFUSED         The job was refused by the other component.
	_code_  FAIL            The transfer failed for other reason than explicit refusal (eg. network timeout).
	string	reason		Detailed description of the transfer, especially reason of failure.
	_optional_
	string	dest_jobid	Job id as assigned by the receiving software component.
	_optional_

@type Accepted		Accepting job (successful counterpart to Transfer).
	logsrc	from		The software component  the job was received from.
	string	from_host	Hostname of the component the job was received from.
	string	from_instance	Instance of the component the job was received from.
	_optional_
	string	local_jobid	New job id as assigned by the receiving component.

@type Refused		Refusing job (unsuccessful counterpart to Transfer).
	logsrc	from		The software component that tried to send the job.
	string	from_host	Hostname of the component that tried to send the job.
	string	from_instance	Instance of the component that tried to send the job.
	_optional_
	string	reason		Description of the reason why the job was refused.

@type EnQueued		The job has been enqueued in an inter-component queue.
	string	queue		Queue into which the job has been stored for retrieval by another component.
	string	job		Job description in the receiver's language.
	int     result          Result code of the attempt to put job into the queue (START, OK, REFUSED or FAIL).
	_code_  START           The sending component has started or is about to start enqueuing the job.
	_code_  OK              The job was enqueued successfully.
	_code_  REFUSED         The job was refused by the other component.
	_code_  FAIL            The transfer failed for other reason than explicit refusal.
	string	reason		Detailed description of the attempt to enqueue the job, especially the reason of failure.

@type DeQueued		The job has been dequeued from an inter-component queue.
	string	queue		Name of the queue the job was obtained from.
	string	local_jobid	New job id as assigned by the retreiving component.

@type HelperCall	Helper component is called.
	string	helper_name	Name of the called helper component.
	string	helper_params	Parameters of the call to the helper component.
	int	src_role	The role the event sender is playing in the helper call (CALLING or CALLEE).
	_code_	CALLING		The logging component is caller.
	_code_	CALLED		The logging component is callee.

@type HelperReturn	Helper component is returning the control.
	string	helper_name	Name of the called helper component.
	string	retval		Data returned by the call to the helper component.
	int	src_role	The role the event sender is playing in the helper call (CALLING or CALLEE).
	_code_	CALLING		The logging component is caller.
	_code_	CALLED		The logging component is callee.

@type Running		Job wrapper started.
	string	node		Worker node on which the job executable is being run.

@type Resubmission	Result of resubmission decision.
	int	result		Result code of the resubmission decision (WILLRESUB or WONTRESUB or SHALLOW).
	_code_	WILLRESUB	The job will be resubmitted (deep resubmission).
	_code_	WONTRESUB	The job will not be resubmitted.
	_code_	SHALLOW		Shallow resubmission (user payload has not started yet)
	string	reason		Reason why the job will or will not be resubmitted.
	string	tag		Value of the attribute on which the decision to resubmit the job was based.

@type Done		Execution terminated (normally or abnormally).
	int	status_code	Reason code for the termination of the job (OK, FAILED or CANCELLED).
	_code_	OK		The job terminated by itself.
	_code_	FAILED		The job disappeared from LRMS.
	_code_	CANCELLED	The job was cancelled by user request.
	string	reason		Detailed description why the job was terminated.
	int	exit_code	Exit code of the job's process.
	_null_	-1

@type Cancel		Cancel operation has been attempted on the job.
	int	status_code	Classification of the attempt to cancel the job (REQ, REFUSE, DONE or ABORT).
	_code_	REQ		The request was acknowledged.
	_code_	REFUSE		The request was declined by this component.
	_code_	DONE		The request was completed by whole WMS.
	_code_	ABORT		The request was refused by whole WMS.
	string	reason		Detailed description of the attempt to cancel the job, especially the reason of failure.

@type Abort		Job aborted by system.
	string	reason		Reason why the job was aborted by the system.

@type Clear		Job cleared, output sandbox removed
	int	reason		Description of the reason why the job was cleared and the output sandbox removed (USER, TIMEOUT or NOOUTPUT).
	_code_	USER		User retrieved output sandbox.
	_code_	TIMEOUT		Timed out, resource forced purge of the sandbox.
	_code_	NOOUTPUT	No output was generated.

@type Purge		Job is purged from bookkepping server.

@type Match		Matching CE found.
	string	dest_id		Identification of the queue on the CE that the job could be send to.
	
@type Pending		No matching CE found yet.
	string	reason		Description why the matching CE for the job was not found (yet).

@type RegJob		New job registration.
	string	jdl		Job description of the job being registered.
	string	ns		NetworkServer handling the newly registered job.
	jobid	parent		Grid job id of the parent job registering this new one.
	_optional_

	int	jobtype		Type of the job being registered (SIMPLE, DAG, PARTITIONABLE or PARTITIONED).
	_code_	SIMPLE		The job is simple job.
	_code_	DAG		The job is dag (containing static set of subjobs).
	_code_	PARTITIONABLE	The job is partitionable (may become partitioned).
	_code_	PARTITIONED	The job is partitioned (dynamically created dag).
	_code_	COLLECTION	The job is collection (containing static set of subjobs).
	_code_	PBS		PBS job
	_code_	CONDOR		Condor job
	
	int	nsubjobs	Number of subjobs this job plans to spawn.
	_optional_
	string	seed		Seed for subjob id generation.
	_optional_

@type Chkpt		Application-specific checkpoint record.
	string	tag		Application specific checkpoint tag.
	string	classad		Application specific checkpoint value.

@type Listener 		Listening network port for interactive control.
	string	svc_name	Name of the port instance for interactive job control.
	string	svc_host	Hostname of the interactive job controller.
	port	svc_port	Port number of the interactive job controller.

@type CurDescr		Current state of job processing (optional event).
	string	descr		Description of the current job transformation (output of the helper).

@type UserTag		User tag -- arbitrary name=value pair.
	string	name	Arbitrary user tag name.
	string	value	Arbitrary user tag value.

@type ChangeACL		Management of ACL stored on bookkepping server.
	string	user_id		DN or VOMS parameter (in format VO:group).
	int	user_id_type	Type of information given in user_id (DN or VOMS).
	_null_	-1
	int	permission	ACL permission to change (currently only READ).
	_null_	-1
	int	permission_type	Type of permission requested ('allow', 'deny').
	_null_	-1
	int	operation	Operation requested to perform with ACL (add, remove).
	_null_	-1

@type Notification	Management of notification service.
	notifid	notifId		Notification id.
	string	owner		Identification of the job owner (certificate subject).
	string	dest_host	Hostname the notification is sent to.
	port	dest_port	Port number the notification is sent to.
	string	jobstat		Status of the job (the notification content).


@type ResourceUsage	Resource (CPU, memory etc.) consumption.
	string	resource	Resource's name.
	double	quantity	Resources's quantity (how much).
	string	unit		Units (sec, kB, etc.).

@type ReallyRunning	User payload started.
	_optional_
	string	wn_seq		Sequence code on the worker node.

@type Suspend		Job execution (queuing) was suspended.
	_optional_
	string	reason		Reason for the suspend.

@type Resume		Job execution (queuing) was resumed.
	_optional_
	string	reason		Reason for the resume.

@type CollectionState	State of the collection.
	string	state		New collection state.
	_optional_
	int	done_code	In case of (state == Done) contains done code 
	_null_	-1
	string	histogram	User readable histogram; useful for debugging.
	jobid   child		JobId of subjob, which triggered the state change.	
	string	child_event	Event which triggered the state change.	


@type PBSQueued		Job enqued
	string	queue		Queue name
	string	owner		Job owner
	string	name		Job name

@type PBSMatch		Scheduler created exec_host
	string	dest_host	Aka exec_host

@type PBSPending	Scheduler is not able to find exec_host, or some error occured
	string	reason		Reasons of job pendation or errors

@type PBSRun		Job attempted to be run by the logging component
	string	scheduler	Scheduler ID
	_optional_
	string	dest_host	Where to run the job
	_optional_
	int	pid		Actual process ID
	_optional_

@type PBSRerun            Job rerun requested

@type PBSDone		Job terminated
	int	exit_status	Exit status
	_optional_		Bypass need of 'null value'

@type PBSDequeued	Job dequeued

@type PBSResourceUsage	Resources requested/consumed
	int	usage		Type of record
	_code_	REQUESTED	Requested value
	_code_	USED		Consumed quantity
	string	name		Name of resource
	double	quantity	The quantity
	_optional_		Bypass need of 'null value'
	string	unit		Units (sec, kB, etc.)

@type PBSError		Any error occured
	string	error_desc	Error reason

@type CondorMatch	Job MATCHed
	string	owner		Owner
	string	dest_host	Matched host
	string	preempting	Preempting
	_optional_

@type CondorReject	Job REJECTed
	string	owner		Owner
	int	status_code	Reason code for the rejection of the job
	_code_	NOMATCH		No match found
	_code_	OTHER		Other reason

@type CondorShadowStarted	Condor Shadow Started
	string	shadow_host	Shadow host
	port	shadow_port	Shadow port
	int	shadow_pid	PID of shadow process
	string	shadow_status	Shadow status
	_optional_

@type CondorShadowExited	Condor Shadow Exited
	int	shadow_pid	PID of shadow process
	int	shadow_exit_status	Exit status of shadow process
	_null_	-1

@type CondorStarterStarted	Condor Starter Started
	int	starter_pid	PID of starter process
	_optional_
	string	universe	Condor Universe
	_optional_

@type CondorStarterExited	Condor Starter Exited
	int	starter_pid	PID of starter process
	_optional_
	int	starter_exit_status	Exit status of starter process
	_null_	-1
	_optional_
	int	job_pid		PID of running job
	_optional_
	int	job_exit_status	Job exit status
	_null_	-1
	_optional_

@type CondorResourceUsage       Resources requested/consumed
	int	usage		Type of record
	_code_	REQUESTED	Requested value
	_code_	USED		Consumed quantity
	string	name		Name of resource
	double	quantity	The quantity
	_optional_		Bypass need of 'null value'
	string	unit		Units (sec, kB, etc.)

@type CondorError		Any Error occured
	string	error_desc	Error reason

