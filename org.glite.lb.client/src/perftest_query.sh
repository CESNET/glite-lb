#!/bin/sh
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

clients=${1:-3}

jobs=${2:-/tmp/perftest_10.jobids}

# XXX - there must be better way to find stage
if [ -z "${GLITE_LOCATION}" ]; then
        STAGEDIR=/home/michal/shared/egee/jra1-head/stage
else
        STAGEDIR=${GLITE_LOCATION}
fi
JOBSTAT=$STAGEDIR/examples/glite-lb-job_status

ask() {
	$JOBSTAT `cat $jobs`
}

do_ask() {
        while (true)
        do
                ask
                sleep `expr 5 \* $RANDOM / 32767`
        done
}

for i in `seq 1 $clients`
do
	do_ask &
done

