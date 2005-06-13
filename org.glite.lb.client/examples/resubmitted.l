# resubmitted job example

! DESTINATION=CE1/queue1
! CE_NODE=worker1
:running:

-s LogMonitor, -e Done, --status_code=FAILED, --reason="reason for the change", --exit_code=141
-s LogMonitor, -e Resubmission, --result=WILLRESUB, --reason="let's try again", --tag=none

! DESTINATION=CE2/queue2
! CE_NODE=worker2
:resubmission:

-s LogMonitor, -e Done, --status_code=FAILED, --reason="reason for the change", --exit_code=141
-s LogMonitor, -e Resubmission, --result=WILLRESUB, --reason="let's try again", --tag=none

! DESTINATION=CE3/queue3
! CE_NODE=worker3
:resubmission:

