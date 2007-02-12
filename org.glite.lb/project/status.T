@type _common_
jobid	jobId		Id of the job
string	owner		Job owner
_index_

int	jobtype		Type of job
	_null_	-1
	_code_ SIMPLE		simple job
	_code_ DAG		composite job
	_code_ COLLECTION	parent of collection of jobs
	_code_ PBS		PBS job
	_code_ CONDOR		Condor job
jobid	parent_job	parent job of subjob

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
timeval	lastUpdateTime	Last known event of the job

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

string	pbs_state	Job state which would probably return PBS qstat (Q/R/C/....)
string  pbs_queue	Name of queue in which is job queued
string  pbs_owner	Owner of job
string  pbs_name	Name of job
string  pbs_reason	Glued reasons/errors leading to pending events
string  pbs_scheduler	Name of pbs scheduler
string  pbs_dest_host	Hostname of node where job is running
int	pbs_pid		PID of running job
int	pbs_exit_status	Job exit status
string	pbs_error_desc	Glued error descriptions from error events

string	condor_status	Condor job status 
string	condor_universe	Condor job Universe (in job ClassAds)
string	condor_owner	Job owner
string	condor_pid	PID of running job
int	condor_notification	Condor notification 
int	condor_job_exit_status	Job exit status
int	condor_starter_exit_status	Condor starter exit status
int	condor_shadow_exit_status	Condor shadow exit status (see h/exit.h)


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
@type Purged		Job has been purged from bookkeeping server (for LB->RGMA interface).
