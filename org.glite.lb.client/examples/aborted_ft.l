# macro definition for RUNNING state

:running_ft:
-s Application, -e FileTransfer, --result=FAIL, --reason="problem with transfer"
-s JobController,-e Abort,--reason "just to test"
