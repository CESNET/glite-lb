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

cat <<END_OF_TEXT
WARNING: this utility is provided for emergency and expert use only.
Exact semantics of job age are slightly different from regular glite-lb-bkpurge,
there is no synchronization with running LB server or proxy when jobs receive
new events while being purged, and error handling is minimal.

END_OF_TEXT

if [ -f "$(echo /tmp/lb_offline_purge.*|sed 's/ .*//')" ];then
echo >&2 $0 probably already running!
fi

if killall -0 glite-lb-bkserverd 2>/dev/null || killall -0 glite-lb-proxy 2>/dev/null
then
echo >&2 glite-lb-bkserverd or glite-lb-proxy running!
fi

echo
echo -n "Continue (y/n)?"; read r
if [ "$r" = "y" ]
then echo Working...
else echo Aborting.; exit 1;
fi


getsecs() {
	echo $(($(echo "$1" | sed 	-e 's/s$//' \
					-e 's/m$/*60/' \
					-e 's/h$/*3600/' \
					-e 's/d$/*86400/') ))
}

now=$(date "+%s")

tclause=""
add_tclause() {
	case "$1" in
		6|7|8|9) spart="states.status=$1";;
		o) spart="(states.status<6 OR states.status>9)";;
	esac
	if [ -z "$tclause" ]; then concat=""; else concat=" OR ";fi
	tclause="$tclause$concat($spart AND states.jobid = es.jobid and states.seq = es.event AND es.arrived<FROM_UNIXTIME($((now-$2))))"
}

dbname=lbserver20
method=staged
thorough=0
help=""

while [ -n "$1" ];do
	case "$1" in 
		-a)	
			add_tclause 8 $(getsecs "$2")
			shift 2;;
		-c)
			add_tclause 7 $(getsecs "$2")
			shift 2;;
		-n)
			add_tclause 9 $(getsecs "$2")
			shift 2;;
		-d)
			add_tclause 6 $(getsecs "$2")
			shift 2;;
		-o)
			add_tclause o $(getsecs "$2")
			shift 2;;

		-M)	method="$2"
			shift 2;;

		-m)	host="-h $(echo "$2" | sed 's/:[0-9]*$//')"
			shift 2;;

		-p)	dbname=lbproxy
			shift;;

		-t)	thorough=1
			shift;;

		-h)	help=yes
			shift;;
	esac
done

if [ -z "$tclause" -o -n "$help" ]
then
	echo "Usage: $0 [option] ...." 
	echo "       -a NNN[smhd]   purge ABORTED jobs older than NNN secs/mins/hours/days"
	echo "       -c NNN[smhd]   purge CLEARED jobs older than given time"
	echo "       -n NNN[smhd]   purge CANCELLED jobs older than given time"
	echo "       -d NNN[smhd]   purge DONE jobs older than given time"
	echo "       -o NNN[smhd]   purge OTHER jobs older than given time"
	echo "       -h             display this help"
	echo "       -p             purge LB proxy database (default=LB server DB)"
	echo "       -m             L&B DB server machine name (default=localhost)"
	echo "       -t             free unused space if using MyISAM DB engine"
	#echo "       -M {singleshot,staged}      cleaning method (default=staged)"

	exit 1
fi

tempfile1=`mktemp /tmp/lb_offline_purge.XXXXXX` || exit 1

mysqlcmd="mysql -B -s -n $host -u lbserver $dbname"
quick=""
if [ "$thorough" = 1 ]; then quick=" QUICK";fi

if [ $method = singleshot ]; then
ds="DELETE$quick FROM jobs,states,e,short_fields,long_fields,status_tags"
ds="$ds USING states STRAIGHT_JOIN events AS es "
ds="$ds LEFT JOIN events AS e ON (e.jobid = states.jobid)"
ds="$ds LEFT JOIN jobs ON (jobs.jobid = states.jobid)"
ds="$ds LEFT JOIN short_fields ON (short_fields.jobid = e.jobid AND short_fields.event=e.event)"
ds="$ds LEFT JOIN long_fields ON (long_fields.jobid = e.jobid AND long_fields.event=e.event)"
ds="$ds LEFT JOIN events_flesh ON (events_flesh.jobid = e.jobid AND events_flesh.event=e.event)"
ds="$ds LEFT JOIN status_tags ON (status_tags.jobid = states.jobid)"
ds="$ds WHERE ($tclause)"

echo "$ds" | $mysqlcmd
fi

if [ $method = staged ];then
ds="SELECT states.jobid FROM states STRAIGHT_JOIN events AS es"
for fl in A B C D E F G H I J K L M N O P Q R S T U V W X Y Z a b c d e f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9 - \\_
do
	echo "$ds WHERE states.jobid LIKE '$fl%' AND ($tclause);"
done | $mysqlcmd > $tempfile1

ds="DELETE$quick FROM jobs,states,e,short_fields,long_fields,status_tags"
ds="$ds USING states"
ds="$ds LEFT JOIN events AS e ON (e.jobid = states.jobid)"
ds="$ds LEFT JOIN jobs ON (jobs.jobid = states.jobid)"
ds="$ds LEFT JOIN short_fields ON (short_fields.jobid = e.jobid AND short_fields.event=e.event)"
ds="$ds LEFT JOIN long_fields ON (long_fields.jobid = e.jobid AND long_fields.event=e.event)"
ds="$ds LEFT JOIN events_flesh ON (events_flesh.jobid = e.jobid AND events_flesh.event=e.event)"
ds="$ds LEFT JOIN status_tags ON (status_tags.jobid = states.jobid)"

#wc -l $tempfile1 /dev/null;echo -n "???";read n
(
	cnt=0
	echo "SET autocommit=0;"
	echo "BEGIN;"
	while read jobid
	do
		echo "UPDATE acls,jobs SET acls.refcnt=acls.refcnt-1 " \
			"WHERE jobs.jobid='$jobid' AND jobs.aclid=acls.aclid;"
		echo "$ds WHERE states.jobid='$jobid';"
		if [ $(($cnt % 10)) = 9 ]
		then
			echo "COMMIT;"
			echo "BEGIN;"
		fi
		cnt=$((cnt+1))
	done
	echo "DELETE FROM acls WHERE refcnt=0;"
	echo "COMMIT;"
) < $tempfile1 | $mysqlcmd
rm $tempfile1
fi

if [ "$thorough" = 1 ]
then
	(
	for table in acls jobs status_tags events \
		short_fields states long_fields events_flesh
	do
		echo "OPTIMIZE TABLE $table;"
	done 
	) | $mysqlcmd
fi

# TODO: acls/singleshot, grey_jobs, notif_jobs, notif_registrations, users
