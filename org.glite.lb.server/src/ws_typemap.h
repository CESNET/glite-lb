#if GSOAP_VERSION >= 20700

#define UNDEF		edgwll__QueryAttr__UNDEF

#define JOBID		edgwll__QueryAttr__JOBID
#define OWNER		edgwll__QueryAttr__OWNER
#define STATUS		edgwll__QueryAttr__STATUS
#define LOCATION	edgwll__QueryAttr__LOCATION
#define DESTINATION	edgwll__QueryAttr__DESTINATION
#define DONECODE	edgwll__QueryAttr__DONECODE
#define USERTAG		edgwll__QueryAttr__USERTAG
#define TIME		edgwll__QueryAttr__TIME
#define LEVEL		edgwll__QueryAttr__LEVEL
#define HOST		edgwll__QueryAttr__HOST
#define SOURCE		edgwll__QueryAttr__SOURCE
#define INSTANCE	edgwll__QueryAttr__INSTANCE
#define EVENT_TYPE	edgwll__QueryAttr__EVENT_TYPE
#define CHKPT_TAG	edgwll__QueryAttr__CHKPT_TAG
#define RESUBMITTED	edgwll__QueryAttr__RESUBMITTED
#define PARENT		edgwll__QueryAttr__PARENT
#define EXITCODE	edgwll__QueryAttr__EXITCODE

#define EQUAL		edgwll__QueryOp__EQUAL
#define UNEQUAL		edgwll__QueryOp__UNEQUAL
#define LESS		edgwll__QueryOp__LESS
#define GREATER		edgwll__QueryOp__GREATER
#define WITHIN		edgwll__QueryOp__WITHIN

#define SUBMITTED	edgwll__JobStatCode__SUBMITTED
#define WAITING		edgwll__JobStatCode__WAITING
#define READY		edgwll__JobStatCode__READY
#define SCHEDULED	edgwll__JobStatCode__SCHEDULED
#define RUNNING		edgwll__JobStatCode__RUNNING
#define DONE		edgwll__JobStatCode__DONE
#define CLEARED		edgwll__JobStatCode__CLEARED
#define ABORTED		edgwll__JobStatCode__ABORTED
#define CANCELLED	edgwll__JobStatCode__CANCELLED
#define UNKNOWN		edgwll__JobStatCode__UNKNOWN
#define PURGED		edgwll__JobStatCode__PURGED

#define SIMPLE		edgwll__StatJobType__SIMPLE
#define DAG		edgwll__StatJobType__DAG

#define OK		edgwll__StatDoneCode__OK
#define FAILED		edgwll__StatDoneCode__FAILED
#define CANCELLED_	edgwll__StatDoneCode__CANCELLED

#define CLASSADS	edgwll__JobStatFlag__CLASSADS
#define CHILDREN	edgwll__JobStatFlag__CHILDREN
#define CHILDSTAT	edgwll__JobStatFlag__CHILDSTAT

#endif
