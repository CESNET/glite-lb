# macro definition for WAITING state

:waiting_subjob:
# WorkloadManager

-s WorkloadManager,-e Match,--dest_id="Id of the destination CE/queue"
-s WorkloadManager,-e HelperReturn, --helper_name="name of the called component",--retval="returned data", --src_role=CALLING

