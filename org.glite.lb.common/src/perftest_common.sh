#!/bin/bash
#
# Copyright (c) Members of the EGEE Collaboration. 2004-2010.
# See http://www.eu-egee.org/partners for details on the copyright holders.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# those files should be staged by invoking 
#   LB_PERF=1 make stage (or ant equivalent)
# in org.glite.lb.common

if [[ -z $JOB_FILE ]]
then
    JOB_FILE[0]=$STAGEDIR/examples/perftest/perf_simple_avg_events.log
    JOB_DESC[0]="avg_job"
    JOB_FILE[1]=$STAGEDIR/examples/perftest/perf_simple_max_events.log
    JOB_DESC[1]="big_job"
    JOB_FILE[2]=$STAGEDIR/examples/perftest/perf_dag_avg_events.log
    JOB_DESC[2]="avg_dag"
    JOB_FILE[3]=$STAGEDIR/examples/perftest/perf_dag_max_events.log
    JOB_DESC[3]="big_dag"
    JOB_FILE[4]=$STAGEDIR/examples/perftest/perf_collection_avg_events.log
    JOB_DESC[4]="collection"
fi

# path to the job event producer
LOGJOBS=${LOGJOBS:-$STAGEDIR/sbin/glite-lb-perftest_logjobs}

# some defaults for log files
CONSUMER_LOG=/tmp/perftest_${USER}_consumer.log
PRODUCER_LOG=/tmp/perftest_${USER}_producer.log
COMPONENT_LOG=/tmp/perftest_${USER}_component.log


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
    for file in ${JOB_FILE[*]} 
    do
	check_file_readable $file || return 1
    done
    check_file_executable $LOGJOBS 
}


get_result()
{
    tmpfile=`mktemp -p /tmp`
    grep -a PERFTEST $CONSUMER_LOG > $tmpfile
    . $tmpfile
    grep -a PERFTEST $PRODUCER_LOG > $tmpfile
    . $tmpfile
    rm $tmpfile
    [[ $DEBUG -gt 0 ]] && echo "Timestamps: from $PERFTEST_BEGIN_TIMESTAMP to $PERFTEST_END_TIMESTAMP"
    total=`echo "scale=6; $PERFTEST_END_TIMESTAMP - $PERFTEST_BEGIN_TIMESTAMP" | bc`
    PERFTEST_EVENT_THROUGHPUT=`echo "scale=6; $PERFTEST_NUM_JOBS * $PERFTEST_JOB_SIZE / $total" |bc`
    PERFTEST_JOB_THROUGPUT=`echo "scale=6; $PERFTEST_NUM_JOBS / $total" |bc`
    PERFTEST_DAY_JOB_THROUGHPUT=`echo "scale=6; t=$PERFTEST_NUM_JOBS / $total * 3600 * 24; scale=0; 2*t/2" |bc`
}

init_result()
{
    j=0
    while [[ $j -lt ${#JOB_FILE[*]} ]]
    do
	PERFTEST_THROUGHPUT[$j]=0
	PERFTEST_EV_THROUGHPUT[$j]=0
        j=$((j+1))
    done
}

print_result_header()
{
    for desc in ${JOB_DESC[*]}
    do
      printf " %14s " $desc
    done
    printf "\n"
    #echo -e "\tavg_job \t big_job \t avg_dag \t big_dag"
}

print_result()
{
    for res in ${PERFTEST_THROUGHPUT[*]}
    do
      printf " %14d " $res
    done
    printf " [jobs/day]\n"
}

#print_result_ev()
#{
#    printf " %16.6f  %16.6f  %16.6f  %16.6f\t [events/sec]\n" \
#    ${PERFTEST_EV_THROUGHPUT[0]} \
#    ${PERFTEST_EV_THROUGHPUT[1]} \
#    ${PERFTEST_EV_THROUGHPUT[2]} \
#    ${PERFTEST_EV_THROUGHPUT[3]} 
#    j=0
#    while [[ $j -lt 4 ]]
#    do
#        echo -e -n "\t ${PERFTEST_EV_THROUGHPUT[$j]}"  
#        j=$((j+1))
#    done
#    echo -e "\t [events/sec]"
#}

shutdown()
{
    [[ -z $1 ]] && return
    kill -0 $1 > /dev/null 2>&1 && kill $1
    if kill -0 $1 >/dev/null 2>&1
    then
	sleep 2
	kill -9 $1 >/dev/null 2>&1
    fi
}

# run_test dest numjobs [testname]
run_test()
{
    local i file lj_flags linesbefore linesafter CONSUMER_PID COMPONENT_PID
    rm -f $CONSUMER_LOG $PRODUCER_LOG $COMPONENT_LOG
    touch $CONSUMER_LOG $PRODUCER_LOG $COMPONENT_LOG
    if [[ $DEBUG -gt 1 ]]
    then 
	tail -f $CONSUMER_LOG &
	tail -f $PRODUCER_LOG &
	tail -f $COMPONENT_LOG &
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
	$PERFTEST_COMPONENT $COMPONENT_ARGS > $COMPONENT_LOG 2>&1 &
	COMPONENT_PID=$!
    fi
    # wait for components to come up
    sleep 2
    # feed the beast
    i=0
    for file in ${JOB_FILE[*]}
    do
	[[ $DEBUG -gt 0 ]] && echo -e "\n\nRunning test with input $file"
	savePERFTEST_NAME=$PERFTEST_NAME
	PERFTEST_NAME="${PERFTEST_NAME}_${JOB_DESC[i]}"
	linesbefore=`grep -a PERFTEST $CONSUMER_LOG|wc -l`
	$LOGJOBS $lj_flags -f $file >> $PRODUCER_LOG 2>&1
	linesafter=`grep -a PERFTEST $CONSUMER_LOG|wc -l`
	PERFTEST_NAME=$savePERFTEST_NAME
	# if there are no new lines in the log, give it some time
	[[ $linesbefore -eq $linesafter ]] && sleep 5
	linesafter=`grep -a PERFTEST $CONSUMER_LOG|wc -l`
	[[ $DEBUG -gt 0 ]] && echo "Lines before " $linesbefore ", after " $linesafter
	if [[ $linesbefore -eq $linesafter ]]
	then
	    [[ $DEBUG -gt 0 ]] && echo "Test failed - consumer did not report timestamp."
	    PERFTEST_THROUGHPUT[$i]=0
	    PERFTEST_EV_THROUGHPUT[$i]=0
	else
	    get_result
	    [[ $DEBUG -gt 0 ]] && echo Result: $PERFTEST_DAY_JOB_THROUGHPUT
	    PERFTEST_THROUGHPUT[$i]=$PERFTEST_DAY_JOB_THROUGHPUT
	    PERFTEST_EV_THROUGHPUT[$i]=$PERFTEST_EVENT_THROUGHPUT
	fi
	i=$(($i + 1))
    done
    # clean up
    mv $PRODUCER_LOG ${PRODUCER_LOG}.${PERFTEST_NAME}
    mv $CONSUMER_LOG ${CONSUMER_LOG}.${PERFTEST_NAME}
    mv $COMPONENT_LOG ${COMPONENT_LOG}.${PERFTEST_NAME}
    shutdown $COMPONENT_PID
    shutdown $CONSUMER_PID
    [[ $DEBUG -gt 1 ]] && killall tail
}


