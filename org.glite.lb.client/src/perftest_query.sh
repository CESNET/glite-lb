#!/bin/sh

clients=${1:-3}

jobs=${2:-/tmp/perftest_10.jobids}

# XXX - there must be better way to find stage
if [ -z "${GLITE_LOCATION}" ]; then
        STAGEDIR=/home/michal/shared/egee/jra1-head/stage
else
        STAGEDIR=${GLITE_LOCATION}
fi
JOBSTAT:=$STAGEDIR/examples/glite-lb-job_status

ask() {
	$JOBSTAT `cat $jpbs`
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

