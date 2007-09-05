# macro definition for SCHEDULED state

:ready_subjob:
# LogMonitor

-s LogMonitor,-e Transfer,--destination="LRMS",--dest_host="destination hostname",--dest_instance="destination instance",--job="job description in receiver language",--result=OK,--reason="detailed description of transfer",--dest_jobid="destination internal jobid"

