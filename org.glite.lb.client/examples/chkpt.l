# macro definition for DONE state

:running:
-s LRMS, -e Chkpt, --tag=chkpt1, --classad=""
-s LRMS, -e Chkpt, --tag=chkpt2, --classad=""
-s LogMonitor, -e Done, --status_code=OK, --reason="reason for the change", --exit_code=0 
