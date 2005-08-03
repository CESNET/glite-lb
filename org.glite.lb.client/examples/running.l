# macro definition for RUNNING state

:scheduled:
-s LogMonitor,-e Running,--node="${CE_NODE:-worker node}"
