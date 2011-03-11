#!/bin/sh

jobreg=./glite-lb-job_reg
logevent=../../../bin/glite-lb-logevent

usage() {
	echo "Usage: $0 -m bkserver -p parent"
	exit 1
}

while test -n "$1"; do
	case "$1" in
		"-m") shift; bkserver="$1";;
		"-p") shift; parent="$1";;
		*) usage ;;
	esac
	shift
done

eval `$jobreg -s userinterface -m $bkserver -P | tail -n 2`

$logevent -s logmonitor -j $EDG_JOBID -e pbsqueued --queue 'myQueue' --owner 'jobOwner' --name 'jobName' -c 'TIMESTAMP=00000000000000:POS=0000000000:EV.CODE=000:SRC=s'
$logevent -s logmonitor -j $EDG_JOBID -e pbsmatch --dest_host 'PBSworker' -c 'TIMESTAMP=00000000000001:POS=0000000000:EV.CODE=000:SRC=s'

$logevent -s logmonitor -j $EDG_JOBID -e workflowmember -c 'TIMESTAMP=00000000000002:POS=0000000000:EV.CODE=000:SRC=s' --parent $parent --node_id 'abfd-0123-5678-abcd' --instance 1

$logevent -s logmonitor -j $EDG_JOBID -e pbsrun -c 'TIMESTAMP=00000000000003:POS=0000000000:EV.CODE=000:SRC=s'
$logevent -s logmonitor -j $EDG_JOBID -e pbsrun -c 'TIMESTAMP=00000000000004:POS=0000000000:EV.CODE=000:SRC=m'

$logevent -s logmonitor -j $EDG_JOBID -e pbsdone -c 'TIMESTAMP=00000000000005:POS=0000000000:EV.CODE=000:SRC=s'
