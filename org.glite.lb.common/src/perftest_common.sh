#!/bin/bash

# those files should be staged by invoking 
#   LB_PERF=1 make stage (or ant equivalent)
# in org.glite.lb.common

JOB_AVG_SIMPLE=${JOB_AVG_SIMPLE:-$STAGEDIR/examples/perftest/perf_simple_avg_events.log}
JOB_MAX_SIMPLE=${JOB_MAX_SIMPLE:-$STAGEDIR/examples/perftest/perf_simple_max_events.log}
JOB_AVG_DAG=${JOB_AVG_DAG:-$STAGEDIR/examples/perftest/perf_dag_avg_events.log}
JOB_MAX_DAG=${JOB_MAX_DAG:-$STAGEDIR/examples/perftest/perf_dag_max_events.log}

# path to the job event producer
LOGJOBS=${LOGJOBS:-$STAGEDIR/sbin/glite-lb-perftest_logjobs}

# some defaults for log files
CONSUMER_LOG=/tmp/perftest_consumer.log
PRODUCER_LOG=/tmp/perftest_producer.log


check_file_readable()
{
    [[ $DEBUG -gt 0 ]] && echo -n "Looking up file $1..."
    [[ -r $1 ]]
    RV=$?
    [[ $DEBUG -gt 0 ]] && ( [[ $RV -eq 0 ]] && echo "OK" ||  echo "FAILED" )
    return $RV
}

check_file_executable()
{
    [[ $DEBUG -gt 0 ]] && echo -n "Looking up program $1..."
    [[ -x $1 ]]
    RV=$?
    [[ $DEBUG -gt 0 ]] && ( [[ $RV -eq 0 ]] && echo "OK" || echo "FAILED" )
    return $RV
}

check_test_files()
{
    check_file_readable $JOB_AVG_SIMPLE && \
    check_file_readable $JOB_MAX_SIMPLE && \
    check_file_readable $JOB_AVG_DAG && \
    check_file_readable $JOB_MAX_DAG && \
    check_file_executable $LOGJOBS 
}


get_result()
{
    tmpfile=`mktemp -p /tmp`
    grep PERFTEST $CONSUMER_LOG > $tmpfile
    . $tmpfile
    grep PERFTEST $PRODUCER_LOG > $tmpfile
    . $tmpfile
    rm $tmpfile
    [[ $DEBUG -gt 0 ]] && echo "Timestamps: from $PERFTEST_BEGIN_TIMESTAMP to $PERFTEST_END_TIMESTAMP"
    total=`echo "scale=6; $PERFTEST_END_TIMESTAMP - $PERFTEST_BEGIN_TIMESTAMP" | bc`
    PERFTEST_EVENT_THROUGHPUT=`echo "scale=6; $PERFTEST_NUM_JOBS * $PERFTEST_JOB_SIZE / $total" |bc`
    PERFTEST_JOB_THROUGPUT=`echo "scale=6; $PERFTEST_NUM_JOBS / $total" |bc`
    PERFTEST_DAY_JOB_THROUGHPUT=`echo "scale=6; $PERFTEST_NUM_JOBS / $total * 3600 * 24" |bc`
}

shutdown()
{
    [[ -z $1 ]] && return
    kill -0 $1 > /dev/null 2>&1 && kill $1
    if kill -0 $1 >/dev/null 2>&1
    then
	sleep 10
	kill -9 $1
    fi
}

# run_test dest numjobs [testname]
run_test()
{
    local i file lj_flags linesbefore linesafter CONSUMER_PID COMPONENT_PID
    rm -f $CONSUMER_LOG $PRODUCER_LOG
    touch $CONSUMER_LOG $PRODUCER_LOG
    if [[ $DEBUG -gt 1 ]]
    then 
	tail -f $CONSUMER_LOG &
	tail -f $PRODUCER_LOG &
    fi
    # set args to logjobs
    lj_flags="$LOGJOBS_ARGS -d $1 -n $2"
    [[ -n $3 ]] && lj_flags="$lj_flags -t $3"
    # There is always consumer - start it
    $PERFTEST_CONSUMER $CONSUMER_ARGS > $CONSUMER_LOG 2>&1 &
    CONSUMER_PID=$!
    sleep 2
    # Start component (if specified)
    if [[ -n $PERFTEST_COMPONENT ]]
    then
	$PERFTEST_COMPONENT $COMPONENT_ARGS &
	COMPONENT_PID=$!
    fi
    # wait for components to come up
    sleep 2
    # feed the beast
    i=0
    for file in $JOB_AVG_SIMPLE $JOB_MAX_SIMPLE $JOB_AVG_DAG $JOB_MAX_DAG
    do
	[[ $DEBUG -gt 0 ]] && echo -e "\n\nRunning test with input $file"
	linesbefore=`grep PERFTEST $CONSUMER_LOG|wc -l`
	$LOGJOBS $lj_flags -f $file >> $PRODUCER_LOG 2>&1
	linesafter=`grep PERFTEST $CONSUMER_LOG|wc -l`
	# if there are no new lines in the log, give it some time
	[[ $linesbefore -eq $linesafter ]] && sleep 5
	linesafter=`grep PERFTEST $CONSUMER_LOG|wc -l`
	if [[ $linesbefore -eq $linesafter ]]
	then
	    echo "Test failed - consumer did not report timestamp."
	else
	    get_result
	    [[ $DEBUG -gt 0 ]] && echo Result: $PERFTEST_DAY_JOB_THROUGHPUT
	    PERFTEST_THROUGHPUT[$i]=$PERFTEST_DAY_JOB_THROUGHPUT
	    PERFTEST_EVENT_THROUGHPUT[$i]=$PERFTEST_EVENT_THROUGHPUT
	fi
	i=$(($i + 1))
    done
    # clean up
    shutdown $COMPONENT_PID
    shutdown $CONSUMER_PID
    [[ $DEBUG -gt 1 ]] && killall tail
}


