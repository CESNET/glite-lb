# macro definition for SCHEDULED state

:ready:
# JobSubmition

-s JobController,-e DeQueued, --queue="queue name", --local_jobid="new jobId assigned by the receiving component"
-s JobController,-e Transfer,--destination="LRMS",--dest_host="destination hostname",--dest_instance="destination instance",--job="job description in receiver language",--result=OK,--reason="detailed description of transfer",--dest_jobid="destination internal jobid"

# LogMonitor

-s LogMonitor,-e Accepted, --from="JobController", --from_host="sending component hostname", --from_instance="sending component instance", --local_jobid="new jobId (Condor, Globus ...)"
-s LogMonitor,-e Transfer,--destination="LRMS",--dest_host="destination hostname",--dest_instance="destination instance",--job="job description in receiver language",--result=OK,--reason="detailed description of transfer",--dest_jobid="destination internal jobid"

