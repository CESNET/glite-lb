#!/bin/bash

numjobs=${1:-1}

# XXX - there must be better way to find stage
if [ -z "${GLITE_LOCATION}" ]; then
	STAGEDIR=/home/michal/shared/egee/jra1-head/stage
else
	STAGEDIR=${GLITE_LOCATION}
fi

. $STAGEDIR/sbin/perftest_common.sh

DEBUG=${DEBUG:-0}
# CONSUMER_ARGS=
# PERFTEST_COMPONENT=
# COMPONENT_ARGS=
# LOGJOBS_ARGS="" 
COMM_ARGS="--file-prefix /tmp/perftest.log -p 45678"
export EDG_WL_LOG_DESTINATION="localhost:45678"

check_test_files || exit 1

echo "----------------
Locallogger test
----------------
a) glite-lb-logd-perf-nofile --noParse --noIPC
b) glite-lb-logd-perf-nofile --noIPC
c) glite-lb-logd-perf --noIPC
d) glite-lb-logd-perf

Number of jobs: $numjobs
"
echo -e "\tavg_job \t big_job \t avg_dag \t big_dag"

# a)
echo -n "a)"
PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-logd-perf-nofile
CONSUMER_ARGS="-d --noIPC --noParse $COMM_ARGS"
init_result
run_test ll $numjobs
#print_result_ev
print_result

# b)
echo -n "b)"
PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-logd-perf-nofile
CONSUMER_ARGS="-d --noIPC $COMM_ARGS"
init_result
run_test ll $numjobs
#print_result_ev
print_result

# c)
echo -n "c)"
PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-logd-perf
CONSUMER_ARGS="-d --noIPC $COMM_ARGS"
init_result
run_test ll $numjobs
#print_result_ev
print_result

rm -f /tmp/perftest.log.*

# d)
echo -n "d)"
PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-interlogd-perf-empty
CONSUMER_ARGS="-d -s /tmp/perftest.sock"
PERFTEST_COMPONENT=$STAGEDIR/bin/glite-lb-logd-perf
COMPONENT_ARGS="-d -s /tmp/perftest.sock $COMM_ARGS"
init_result
run_test ll $numjobs
#print_result_ev
print_result

rm -f /tmp/perftest.log.*
