#! /bin/bash
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

#
# bug #29165: Times in job status history downloaded from LB are schifted
#
# description: 
#   Test compares returned history from:
#    - official job status example (C API)
#    - WS call (gsoap)
#    - WS XML response message (gsoap)
#
# requirements:
#   - stage area, default `pwd`
#   - built WS LB server examples
#   - gsoap-plugin in stage built with 'make DEBUG="-DDEBUG"'
#   - running LB server, LB locallogger, and LB interlogger
#   - proxy certificate for logging and querying
#

STAGE_LOCATION=${STAGE_LOCATION:-`pwd`}
WS_EXAMPLES_LOCATION=${WS_EXAMPLES_LOCATION:-$STAGE_LOCATION/../org.glite.lb.server/build}
LBSERVER=${LBSERVER:-`hostname -f`:9000}
LBLOGGER=${LBLOGGER:-`hostname -f`:9002}

script=$(EDG_WL_LOG_DESTINATION=$LBLOGGER $STAGE_LOCATION/examples/glite-lb-submitted.sh -m $LBSERVER 2>log | tail -n 1)
eval $script
test -z "$EDG_JOBID" && exit 2

echo "jobid: $EDG_JOBID"
EDG_WL_LOG_DESTINATION=$LBLOGGER $STAGE_LOCATION/examples/glite-lb-waiting.sh -j $EDG_JOBID 2>>log|| exit 3
sleep 1
EDG_WL_LOG_DESTINATION=$LBLOGGER $STAGE_LOCATION/examples/glite-lb-ready.sh -j $EDG_JOBID 2>>log|| exit 4
sleep 1
EDG_WL_LOG_DESTINATION=$LBLOGGER $STAGE_LOCATION/examples/glite-lb-done.sh -j $EDG_JOBID 2>>log|| exit 5
sleep 1
EDG_WL_LOG_DESTINATION=$LBLOGGER $STAGE_LOCATION/examples/glite-lb-cleared.sh -j $EDG_JOBID 2>>log|| exit 6
sleep 1
rm -f log

# C API
status="`TZ=C $STAGE_LOCATION/examples/glite-lb-job_status $EDG_JOBID`"
if test x$(echo "$status" | grep '^state[ :]' | sed -e 's/^.*:[ ]*\(.*\)/\1/') != xCleared; then
	echo "ERROR: Expected Cleared status of $EDG_JOBID"
	exit 7
fi
echo "$status" | grep '^stateEnterTimes[ :]' -A 12 > hist-c.txt

# WS call result
rm -f RECV.log SENT.log TEST.log
TZ=C $WS_EXAMPLES_LOCATION/ws_jobstat -j $EDG_JOBID | grep stateEnterTimes -A 12 > hist-ws.txt
diff -u hist-c.txt hist-ws.txt
ret1=$?
if [ "$ret1" != "0" ]; then
	echo "FAILED: WS and C API results differs"
fi

# WS XML message
if [ ! -s "RECV.log" ]; then
	echo "WARNING: RECV.log not found, WS/XML not compared"
	exit $ret1
fi
echo "stateEnterTimes : " > hist-ws-recv.txt
echo "     Undefined      - not available -" >> hist-ws-recv.txt
grep '^[ \t]*<' RECV.log | xmllint --format - | grep -A3 '<stateEnterTimes>' | grep '<state>\|<time\>' | tr '\n' ' ' | sed -e 's/<\/time>/<\/time>\n/g' | while read statexml timexml; do
	state=`echo $statexml | tr '\n' ' ' | sed -e 's/.*<state>\(.*\)<\/state>.*/\1/'`
	time=`echo $timexml | tr '\n' ' ' | sed -e 's/.*<time>\(.*\)<\/time>.*/\1/'`
	if [ x"$time" = x"1970-01-01T00:00:00Z" ]; then
		timetxt="    - not available -"
	else
		timetxt=`TZ=C perl -e 'use POSIX; $ARGV[0] =~ /^(\d\d\d\d)-(\d\d)-(\d\d)T(\d\d):(\d\d):(\d\d)Z$/; print ctime(mktime($6, $5, $4, $3, $2 - 1, $1 - 1900, 0, 0, 0));' $time`
	fi
	printf "%14s  " $state; printf "$timetxt\n";
done >> hist-ws-recv.txt
diff -iu hist-c.txt hist-ws-recv.txt
ret2=$?
if [ "$ret2" != "0" ]; then
	echo "FAILED: XML and C API results differs"
fi

if [ x"$ret1" = x"0" -a x"$ret2" = x"0" ]; then
	echo "PASSED"
	exit 0
else
	exit 1
fi
