@type _common_
jobid	jobId		Id of the job
string	owner		Job owner
_index_

int	jobtype		Type of job
	_null_	-1
	_code_ SIMPLE	simple job
	_code_ DAG	composite job
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

@type Submitted		entered by the user to the User Interface or registered by Job Partitioner
@type Waiting		Accepted by WMS, waiting for resource allocation
@type Ready		Matching resources found
@type Scheduled		Accepted by LRMS queue
@type Running		Executable is running
@type Done		Execution finished, output is available
@type Cleared		Output transfered back to user and freed
@type Aborted		Aborted by system (at any stage)
@type Cancelled		Cancelled by user
@type Unknown		Status cannot be determined
@type Purged		Job has been purged from bookkeeping server (for LB->RGMA interface)
