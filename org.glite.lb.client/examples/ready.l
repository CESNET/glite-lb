# macro definition for WAITING state

:waiting:
# NetworkServer

-s NetworkServer,-e EnQueued, --queue="destination queue", --job="job description in receiver language", --result=OK, --reason="detailed description of transfer"
# -s NetworkServer,-e Transfer,--destination="WorkloadManager",--dest_host="destination hostname",--dest_instance="destination instance",--job="job description in receiver language",--result=OK,--reason="detailed description of transfer",--dest_jobid="destination internal jobid"

# WorkloadManager

-s WorkloadManager,-e DeQueued, --queue="queue name", --local_jobid="new jobId assigned by the receiving component" 
-s WorkloadManager,-e HelperCall, --helper_name="name of the called component",--helper_params="parameters of the call", --src_role=CALLING
#-s Helper,-e HelperCall, --helper_name="name of the called component",--helper_params="parameters of the call", --src_role=CALLED
#-s Helper,-e HelperReturn, --helper_name="name of the called component",--retval="returned data", --src_role=CALLED
-s WorkloadManager,-e Match,--dest_id="${DESTINATION:-destination CE/queue}"
-s WorkloadManager,-e HelperReturn, --helper_name="name of the called component",--retval="returned data", --src_role=CALLING
-s WorkloadManager,-e EnQueued, --queue="destination queue", --job="job description in receiver language", --result=OK, --reason="detailed description of transfer"

