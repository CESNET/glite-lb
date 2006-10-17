@type _common_
jobid	jobId		Id of the job
string	owner		Job owner
_index_
	_special_	JP

int	jobtype		Type of job
	_null_	-1
	_code_ SIMPLE	simple job
	_code_ DAG	composite job
	_special_	JP
jobid	parent_job	parent job of subjob
	_special_	JP

string	seed		string used for generation of subjob IDs
	_special_	JP
int	children_num	number of subjobs
	_special_	JP
strlist	children	list of subjob IDs
	_special_	XMLstructured
intlist	children_hist	summary (histogram) of children job states
	_special_	XMLstructured
stslist children_states full status information of the children
	_special_	XMLstructured

string 	condorId	Id within Condor-G
	_special_	JP
string	globusId	Globus allocated Id
	_special_	JP
string	localId		Id within LRMS
	_special_	JP

string	jdl		User submitted job description
	_special_	JP
string	matched_jdl	Full job description after matchmaking
	_special_	JP
string	destination	ID of CE where the job is being sent
	_special_	JP
_index_
string	condor_jdl	ClassAd passed to Condor-G for last job execution
	_special_	JP
string	rsl		Job RSL sent to Globus

string	reason		Reason of being in this status, if any
	_special_	JP

string	location	Where the job is being processed
	_special_	JP
_index_
string	ce_node		Worker node where the job is executed
	_special_	JP
string	network_server	Network server handling the job
	_special_	JP

bool	subjob_failed	Subjob failed (the parent job will fail too)
int	done_code		Return code
	_null_	-1
	_code_	OK		Finished correctly
	_code_	FAILED		Execution failed
	_code_	CANCELLED	Cancelled by user
	_special_	JP
int	exit_code		Unix exit code
	_special_	JP
bool	resubmitted	Resubmit count (the job was resubmitted if non-zero)
	_special_	JP

bool	cancelling	Cancellation request in progress
string	cancelReason	Reason of cancel
	_special_	JP

int	cpuTime		Consumed CPU time
	_null_	-1
	_special_	JP

taglist	user_tags	List of pairs (user_tag, user_value)
	_special_	XMLstructured

timeval	stateEnterTime	When entered this status
timeval	lastUpdateTime	Last known event of the job

intlist	stateEnterTimes	When all previous states were entered
	_special_	XMLstructured

bool	expectUpdate	Some logged information has not arrived yet
string	expectFrom	Sources of the missing information
string	acl		ACL of the job
	_special_	JP

bool	payload_running	User payload started
strlist	possible_destinations	Possible job destinations
	_special_	XMLstructured
strlist possible_ce_nodes	CE nodes matching to possible_destinations
	_special_	XMLstructured

bool	suspended	Job is suspended
string	suspend_reason	Reason for the suspend

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
