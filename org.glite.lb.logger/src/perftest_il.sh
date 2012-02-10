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

numjobs=10
parallel=1

if [ -z "${GLITE_LOCATION}" ]; then
	# let's be in stage
	STAGEDIR=`pwd`/usr
else
	STAGEDIR=${GLITE_LOCATION}
fi

. $STAGEDIR/sbin/perftest_common.sh

DBNAME=${DBNAME:-lbserver20}
export LBDB=lbserver/@localhost:$DBNAME

DEBUG=${DEBUG:-0}
# CONSUMER_ARGS=
# PERFTEST_COMPONENT=
# COMPONENT_ARGS=
LOGJOBS_ARGS="-s /tmp/interlogger.perftest" 

check_test_files || exit 1

COMM_ARGS="-s /tmp/interlogger.perftest --file-prefix=/tmp/perftest.log"

#TEST_GROUP=
#TEST_VARIANT=

SILENT=0
while getopts "G:t:n:sp:" OPTION 
do
    case "$OPTION" in 
    "G") TEST_GROUP=$OPTARG
    ;;

    "t") TEST_VARIANT=$OPTARG
    ;;

    "n") numjobs=$OPTARG
    ;;

    "s") SILENT=1
    ;;

    "p") parallel=$OPTARG
    ;;

    esac
done


group_a () 
{
if [[ $SILENT -eq 0 ]]
then
echo "-------------------------------------------"
echo "Logging test:"
echo "  - events sent through IPC and/or files"
echo "  - events discarded by IL immediately"
echo "-------------------------------------------"
echo "a) events sent only by IPC"
echo "b) events stored to files and sent by IPC"
echo ""
fi

PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-interlogd-perf-empty
CONSUMER_ARGS="-i /tmp/perftest_il.pid -d $COMM_ARGS"
}

group_a_test_a () 
{
    LOGJOBS_ARGS="--nofile $COMM_ARGS"
    echo -n "a)"
    run_test il $numjobs
    print_result
}

group_a_test_b () {
    LOGJOBS_ARGS=" $COMM_ARGS"
    echo -n "b)"
    run_test il $numjobs
    print_result
    find /tmp -maxdepth 1 -name perftest.log.\* -exec rm -f \{\} \;
}



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


group_b () {
if [[ $SILENT -eq 0 ]]
then
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
fi
PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-interlogd-perf
LOGJOBS_ARGS=" $COMM_ARGS"
}

group_b_test_a ()
{
    CONSUMER_ARGS="-i /tmp/perftest_il.pid -d --nosend --noparse $COMM_ARGS"
    echo -n "a)"
    run_test il $numjobs
    print_result
    find /tmp -maxdepth 1 -name perftest.log.\* -exec rm \{\} \;
}

group_b_test_b () 
{
    CONSUMER_ARGS="-i /tmp/perftest_il.pid -d --nosend --nosync $COMM_ARGS"
    echo -n "b)"
    run_test il $numjobs
    print_result
    find /tmp -maxdepth 1 -name perftest.log.\* -exec rm \{\} \;
}

group_b_test_c () 
{
    CONSUMER_ARGS="-i /tmp/perftest_il.pid -d --nosend --norecover $COMM_ARGS"
    echo -n "c)"
    run_test il $numjobs
    print_result
    find /tmp -maxdepth 1 -name perftest.log.\* -exec rm \{\} \;
}

group_b_test_x ()
{
    CONSUMER_ARGS="-i /tmp/perftest_il.pid -d --nosend --nosync --norecover $COMM_ARGS"
    echo -n "x)"
    run_test il $numjobs
    print_result
    find /tmp -maxdepth 1 -name perftest.log.\* -exec rm \{\} \;
}

group_b_test_d ()
{
    echo "d)  this test is not applicable"
}

