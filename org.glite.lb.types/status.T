@type _common_
jobid	jobId		Id of the job
string	owner		Job owner
_index_

int	jobtype		Type of job
	_null_	-1
	_code_ SIMPLE			simple job
	_code_ DAG			composite job
	_code_ _PARTITIONABLE_UNUSED	placholder for unused code
	_code_ _PARTITIONED_UNUSED	placholder for unused code
	_code_ COLLECTION		parent of collection of jobs
	_code_ PBS			PBS job
	_code_ CONDOR			Condor job
	_code_ CREAM			CREAM job
	_code_ FILE_TRANSFER_COLLECTION	job containing all file transfers (i.e. sandbox)
	_code_ FILE_TRANSFER		subjob holding one file transfer
	_code_ VIRTUAL_MACHINE		job representing virtual machine
jobid	parent_job			parent job of subjob

string	seed		string used for generation of subjob IDs
int	children_num	number of subjobs
strlist	children	list of subjob IDs
	_special_	XMLstructured
intlist	children_hist	summary (histogram) of children job states
	_special_	XMLstructured
stslist children_states full status information of the children
	_special_	XMLstructured

string 	condorId	Id within Condor-G
string	globusId	Globus allocated Id
string	localId		Id within LRMS

string	jdl		User submitted job description
string	matched_jdl	Full job description after matchmaking
string	destination	ID of CE where the job is being sent
_index_
string	condor_jdl	ClassAd passed to Condor-G for last job execution
string	rsl		Job RSL sent to Globus

string	reason		Reason of being in this status, if any

string	location	Where the job is being processed
_index_
string	ce_node		Worker node where the job is executed
string	network_server	Network server handling the job
_index_

bool	subjob_failed	Subjob failed (the parent job will fail too)
int	done_code		Return code
	_null_	-1
	_code_	OK		Finished correctly
	_code_	FAILED		Execution failed
	_code_	CANCELLED	Cancelled by user
int	exit_code		Unix exit code
bool	resubmitted	The job was resubmitted

bool	cancelling	Cancellation request in progress
string	cancelReason	Reason of cancel

int	cpuTime		Consumed CPU time
	_null_	-1

taglist	user_tags	List of pairs (user_tag, user_value)
	_special_	XMLstructured

timeval	stateEnterTime	When entered this status
_index_
timeval	lastUpdateTime	Last known event of the job
_index_

intlist	stateEnterTimes	When all previous states were entered
	_special_	XMLstructured

bool	expectUpdate	Some logged information has not arrived yet
string	expectFrom	Sources of the missing information
string	acl		ACL of the job

bool	payload_running	User payload started
strlist	possible_destinations	Possible job destinations
	_special_	XMLstructured
strlist possible_ce_nodes	CE nodes matching to possible_destinations
	_special_	XMLstructured

bool	suspended	Job is suspended
string	suspend_reason	Reason for the suspend

string	failure_reasons Concatenated job failure reasons, possibly from deeper middleware layers
bool	remove_from_proxy	Job reached final status and may be removed from proxy
string	ui_host		Hostname or address of UI used to submit the job
strlist	user_fqans	FQAN used to register the job
	_special_	XMLstructured
int	sandbox_retrieved	Flag is set on incomming Clear event
	_code_	NOTYET		Sandbox was not retrieved yet.
	_code_	USER		User retrieved output sandbox.
	_code_	TIMEOUT		Timed out, resource forced purge of the sandbox.
	_code_	NOOUTPUT	No output was generated.
int	jw_status		Status of Job wrapper
	_code_	UNKNOWN		Status of JW unknown
	_code_	WRAPPER_RUNNING Wrapper started
	_code_	PAYLOAD_RUNNING Payload running
	_code_	DONE		JW finished
cclassad	jdl_classad	Unparsed JDL classad

jobid	isb_transfer		Input sandbox corresponding to the job
jobid	osb_transfer		Output sandbox corresponding to the job
string	payload_owner		Owner of the actual job payload (e.g., for pilot jobs)
string	access_rights			List of access rights (human readable variant of acl)
string	history			Job history.

