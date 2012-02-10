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

# XXX - there must be better way to find stage
STAGEDIR=$GLITE_LOCATION
. $GLITE_LOCATION/sbin/perftest_common.sh

LOGEVENT=${LOGEVENT:-$GLITE_LOCATION/bin/glite-lb-logevent}

DEBUG=${DEBUG:-0}

DBNAME=${DBNAME:-lbserver20}
export LBDB=lbserver/@localhost:$DBNAME

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
      cat <<EOF
UPDATE acls, jobs SET acls.refcnt = acls.refcnt - 1 WHERE jobs.dg_jobid="$jobid" AND jobs.aclid=acls.aclid;
DELETE FROM jobs,states,e,ef,short_fields,long_fields,status_tags USING states  LEFT JOIN events AS e ON (e.jobid = states.jobid) LEFT JOIN events_flesh AS ef ON (ef.jobid = states.jobid) LEFT JOIN jobs ON (jobs.jobid = states.jobid) LEFT JOIN short_fields ON (short_fields.jobid = e.jobid AND short_fields.event = e.event) LEFT JOIN long_fields ON (long_fields.jobid = e.jobid AND long_fields.event = e.event) LEFT JOIN status_tags ON (status_tags.jobid = states.jobid) WHERE jobs.dg_jobid="$jobid";

EOF
#$LOGEVENT -x -S /tmp/proxy.perfstore.sock -c $SEQCODE -j $jobid -s UserInterface -e Abort --reason Purge > /dev/null 2>&1
    done | mysql -u lbserver $DBNAME
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
echo "5) by IL on input socket"
echo "6) by IL at delivery"
echo ""
LOGJOBS_ARGS="-s /tmp/proxy.perf"
}

echo -e "\tavg_job \t big_job \t avg_dag \t big_dag"


group_a_test_n () 
{
    PERFTEST_CONSUMER=$GLITE_LOCATION/bin/glite-lb-bkserverd
    i=$1
    CONSUMER_ARGS="-P -d --perf-sink $i -o /tmp/proxy.perf -D /tmp -t 1 " 
    export PERFTEST_NAME="proxy_test_$i"
    echo -n "${i})"
    run_test proxy $numjobs
    print_result
    # purge jobs from database
    # we have to start proxy again 
    #$PERFTEST_CONSUMER -P -d -o /tmp/proxy.perf -D /tmp -t 1 >/dev/null 2>&1  &
    #PID=$!
    purge_proxy `i=0; for file in ${JOB_FILE[*]}; do sPN=$PERFTEST_NAME; PERFTEST_NAME="${PERFTEST_NAME}_${JOB_DESC[i]}"; $LOGJOBS -f $file -n $numjobs 2>/dev/null; PERFTEST_NAME=$sPN; i=$(($i + 1)); done | sort | uniq`
    #sleep 2
    #shutdown $PID
}

group_a_test_5 ()
{
    PERFTEST_COMPONENT="$GLITE_LOCATION/bin/glite-lb-bkserverd"
    COMPONENT_ARGS="-P -d -o /tmp/proxy.perf --proxy-il-sock /tmp/interlogger.perf  --proxy-il-fprefix /tmp/perftest.log -D /tmp -t 1 "

    PERFTEST_CONSUMER="$GLITE_LOCATION/bin/glite-lb-interlogd-perf-empty"
    CONSUMER_ARGS="-d -s /tmp/interlogger.perf --file-prefix=/tmp/perftest.log -i /tmp/il-perf.pid"
    export PERFTEST_NAME="proxy_test_5"
    echo -n "5)"
    run_test proxy $numjobs
    print_result
    #$PERFTEST_COMPONENT -P -d -o /tmp/proxy.perf -t 1 -D /tmp  >/dev/null 2>&1  &
    #PID=$!
    purge_proxy `i=0; for file in ${JOB_FILE[*]}; do sPN=$PERFTEST_NAME; PERFTEST_NAME="${PERFTEST_NAME}_${JOB_DESC[i]}"; $LOGJOBS -f $file -n $numjobs 2>/dev/null ; PERFTEST_NAME=$sPN; i=$(($i + 1)); done | sort | uniq`
    #sleep 2
    #shutdown $PID
    rm -f /tmp/perftest.log.*
}

group_a_test_6 ()
{
    PERFTEST_COMPONENT="$GLITE_LOCATION/bin/glite-lb-bkserverd"
    COMPONENT_ARGS="-P -d -o /tmp/proxy.perf --proxy-il-sock /tmp/interlogger.perf  --proxy-il-fprefix /tmp/perftest.log -D /tmp -t 1 "

    PERFTEST_CONSUMER="$GLITE_LOCATION/bin/glite-lb-interlogd-perf"
    CONSUMER_ARGS="-d --nosend -s /tmp/interlogger.perf --file-prefix=/tmp/perftest.log -i /tmp/il-perf.pid"
    export PERFTEST_NAME="proxy_test_6"
    echo -n "6)"
    run_test proxy $numjobs
    print_result
    #$PERFTEST_COMPONENT -P -d -o /tmp/proxy.perf -t 1 -D /tmp  >/dev/null 2>&1  &
    #PID=$!
    purge_proxy `i=0; for file in ${JOB_FILE[*]}; do sPN=$PERFTEST_NAME; PERFTEST_NAME="${PERFTEST_NAME}_${JOB_DESC[i]}"; $LOGJOBS -f $file -n $numjobs 2>/dev/null ; PERFTEST_NAME=$sPN; i=$(($i + 1)); done | sort | uniq`
    #sleep 2
    #shutdown $PID
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
    #echo -e "\tavg_job \t big_job \t avg_dag \t big_dag"
    print_result_header
fi

if [[ "x$TEST_VARIANT" = "x*" ]]
then
   TEST_VARIANT="1 2 3 4 5 6"
fi

for variant in $TEST_VARIANT
do
    if [[ "$variant" = "5" || "$variant" = "6" ]]
    then
	group_${group}_test_${variant}
    else
	group_${group}_test_n $variant
    fi
done
