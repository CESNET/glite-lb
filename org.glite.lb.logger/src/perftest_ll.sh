#!/bin/bash

numjobs=$1

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
#LOGJOBS_ARGS="" 

check_test_files || exit 1

PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-logd-perf-nofile
CONSUMER_ARGS="-d -v --noIPC --noParse"

echo -e "\tsmall_job \t big_job \t small_dag \t big_dag"
run_test ll $numjobs
j=0
while [[ $j -lt 4 ]]
do
    echo -e -n "\t ${PERFTEST_EVENT_THROUGHPUT[$j]}"	
    j=$((j+1))
done
echo ""

