# macro definition for WAITING state

:waiting_dag:
# NetworkServer

-s NetworkServer,-e EnQueued, --queue="destination queue", --job="job description in receiver language", --result=OK, --reason="detailed description of transfer"

# WorkloadManager

-s WorkloadManager,-e DeQueued, --queue="queue name", --local_jobid="new jobId assigned by the receiving component" 

-s WorkloadManager,-e EnQueued, --queue="destination queue", --job="job description in receiver language", --result=OK, --reason="detailed description of transfer"

