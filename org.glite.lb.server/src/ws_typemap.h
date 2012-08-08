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

#ifndef GLITE_LB_WS_TYPEMAP_H
#define GLITE_LB_WS_TYPEMAP_H

#ident "$Header"

#if GSOAP_VERSION >= 20700

#define JOBID		lbt__queryAttr__JOBID
#define OWNER		lbt__queryAttr__OWNER
#define STATUS		lbt__queryAttr__STATUS
#define LOCATION	lbt__queryAttr__LOCATION
#define DESTINATION	lbt__queryAttr__DESTINATION
#define DONECODE	lbt__queryAttr__DONECODE
#define USERTAG		lbt__queryAttr__USERTAG
#define TIME		lbt__queryAttr__TIME
#define LEVEL		lbt__queryAttr__LEVEL
#define HOST		lbt__queryAttr__HOST
#define SOURCE		lbt__queryAttr__SOURCE
#define INSTANCE	lbt__queryAttr__INSTANCE
#define EVENTTYPE	lbt__queryAttr__EVENTTYPE
#define CHKPTTAG	lbt__queryAttr__CHKPTTAG
#define RESUBMITTED	lbt__queryAttr__RESUBMITTED
#define PARENT		lbt__queryAttr__PARENT
#define EXITCODE	lbt__queryAttr__EXITCODE
#define JDLATTR		lbt__queryAttr__JDLATTR
#define STATEENTERTIME	lbt__queryAttr__STATEENTERTIME
#define LASTUPDATETIME	lbt__queryAttr__LASTUPDATETIME
#define NETWORKSERVER	lbt__queryAttr__NETWORKSERVER
#define JOBTYPE		lbt__queryAttr__JOBTYPE

#define EQUAL		lbt__queryOp__EQUAL
#define UNEQUAL		lbt__queryOp__UNEQUAL
#define LESS		lbt__queryOp__LESS
#define GREATER		lbt__queryOp__GREATER
#define WITHIN		lbt__queryOp__WITHIN
#define CHANGED		lbt__queryOp__CHANGED

#define SUBMITTED	lbt__statName__SUBMITTED
#define WAITING		lbt__statName__WAITING
#define READY		lbt__statName__READY
#define SCHEDULED	lbt__statName__SCHEDULED
#define RUNNING		lbt__statName__RUNNING
#define DONE		lbt__statName__DONE
#define CLEARED		lbt__statName__CLEARED
#define ABORTED		lbt__statName__ABORTED
#define CANCELLED	lbt__statName__CANCELLED
#define UNKNOWN		lbt__statName__UNKNOWN
#define PURGED		lbt__statName__PURGED

#define SIMPLE		lbt__jobtype__SIMPLE
#define DAG		lbt__jobtype__DAG

#define OK		lbt__doneCode__OK
#define FAILED		lbt__doneCode__FAILED
#define CANCELLED_	lbt__doneCode__CANCELLED

#define CLASSADS	lbt__jobFlagsValue__CLASSADS
#define CHILDREN	lbt__jobFlagsValue__CHILDREN
#define CHILDSTAT	lbt__jobFlagsValue__CHILDSTAT
#define CHILDHIST_FAST lbt__jobFlagsValue__CHILDHIST_USCOREFAST
#define CHILDHIST_THOROUGH lbt__jobFlagsValue__CHILDHIST_USCORETHOROUGH

#define UserInterface	lbt__eventSource__UserInterface
#define NetworkServer	lbt__eventSource__NetworkServer
#define WorkloadManager	lbt__eventSource__WorkloadManager
#define BigHelper	lbt__eventSource__BigHelper
#define JobSubmission	lbt__eventSource__JobSubmission
#define LogMonitor	lbt__eventSource__LogMonitor
#define LRMS		lbt__eventSource__LRMS
#define Application	lbt__eventSource__Application
#define LBServer	lbt__eventSource__LBServer
#define CREAMInterface	lbt__eventSource__CREAMInterface
#define CREAMExecutor	lbt__eventSource__CREAMExecutor
#define PBSClient	lbt__eventSource__PBSClient
#define PBSServer	lbt__eventSource__PBSServer
#define PBSsMom		lbt__eventSource__PBSsMom
#define PBSMom		lbt__eventSource__PBSMom
#define PBSScheduler	lbt__eventSource__PBSScheduler

#endif

#endif /* GLITE_LB_WS_TYPEMAP_H */
