#!/bin/bash

numjobs=10

# XXX - there must be better way to find stage
STAGEDIR=/home/michal/shared/egee/jra1-head/stage
. $STAGEDIR/sbin/perftest_common.sh

LOGEVENT=${LOGEVENT:-$STAGEDIR/bin/glite-lb-logevent}

DEBUG=${DEBUG:-0}

SILENT=0
while getopts "t:n:s" OPTION 
do
    case "$OPTION" in 
    "t") TEST_VARIANT=$OPTARG
    ;;

    "n") numjobs=$OPTARG
    ;;

    "s") SILENT=1
    ;;

    esac
done

# CONSUMER_ARGS=
# PERFTEST_COMPONENT=
# COMPONENT_ARGS=
#LOGJOBS_ARGS="" 

check_test_files || exit 1
check_file_executable $LOGEVENT || exit 1

SEQCODE="UI=999990:NS=9999999990:WM=999990:BH=9999999990:JSS=999990:LM=999990:LRMS=999990:APP=999990"

purge_proxy ()
{
    for jobid in $@
    do
	$LOGEVENT -x -S /tmp/proxy.perfstore.sock -c $SEQCODE -j $jobid -s UserInterface -e Abort --reason Purge > /dev/null 2>&1
    done
}

group_a () {
echo "----------------------------------"
echo "LB Proxy test"
echo "----------------------------------"
echo "Events are consumed:"
echo "1) before parsing"
echo "2) after parsing, before storing into database"
echo "3) after storing into db, before computing state"
echo "4) after computing state, before sending to IL"
echo "5) by IL"
echo ""
LOGJOBS_ARGS="-s /tmp/proxy.perf"
}

echo -e "\tavg_job \t big_job \t avg_dag \t big_dag"


group_a_test_n () 
{
    PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-proxy
    i=$1
    CONSUMER_ARGS="-d --perf-sink $i -p /tmp/proxy.perf" 
    export PERFTEST_NAME="proxy_test_$i"
    echo -n "${i})"
    run_test proxy $numjobs
    print_result
    # purge jobs from database
    # we have to start proxy again 
    $PERFTEST_CONSUMER -d -p /tmp/proxy.perf -s 1 >/dev/null 2>&1  &
    PID=$!
    purge_proxy `$LOGJOBS -n $numjobs`
    sleep 2
    shutdown $PID
}

group_a_test_5 ()
{
    PERFTEST_COMPONENT="$STAGEDIR/bin/glite-lb-proxy"
    COMPONENT_ARGS="-d -p /tmp/proxy.perf --proxy-il-sock /tmp/interlogger.perf  --proxy-il-fprefix /tmp/perftest.log"

    PERFTEST_CONSUMER="$STAGEDIR/bin/glite-lb-interlogd-perf-empty"
    CONSUMER_ARGS="-d -s /tmp/interlogger.perf --file-prefix=/tmp/perftest.log"
    export PERFTEST_NAME="proxy_test_5"
    echo -n "5)"
    run_test proxy $numjobs
    print_result
    $PERFTEST_COMPONENT -d -p /tmp/proxy.perf -s 1 >/dev/null 2>&1  &
    PID=$!
    purge_proxy `$LOGJOBS -n $numjobs`
    sleep 2
    shutdown $PID
    rm -f /tmp/perftest.log.*
}

group="a"

group_$group

if [[ $SILENT -eq 0 ]]
then
    while [[ -z $TEST_VARIANT ]]
    do
	echo -n "Your choice: "
	read -e TEST_VARIANT
    done
    echo -e "\tavg_job \t big_job \t avg_dag \t big_dag"
fi

if [[ "x$TEST_VARIANT" = "x*" ]]
then
   TEST_VARIANT="1 2 3 4 5"
fi

for variant in $TEST_VARIANT
do
    if [[ "$variant" = "5" ]]
    then
	group_${group}_test_${variant}
    else
	group_${group}_test_n $variant
    fi
done
