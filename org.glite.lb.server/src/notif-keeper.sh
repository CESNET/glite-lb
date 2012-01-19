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

GLITE_LOCATION=${GLITE_LOCATION:-"/opt/glite"}
NOTIFY=${GLITE_LB_NOTIFY:-"$GLITE_LOCATION/bin/glite-lb-notify"}

# This function reads the full list of notifications to maintain into an array
function read_list() {

infile="$1"
TOTALNOTIFS=0

while read line ; do
	cleanline=`echo $line | sed -r 's/^\s*//'`
	echo "$line" | grep -E "^#" > /dev/null
	if [ $? -ne 0 ]; then
		HANDLES[$TOTALNOTIFS]=`echo $cleanline | sed -r 's/\s+.*$//'`
		OPTIONS[$TOTALNOTIFS]=`echo $cleanline | sed -r 's/^\w+\s+//'`
		TOTALNOTIFS=$(($TOTALNOTIFS+1))
	fi
done < $infile

}

function lookup_notifid() {
	retnotifid=""
	if [ -f $fname ]; then
		retnotifid=`grep -E '^Notifid: ' $fname | sed -r 's/^Notifid: //'`
	fi
}

function setup_new() {
	opts=${OPTIONS[$1]}
	echo glite-lb-notify new ${opts}
	retnotifid=`glite-lb-notify new ${opts} | grep -E "notification ID: " | sed 's/^notification ID: //'`
	echo $retnotifid
}

function drop() {
	notifid=${NOTIFID[${1}]}
	echo glite-lb-notify drop $notifid
	glite-lb-notify drop $notifid > /dev/null
}

function extend() {
	notifid=${NOTIFID[${1}]}
	echo glite-lb-notify refresh $notifid
	glite-lb-notify refresh $notifid > /dev/null
	if [ $? -ne 0 ]; then
		echo Failed to refresh notification for handle ${HANDLES[${1}]}
		NOTIFID[${1}]=""
	fi
}

function check_opts() {
	opts=${OPTIONS[$1]}
	if [ -f $fname ]; then
		storedopts=`grep -E '^Options: ' $fname | sed -r 's/^Options: //'`
	fi
	nospcopts=`echo $opts | sed -r 's/\s//g'`
	nospcstoredopts=`echo $storedopts | sed -r 's/\s//g'`
	printf "Checking options for ${HANDLES[$1]}\n\t$opts\n\t$storedopts\n"
	if [ "$nospcopts" == "$nospcstoredopts" ]; then
		checkopts_ret=0
	else
		checkopts_ret=1
	fi
}

function load() {
	out="`cat \"$FILE\"`"
	nid=`echo "$out" | grep '^notification ID:' | cut -f3 -d' '`
	val=`echo "$out" | grep '^valid' | sed 's/[^(]*(\([^)]*\)).*/\1/'`
	ori=`$STAT "$FILE"`

	if [ -z "$nid" -o -z "$val" ]; then
		val=-1
		return 1
	fi
}

function save_ttl() {
	mv "$FILE" "$FILE.1"
	cat "$FILE" | grep -v '^valid' > "$FILE"
}


# -- set up --

#if [ -z "$SERVER" ]; then
#	echo "Usage: $0 LB_SERVER [ TTL NEW_NOTIF_ARGUMRNTS... ]"
#	echo
#	echo "Environment:"
#	echo "	DROP: drop the notification"
#	echo "	DEBUG: show progress"
#	exit 1
#fi
#shift
#shift

if [ -n "$GLITE_HOST_CERT" -a -n "$GLITE_HOST_KEY" ] ;then
        X509_USER_CERT="$GLITE_HOST_CERT"
        X509_USER_KEY="$GLITE_HOST_KEY"
else
	echo "WARNING: host certificate not specified"
fi

#export GLITE_WMS_NOTIF_SERVER="$SERVER"
export X509_USER_CERT
export X509_USER_KEY

read_list $1

for ((i=0 ; i < ${TOTALNOTIFS} ; i++))
do
	fname="$GLITE_LB_LOCATION_VAR/notif-keeper-${HANDLES[${i}]}.notif"
	lookup_notifid ${HANDLE[${i}]}
	NOTIFID[i]=$retnotifid

	if [ "${NOTIFID[${i}]}" == "" ]; then
		setup_new $i
		NOTIFID[i]=$retnotifid
	else 
		check_opts $i
		if [ $checkopts_ret -eq 0 ]; then
			extend $i
			if [ "${NOTIFID[${i}]}" == "" ]; then
				setup_new $i
				NOTIFID[i]=$retnotifid
			fi
		else
			drop $i 
			setup_new $i
			NOTIFID[i]=$retnotifid
		fi
	fi
	echo Writing $fname
	printf "#This file is maintained automatically by script $0 initiated by cron\nOptions: ${OPTIONS[${i}]}\nNotifid: ${NOTIFID[${i}]}\n" > $fname
done
