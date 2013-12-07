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

numjobs=1

if [ -z "${GLITE_LOCATION}" ]; then
	# let's be in stage
	STAGEDIR=`pwd`/usr
else
	STAGEDIR=${GLITE_LOCATION}
fi

. $STAGEDIR/sbin/perftest_common.sh

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

DEBUG=${DEBUG:-0}
# CONSUMER_ARGS=
# PERFTEST_COMPONENT=
# COMPONENT_ARGS=
# LOGJOBS_ARGS="" 
COMM_ARGS="--file-prefix /tmp/perftest.log -p 45678"
export EDG_WL_LOG_DESTINATION="localhost:45678"

check_test_files || exit 1

group_a () {
echo "----------------
Locallogger test
----------------
a) glite-lb-logd-perf-nofile --noParse --noIPC
b) glite-lb-logd-perf-nofile --noIPC
c) glite-lb-logd-perf --noIPC
d) glite-lb-logd-perf

Number of jobs: $numjobs
"
}


# a)
group_a_test_a ()
{
echo -n "a)"
PERFTEST_CONSUMER=$STAGEDIR/sbin/glite-lb-logd-perf-nofile
CONSUMER_ARGS="-d --noIPC --noParse $COMM_ARGS"
init_result
run_test ll $numjobs
#print_result_ev
print_result
}

# b)
group_a_test_b ()
{
echo -n "b)"
PERFTEST_CONSUMER=$STAGEDIR/sbin/glite-lb-logd-perf-nofile
CONSUMER_ARGS="-d --noIPC $COMM_ARGS"
init_result
run_test ll $numjobs
#print_result_ev
print_result
}

# c)
group_a_test_c () 
{
echo -n "c)"
PERFTEST_CONSUMER=$STAGEDIR/sbin/glite-lb-logd-perf
CONSUMER_ARGS="-d --noIPC $COMM_ARGS"
init_result
run_test ll $numjobs
#print_result_ev
print_result
rm -f /tmp/perftest.log.*
}

# d)
group_a_test_d ()
{
echo -n "d)"
PERFTEST_CONSUMER=$STAGEDIR/sbin/glite-lb-interlogd-perf-empty
CONSUMER_ARGS="-d -s /tmp/perftest.sock"
PERFTEST_COMPONENT=$STAGEDIR/sbin/glite-lb-logd-perf
COMPONENT_ARGS="-d -s /tmp/perftest.sock $COMM_ARGS"
init_result
run_test ll $numjobs
#print_result_ev
print_result
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
    print_result_header
fi

if [[ "x$TEST_VARIANT" = "x*" ]]
then
   TEST_VARIANT="a b c d"
fi

for variant in $TEST_VARIANT
do
    group_${group}_test_${variant}
done
