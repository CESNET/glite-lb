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

echo -e "\tsmall_job \t big_job \t small_dag \t big_dag"


PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-interlogd-perf-empty
CONSUMER_ARGS="-d $COMM_ARGS"

LOGJOBS_ARGS="--nofile $COMM_ARGS"
echo "Only IPC"
run_test il $numjobs
print_result
LOGJOBS_ARGS=" $COMM_ARGS"
echo "IPC & files"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*


echo "--------------------------------"
echo "Interlogger test:"
echo "  - events sent through IPC only"
echo "  - events discarded in IL"
echo "--------------------------------"
echo -e "\tsmall_job \t big_job \t small_dag \t big_dag"

PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-interlogd-perf
LOGJOBS_ARGS="--nofile $COMM_ARGS"

CONSUMER_ARGS="-d --nosend --noparse $COMM_ARGS"
echo "No event parsing"
run_test il $numjobs
print_result

CONSUMER_ARGS="-d --nosend --nosync $COMM_ARGS"
echo "No checking of event files"
run_test il $numjobs
print_result

CONSUMER_ARGS="-d --nosend --norecover $COMM_ARGS"
echo "No recovery thread"
run_test il $numjobs
print_result

CONSUMER_ARGS="-d --nosend $COMM_ARGS"
echo "Normal operation:"
run_test il $numjobs
print_result

echo "-----------------------------------"
echo "Interlogger test:"
echo "  - events sent through IPC & files"
echo "  - events discarded in IL"
echo "-----------------------------------"
echo -e "\tsmall_job \t big_job \t small_dag \t big_dag"

PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-interlogd-perf
LOGJOBS_ARGS=" $COMM_ARGS"

CONSUMER_ARGS="-d --nosend --noparse $COMM_ARGS"
echo "No event parsing"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

CONSUMER_ARGS="-d --nosend --nosync $COMM_ARGS"
echo "No checking of event files"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

CONSUMER_ARGS="-d --nosend --norecover $COMM_ARGS"
echo "No recovery thread"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

CONSUMER_ARGS="-d --nosend $COMM_ARGS"
echo "Normal operation:"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*


echo "-------------------------------"
echo "Interlogger test:"
echo "  - events sent through IPC"
echo "  - events consumed by empty BS"
echo "-------------------------------"

PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-bkserverd
CONSUMER_ARGS="-d --perf-sink=1"
PERFTEST_COMPONENT=$STAGEDIR/bin/glite-lb-interlogd-perf
LOGJOBS_ARGS="--nofile $COMM_ARGS"


COMPONENT_ARGS="-d  --noparse $COMM_ARGS"
echo "No event parsing"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

COMPONENT_ARGS="-d  --nosync $COMM_ARGS"
echo "No checking of event files"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

COMPONENT_ARGS="-d  --norecover $COMM_ARGS"
echo "No recovery thread"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

COMPONENT_ARGS="-d $COMM_ARGS"
echo "Normal operation:"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*


echo "-----------------------------------"
echo "Interlogger test:"
echo "  - events sent through IPC & files"
echo "  - events consumed by empty BS"
echo "-----------------------------------"

PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-bkserverd
CONSUMER_ARGS="-d --perf-sink=1"
PERFTEST_COMPONENT=$STAGEDIR/bin/glite-lb-interlogd-perf
LOGJOBS_ARGS=" $COMM_ARGS"


COMPONENT_ARGS="-d  --noparse $COMM_ARGS"
echo "No event parsing"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

COMPONENT_ARGS="-d  --nosync $COMM_ARGS"
echo "No checking of event files"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

COMPONENT_ARGS="-d  --norecover $COMM_ARGS"
echo "No recovery thread"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*

COMPONENT_ARGS="-d $COMM_ARGS"
echo "Normal operation:"
run_test il $numjobs
print_result
rm -f /tmp/perftest.log.*


