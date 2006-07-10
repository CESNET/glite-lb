#!/bin/bash

numjobs=$1

# XXX - there must be better way to find stage
STAGEDIR=/home/michal/shared/egee/jra1-head/stage
. $STAGEDIR/sbin/perftest_common.sh

DEBUG=${DEBUG:-0}
# CONSUMER_ARGS=
# PERFTEST_COMPONENT=
# COMPONENT_ARGS=
#LOGJOBS_ARGS="" 

check_test_files || exit 1

COMM_ARGS="-s /tmp/interlogger.perftest --file-prefix=/tmp/perftest.log"

echo "-------------------------------------------"
echo "Logging test:"
echo "  - events sent through IPC and/or files"
echo "  - events discarded by IL immediately"
echo "-------------------------------------------"
echo "a) events sent only by IPC"
echo "b) events stored to files and sent by IPC"
echo ""
echo -e "\tavg_job \t big_job \t avg_dag \t big_dag"


PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-interlogd-perf-empty
CONSUMER_ARGS="-d $COMM_ARGS"

LOGJOBS_ARGS="--nofile $COMM_ARGS"
echo -n "a)"
run_test il $numjobs
print_result
LOGJOBS_ARGS=" $COMM_ARGS"
echo -n "b)"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*


# echo "--------------------------------"
# echo "Interlogger test:"
# echo "  - events sent through IPC only"
# echo "  - events discarded in IL"
# echo "--------------------------------"
# echo "a) disabled event parsing, the server address (jobid) is hardcoded"
# echo "b) disabled event synchronization from files"
# echo "c) disabled recovery thread"
# echo "d) lazy bkserver connection close"
# echo "e) normal operation"
# echo ""
# echo -e "\tavg_job \t big_job \t avg_dag \t big_dag"

# PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-interlogd-perf
# LOGJOBS_ARGS="--nofile $COMM_ARGS"

# CONSUMER_ARGS="-d --nosend --noparse $COMM_ARGS"
# echo -n "a)"
# run_test il $numjobs
# print_result

# CONSUMER_ARGS="-d --nosend --nosync $COMM_ARGS"
# echo -n "b)"
# run_test il $numjobs
# print_result

# CONSUMER_ARGS="-d --nosend --norecover $COMM_ARGS"
# echo -n "c)"
# run_test il $numjobs
# print_result

# echo "d)  this test is not yet implemented"

# CONSUMER_ARGS="-d --nosend $COMM_ARGS"
# echo -n "e)"
# run_test il $numjobs
# print_result

echo "-----------------------------------"
echo "Interlogger test:"
echo "  - events sent through IPC & files"
echo "  - events discarded in IL"
echo "-----------------------------------"
echo "a) disabled event parsing, the server address (jobid) is hardcoded"
echo "b) disabled event synchronization from files"
echo "c) disabled recovery thread"
echo "x) disabled sync and recovery"
echo "d) lazy bkserver connection close"
echo "e) normal operation"
echo ""
echo -e "\tavg_job \t big_job \t avg_dag \t big_dag"

PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-interlogd-perf
LOGJOBS_ARGS=" $COMM_ARGS"

CONSUMER_ARGS="-d --nosend --noparse $COMM_ARGS"
echo -n "a)"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

CONSUMER_ARGS="-d --nosend --nosync $COMM_ARGS"
echo -n "b)"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

CONSUMER_ARGS="-d --nosend --norecover $COMM_ARGS"
echo -n "c)"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

CONSUMER_ARGS="-d --nosend --nosync --norecover $COMM_ARGS"
echo -n "x)"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

echo "d)  this test is not applicable"

CONSUMER_ARGS="-d --nosend $COMM_ARGS"
echo -n "e)"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*


# echo "-------------------------------"
# echo "Interlogger test:"
# echo "  - events sent through IPC"
# echo "  - events consumed by empty BS"
# echo "-------------------------------"
# echo "a) disabled event parsing, the server address (jobid) is hardcoded"
# echo "b) disabled event synchronization from files"
# echo "c) disabled recovery thread"
# echo "d) lazy bkserver connection close"
# echo "e) normal operation"
# echo ""
# echo -e "\tavg_job \t big_job \t avg_dag \t big_dag"

# PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-bkserverd
# CONSUMER_ARGS="-d --perf-sink=1"
# PERFTEST_COMPONENT=$STAGEDIR/bin/glite-lb-interlogd-perf
# LOGJOBS_ARGS="--nofile $COMM_ARGS"


# COMPONENT_ARGS="-d  --noparse $COMM_ARGS"
# echo -n "a)"
# run_test il $numjobs
# print_result
# rm -f /tmp/perftest.log.*

# COMPONENT_ARGS="-d  --nosync $COMM_ARGS"
# echo -n "b)"
# run_test il $numjobs
# print_result
# rm -f /tmp/perftest.log.*

# COMPONENT_ARGS="-d  --norecover $COMM_ARGS"
# echo -n "c)"
# run_test il $numjobs
# print_result
# rm -f /tmp/perftest.log.*

# echo "d) this test is not yet implemented"

# COMPONENT_ARGS="-d $COMM_ARGS"
# echo -n "e)"
# run_test il $numjobs
# print_result
# rm -f /tmp/perftest.log.*


echo "-----------------------------------"
echo "Interlogger test:"
echo "  - events sent through IPC & files"
echo "  - events consumed by empty BS"
echo "-----------------------------------"
echo "a) disabled event parsing, the server address (jobid) is hardcoded"
echo "b) disabled event synchronization from files"
echo "c) disabled recovery thread"
echo "x) disabled sync and recovery"
echo "d) lazy bkserver connection close"
echo "e) normal operation"
echo ""
echo -e "\tavg_job \t big_job \t avg_dag \t big_dag"

PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-bkserverd
CONSUMER_ARGS="-d --perf-sink=1"
PERFTEST_COMPONENT=$STAGEDIR/bin/glite-lb-interlogd-perf
LOGJOBS_ARGS=" $COMM_ARGS"


COMPONENT_ARGS="-d  --noparse $COMM_ARGS"
echo -n "a)"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

COMPONENT_ARGS="-d  --nosync $COMM_ARGS"
echo -n "b)"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

COMPONENT_ARGS="-d  --norecover $COMM_ARGS"
echo -n "c)"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

COMPONENT_ARGS="-d  --nosync --norecover $COMM_ARGS"
echo -n "x)"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

COMPONENT_ARGS="-d  --lazy=10 --nosync --norecover $COMM_ARGS"
echo -n "d)"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

COMPONENT_ARGS="-d $COMM_ARGS"
echo -n "e)"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*


