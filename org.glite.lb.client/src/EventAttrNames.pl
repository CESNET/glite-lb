#
# Copyright (c) Members of the EGEE Collaboration. 2004-2010.
# See http://www.eu-egee.org/partners for details on the copyright holders.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# The order of strings in this array determines assigned numeric value in the Event::Attr enum.
# It must not be changed unless API major number is incremented

@main::EventAttrNames = qw/
	ARRIVED
	CHILD
	CHILD_EVENT
	CLASSAD
	DESCR
	DEST_HOST
	DEST_ID
	DEST_INSTANCE
	DEST_JOBID
	DEST_PORT
	DESTINATION
	DONE_CODE
	ERROR_DESC
	EXIT_CODE
	EXIT_STATUS
	FROM
	FROM_HOST
	FROM_INSTANCE
	HELPER_NAME
	HELPER_PARAMS
	HISTOGRAM
	HOST
	JDL
	JOB
	JOBID
	JOB_EXIT_STATUS
	JOB_PID
	JOBSTAT
	JOBTYPE
	LEVEL
	LOCAL_JOBID
	NAME
	NODE
	NOTIFID
	NS
	NSUBJOBS
	OPERATION
	OWNER
	PARENT
	PERMISSION
	PERMISSION_TYPE
	PID
	PREEMPTING
	PRIORITY
	QUANTITY
	QUEUE
	REASON
	RESOURCE
	RESULT
	RETVAL
	SCHEDULER
	SEED
	SEQCODE
	SHADOW_EXIT_STATUS
	SHADOW_HOST
	SHADOW_PID
	SHADOW_PORT
	SHADOW_STATUS
	SOURCE
	SRC_INSTANCE
	SRC_ROLE
	STARTER_EXIT_STATUS
	STARTER_PID
	STATE
	STATUS_CODE
	SVC_HOST
	SVC_NAME
	SVC_PORT
	TAG
	TIMESTAMP
	UNIT
	UNIVERSE
	USAGE
	USER
	USER_ID
	USER_ID_TYPE
	VALUE
	WN_SEQ
	EXPIRES
	COMMAND
	CMDID
	CALLEE
	DESTID
	NEW_STATE
	OLD_STATE
	ORIG_TIMESTAMP
	SRC
	DEST
	SANDBOX_TYPE
	TRANSFER_JOB
	COMPUTE_JOB
	DEST_URL
	LRMS_JOBID
	WORKER_NODE
	FAILURE_REASON
	WMS_DN
	PAYLOAD_OWNER
	NODE_ID
	NODE_INSTANCE
/;