_pad_	55

string	pbs_state	Job state which would probably return PBS qstat (Q/R/C/....)
int	pbs_substate	Job substate
string  pbs_queue	Name of queue in which is job queued
string  pbs_owner	Owner of job
string  pbs_name	Name of job
string  pbs_reason	Glued reasons/errors leading to pending events
string  pbs_scheduler	Name of pbs scheduler
string  pbs_dest_host	Hostname of node where job is running
int	pbs_pid		PID of running job
taglist pbs_resource_requested	Resources requested
taglist	pbs_resource_usage	Resource usage
int	pbs_exit_status	Job exit status
string	pbs_error_desc	Glued error descriptions from error events

_pad_	20

string	condor_status	Condor job status 
string	condor_universe	Condor job Universe (in job ClassAds)
string	condor_owner	Job owner
string	condor_preempting	Job preempting
int	condor_shadow_pid	PID of Shadow
int	condor_shadow_exit_status	Condor shadow exit status (see h/exit.h)
int	condor_starter_pid	PID of Starter
int	condor_starter_exit_status	Condor starter exit status
int	condor_job_pid	PID of running job
int	condor_job_exit_status	Job exit status
string	condor_dest_host Hostname of node where job is running
string	condor_reason	Glued reasons/errors leading to pending events
string	condor_error_desc	Glued error descriptions from error events

_pad_	25

int	cream_state	CREAM job state
	_null_	-1
	_code_	REGISTERED	registered
	_code_	PENDING		pending
	_code_	IDLE		idle
	_code_	RUNNING		running
	_code_	REALLY-RUNNING	really-running
	_code_	HELD		held
	_code_	DONE-OK		done-ok
	_code_	DONE-FAILED	done-failed
	_code_	ABORTED		aborted

string  cream_id		CREAM local job id
string	cream_owner		Job owner
string	cream_endpoint		Address of the CREAM service
string	cream_jdl		JDL passed to the CREAM
string	cream_reason		Reason of the state or command
string  cream_failure_reason 	Reason of any failure
string	cream_lrms_id		LRMS local id
string	cream_node		Worker node where the job is executed
int	cream_done_code		Job done code, uses done_code enum
	_null_	-1
int	cream_exit_code		Job exit code
	_null_ -1
bool	cream_cancelling 	Cancellation request in process
int	cream_cpu_time		Consumed CPU time
int	cream_jw_status		Users jw_status enum
	_null_ -1

_pad_	30

jobid	ft_compute_job		Compute job corresponding to file transfer
int	ft_sandbox_type		Type of sandbox
	_null_  -1
	_code_	INPUT		input
	_code_	OUTPUT		output
string	ft_src			File transfer source
string	ft_dest			File transfer destination

_pad_	20

int	vm_state		VM job state
	_null_ 	-1
	_code_	VM_PENDING	pending
	_code_	VM_BOOT		boot
	_code_	VM_RUNNING	running
	_code_	VM_SHUTDOWN	shutdown
	_code_	VM_STOPPED	stopped
	_code_	VM_DONE		done
	_code_ 	VM_FAILURE	failure
string	vm_image		VM image
string	vm_require		VM requirement on physical machine
string	vm_usage		VM usage
string	vm_hostname		VM hostname
string	vm_machine		physical machine on which VM runs

_pad_ 	20

@type Submitted		Entered by the user to the User Interface or registered by Job Partitioner.
@type Waiting		Accepted by WMS, waiting for resource allocation.
@type Ready		Matching resources found.
@type Scheduled		Accepted by LRMS queue.
@type Running		Executable is running.
@type Done		Execution finished, output is available.
@type Cleared		Output transfered back to user and freed.
@type Aborted		Aborted by system (at any stage).
@type Cancelled		Cancelled by user.
@type Unknown		Status cannot be determined.
@type Purged		Job has been purged from bookkeeping server (for LB-RGMA interface).

@terminal Aborted
@terminal Cancelled
@terminal Cleared
@terminal Purged