group_b_test_e ()
{
    CONSUMER_ARGS="-i /tmp/perftest_il.pid -d --nosend $COMM_ARGS"
    echo -n "e)"
    run_test il $numjobs
    print_result
    find /tmp -maxdepth 1 -name perftest.log.\* -exec rm \{\} \;
}

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


group_c () 
{
if [[ $SILENT -eq 0 ]]
then
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
fi

PERFTEST_CONSUMER=$STAGEDIR/bin/glite-lb-bkserverd
CONSUMER_ARGS="-g -S /tmp -D /tmp -t 1 -d --perf-sink=1 -p 10500 -w 10503"
PERFTEST_COMPONENT=$STAGEDIR/bin/glite-lb-interlogd-perf
LOGJOBS_ARGS=" -m localhost:10500 $COMM_ARGS"
}

group_c_test_a ()
{
    COMPONENT_ARGS="-i /tmp/perftest_il.pid -d  --noparse $COMM_ARGS"
    echo -n "a)"
    run_test il $numjobs
    print_result
    find /tmp -maxdepth 1 -name perftest.log.\* -exec rm \{\} \;
}

group_c_test_b ()
{
    COMPONENT_ARGS="-i /tmp/perftest_il.pid -d  --nosync $COMM_ARGS"
    echo -n "b)"
    run_test il $numjobs
    print_result
    find /tmp -maxdepth 1 -name perftest.log.\* -exec rm \{\} \;
}

group_c_test_c ()
{
    COMPONENT_ARGS="-i /tmp/perftest_il.pid -d  --norecover $COMM_ARGS"
    echo -n "c)"
    run_test il $numjobs
    print_result
    find /tmp -maxdepth 1 -name perftest.log.\* -exec rm \{\} \;
}

group_c_test_x () 
{
    COMPONENT_ARGS="-i /tmp/perftest_il.pid -d  --nosync --norecover $COMM_ARGS"
    echo -n "x)"
    run_test il $numjobs
    print_result
    find /tmp -maxdepth 1 -name perftest.log.\* -exec rm \{\} \;
}

group_c_test_d ()
{
    COMPONENT_ARGS="-p$parallel -i /tmp/perftest_il.pid -d  --lazy=10 --nosync --norecover $COMM_ARGS"
    echo -n "d)"
    run_test il $numjobs
    print_result
    find /tmp -maxdepth 1 -name perftest.log.\* -exec rm \{\} \;
}

group_c_test_e ()
{
    COMPONENT_ARGS="-p$parallel -i /tmp/perftest_il.pid -d $COMM_ARGS"
    echo -n "e)"
    run_test il $numjobs
    print_result
    find /tmp -maxdepth 1 -name perftest.log.\* -exec rm \{\} \;
}

   
if [[ $SILENT -eq 0 ]]
then
    while [[ -z $TEST_GROUP ]]
    do
	echo "Choose test group:"
	echo "  a) logging source tests"
	echo "  b) interlogger tests"
	echo "  c) interlogger with external consumer tests"
	echo -n "Your choice: "
	read TEST_GROUP
    done
fi
TEST_GROUP=`echo $TEST_GROUP | tr '[A-Z]' '[a-z]'`

if [[ "x$TEST_GROUP" = "x*" ]]
then
    TEST_GROUP="a b c"
fi

for group in $TEST_GROUP
do
    group_$group

    if [[ $SILENT -eq 0 ]]
    then
	while [[ -z $TEST_VARIANT ]]
	do
	    echo -n "Your choice: "
	    read -e TEST_VARIANT
	done
	print_result_header
    fi

    if [[ "x$TEST_VARIANT" = "x*" ]]
    then
	case $TEST_GROUP in
	    "a") TEST_VARIANT="a b" ;;
	    *)	 TEST_VARIANT="a b c x d e" ;;
	esac
    fi

    for variant in $TEST_VARIANT
    do
    	export PERFTEST_NAME="il_${group}${variant}"
	group_${group}_test_${variant}
    done
done

exit;

