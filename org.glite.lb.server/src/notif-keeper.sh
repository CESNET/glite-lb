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
		TOPICS[$TOTALNOTIFS]=`echo ${OPTIONS[${TOTALNOTIFS}]} | grep -E -o '\-a[ ]+x-msg://[^ ]+' | sed -r 's/^.*msg:\/\///'`
		TOTALNOTIFS=$(($TOTALNOTIFS+1))
	fi
done < $infile

}

function vecho() {
	if [ $1 -le $Verbose ]; then
		shift
		printf "${HANDLES[${i}]}: \t"
		echo $*
	fi
}

function lookup_notifid() {
	retnotifid=""
	if [ -f $fname ]; then
		retnotifid=`grep -E '^Notifid: ' $fname | sed -r 's/^Notifid: //'`
	fi
}

function setup_new() {
	opts=${OPTIONS[$1]}
	vecho 2 glite-lb-notify new ${opts}
	retnotifid=`glite-lb-notify new ${opts} | grep -E "notification ID: " | sed 's/^notification ID: //'`
	vecho 2 $retnotifid
}

function drop() {
	notifid=${NOTIFID[${1}]}
	vecho 2 glite-lb-notify drop $notifid
	glite-lb-notify drop $notifid > /dev/null
	NOTIFID[${1}]=""
}

function extend() {
	notifid=${NOTIFID[${1}]}
	vecho 2 glite-lb-notify refresh $notifid
	glite-lb-notify refresh $notifid > /dev/null
	if [ $? -gt 0 ]; then
		vecho 1 Failed to refresh notification for handle ${HANDLES[${1}]}
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
	vecho 1 "Checking options for changes"
	if [ "$nospcopts" == "$nospcstoredopts" ]; then
		checkopts_ret=0
	else
		checkopts_ret=1
	fi
}

function check_timestamp() {
	tsfile=$FilePrefix.${TOPICS[${1}]}.stat
	if [ ! -f "$tsfile" ]; then
		vecho 0 WARNING: stat file $tsfile not found!
	else
		vecho 1 Parsing stat file $tsfile
		storedlc=`grep -E "^last_connected=" $tsfile | sed 's/^last_connected=//'`
		storedls=`grep -E "^last_sent=" $tsfile | sed 's/^last_sent=//'`
		if [ $storedlc -gt $storedls ]; then
			stored=$storedlc
		else
			stored=$storedls
		fi
		threshold=`expr $stored + $AGE`
		vecho 1 "Comparing current time ($NOW) with stale threshold ($threshold)"
		if [ $NOW -gt $threshold ]; then
			vecho 1 Age Stale
			age_check=1;
		else
			vecho 2 Age OK
			age_check=0;
		fi
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

function showHelp() {
cat << EndHelpHeader
Script for registering, checking and refreshing site-specific notifications.
It is intended primarily to be run by cron, but can be run manually by site
admins to avoid waiting for the next cron cycle.

EndHelpHeader

        echo "Usage: $progname [OPTIONS]"
        echo "Options:"
        echo " -h | --help            Show this help message."
        echo " -f | --file-prefix     Notification files prefix (same value as for server and"
	echo "                        notif-il)."
        echo " -n | --site-notif      Location of the site-notif.conf (input definition) file."
        echo " -a | --stale-age	      Time in seconds since last read before the registration is"
	echo "                        considered stale (default 345600 = 4 days)."
        echo " -v | --verbose         Verbose cmdline output. (Repeat for higher verbosity)"
}

Verbose=0
AGE=345600
while test -n "$1"
do
        case "$1" in
                "-h" | "--help") showHelp && exit 2 ;;
                "-f" | "--file-prefix") shift ; FilePrefix=$1 ;;
                "-n" | "--site-notif") shift ; SiteNotif=$1 ;;
		"-a" | "--stale-age" ) shift ; AGE=$1 ;;
                "-v" | "--verbose") Verbose=$(($Verbose+1)) ;;
                "-vv" ) Verbose=$(($Verbose+2)) ;;
		*) echo WARNING: unknown argument $1 ;;
        esac
        shift
done


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
	vecho 0 "WARNING: host certificate not specified"
fi

if [ -z "$GLITE_LB_LOCATION_VAR" ]; then
	export GLITE_LB_LOCATION_VAR=/var/glite
	vecho 0 "WARNING: GLITE_LB_LOCATION_VAR not specified, using default"
fi

if [ -z "$FilePrefix" ]; then
	export FilePrefix=/var/tmp/glite-lb-notif
	vecho 0 "WARNING: Notif file prefix not specified, using default"
fi

if [ -z "$SiteNotif" ]; then
	if [ -f "/etc/glite-lb/site-notif.conf" ]; then
		vecho 1 Configuration file site-notif.conf not specified, using default
		SiteNotif="/etc/glite-lb/site-notif.conf"
	else
		vecho 0 "ERROR: No configuration file (site-notif.conf)"
		exit 1
	fi
fi

touch $GLITE_LB_LOCATION_VAR/notif-keeper.rw.try.$$.tmp

if [ -f $GLITE_LB_LOCATION_VAR/notif-keeper.rw.try.$$.tmp ]; then
	rm $GLITE_LB_LOCATION_VAR/notif-keeper.rw.try.$$.tmp
else
	vecho 0 ERROR: $GLITE_LB_LOCATION_VAR not writable!
	exit 1
fi

#export GLITE_WMS_NOTIF_SERVER="$SERVER"
export X509_USER_CERT
export X509_USER_KEY

read_list $SiteNotif

for ((i=0 ; i < ${TOTALNOTIFS} ; i++))
do
	NOW=`date +%s`
	fname="$GLITE_LB_LOCATION_VAR/notif-keeper-${HANDLES[${i}]}.notif"
	lookup_notifid ${HANDLE[${i}]}
	NOTIFID[i]=$retnotifid

	if [ "${NOTIFID[${i}]}" == "" ]; then
		setup_new $i
		NOTIFID[i]=$retnotifid
		vecho 1 "New registration ($retnotifid)"
	else 
		check_opts $i
		if [ $checkopts_ret -eq 0 ]; then
			check_timestamp $i
			if [ $age_check -eq 0 ]; then
				extend $i
				if [ "${NOTIFID[${i}]}" == "" ]; then
					setup_new $i
					NOTIFID[i]=$retnotifid
					vecho 1 "Failed to extend. Registration recreated ($retnotifid)."
				else
					vecho 1 "Registration extended (${NOTIFID[${i}]})"
				fi
			else
				drop $i
				vecho 1 "Registration stale => dropped"
			fi
		else
			drop $i 
			setup_new $i
			NOTIFID[i]=$retnotifid
			vecho 1 "Options changed. Registration dropped and recreated ($retnotifid)."
		fi
	fi
	vecho 1 "Writing $fname"
	printf "#This file is maintained automatically by script $0 initiated by cron\nOptions: ${OPTIONS[${i}]}\nNotifid: ${NOTIFID[${i}]}\n" > $fname
done
