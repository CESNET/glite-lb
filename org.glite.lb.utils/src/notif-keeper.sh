#! /bin/bash

GLITE_LOCATION=${GLITE_LOCATION:-"/opt/glite"}
NOTIFY=${GLITE_LB_NOTIFY:-"$GLITE_LOCATION/bin/glite-lb-notify"}
FILE='/var/tmp/msg-notif.txt'
SERVER="$1"
TTL=${2:-"7200"}
STAT="stat -c %Y"
#STAT="stat -f %m"


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

if [ -z "$SERVER" ]; then
	echo "Usage: $0 LB_SERVER [ TTL NEW_NOTIF_ARGUMRNTS... ]"
	echo
	echo "Environment:"
	echo "	DROP: drop the notification"
	echo "	DEBUG: show progress"
	exit 1
fi
shift
shift

if [ -n "$GLITE_HOST_CERT" -a -n "$GLITE_HOST_KEY" ] ;then
        X509_USER_CERT="$GLITE_HOST_CERT"
        X509_USER_KEY="$GLITE_HOST_KEY"
else
	echo "WARNING: host certificate not specified"
fi

export GLITE_WMS_NOTIF_SERVER="$SERVER"
export X509_USER_CERT
export X509_USER_KEY

now=`date '+%s'`
val=-1
ori=$now

# -- load previous/create new --

if [ -s "$FILE" ]; then load; fi
if [ -z "$nid" -o $val -le $now ]; then
	$NOTIFY new -t $TTL "$@" > "$FILE"
	load
	if [ $? != 0 ]; then
		echo "Can't create notification"
		exit 1
	fi
	[ -z "$DEBUG" ] || echo "`date '+%Y-%m-%d %H:%M:%S'`: notification '$nid' created"
fi

# -- refresh --

let ref="($ori+$val)/2"
if [ $ref -le $now ]; then
	$NOTIFY refresh -t $TTL $nid > "$FILE"-new
	if grep '^valid' "$FILE"-new >/dev/null 2>&1; then
		mv "$FILE" "$FILE".1
		grep '^notification ID:' "$FILE".1 > "$FILE"
		cat "$FILE"-new >> "$FILE"
		rm -f "$FILE"-new
		[ -z "$DEBUG" ] || echo "`date '+%Y-%m-%d %H:%M:%S'`: notification '$nid' refreshed"
	else
		echo "Can't refresh notification '$nid'"
		# next attempt in half of the remaining time
		touch "$FILE"
		exit 1
	fi
fi

# -- drop --

if [ ! -z "$DROP" ]; then
	$NOTIFY drop $nid
	rm -f "$FILE"
	[ -z "$DEBUG" ] || echo "`date '+%Y-%m-%d %H:%M:%S'`: notification '$nid' dropped"
fi
