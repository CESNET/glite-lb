# one resubmission cycle

-s LogMonitor,-e EnQueued, --queue="wm_input.fl", --job="job description in receiver language", --result=OK, --reason="detailed description of transfer"

-s WorkloadManager,-e DeQueued, --queue="wm_input.fl", --local_jobid="WM_ID" 
-s WorkloadManager, -e Resubmission, --result=${RESUB:-WILLRESUB}, --reason="let's try again", --tag=none
-s WorkloadManager,-e HelperCall, --helper_name="name of the called component",--helper_params="parameters of the call", --src_role=CALLING
-s WorkloadManager,-e Match,--dest_id="${DESTINATION:-destination CE/queue}"
-s WorkloadManager,-e HelperReturn, --helper_name="name of the called component",--retval="returned data", --src_role=CALLING
-s WorkloadManager,-e EnQueued, --queue="wm_output.fl", --job="job description in receiver language", --result=OK, --reason="detailed description of transfer"

-s JobController,-e DeQueued, --queue="wm_output.fl", --local_jobid="JC_ID"
-s JobController,-e Transfer,--destination="LRMS",--dest_host="destination hostname",--dest_instance="destination instance",--job="job description in receiver language",--result=OK,--reason="detailed description of transfer",--dest_jobid="destination internal jobid"

-s LogMonitor,-e Accepted, --from="JobController", --from_host="sending component hostname", --from_instance="sending component instance", --local_jobid="new jobId (Condor, Globus ...)"
-s LogMonitor,-e Transfer,--destination="LRMS",--dest_host="destination hostname",--dest_instance="destination instance",--job="job description in receiver language",--result=OK,--reason="detailed description of transfer",--dest_jobid="destination internal jobid"

-s LogMonitor,-e Running,--node="${CE_NODE:-worker node}"

