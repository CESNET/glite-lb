# shallow resubmission job example

! DESTINATION=CE1/queue1
! CE_NODE=worker1
:running:

-s LogMonitor, -e Done, --status_code=FAILED, --reason="reason for the change", --exit_code=141
-s LogMonitor, -e Resubmission, --result=WILLRESUB, --reason="let's try again", --tag=none

! DESTINATION=CE2/queue2
! CE_NODE=worker2
! RESUB=SHALLOW
:resubmission:

-s LogMonitor, -e Done, --status_code=FAILED, --reason="reason for the change", --exit_code=141
-s LogMonitor, -e Resubmission, --result=WILLRESUB, --reason="let's try again", --tag=none

! DESTINATION=CE3/queue3
! CE_NODE=worker3
! RESUB=SHALLOW
:resubmission:

-s LogMonitor, -e ReallyRunning, --wn_seq="$EDG_WL_SEQUENCE"

# deep resubmission

-s LogMonitor, -e Done, --status_code=FAILED, --reason="reason for the change", --exit_code=141
-s LogMonitor, -e Resubmission, --result=WILLRESUB, --reason="let's try again", --tag=none

! DESTINATION=CE2/queue2
! CE_NODE=worker2
! RESUB=DEEP
:resubmission:

#       payload_running=false
#       destination=CE2/queue2
#       ce_node=worker2
#       possible_destinations=CE2/queue2 
#       possible_ce_nodes=worker2

