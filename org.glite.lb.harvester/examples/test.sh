#! /bin/sh
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


usage() {
cat <<EOF

 ./test.sh

 Testing of the L&B harvester using local daemons and databases.

 Requirements:
  - working L&B software (ideally with passed org.glite.testsuites.ctb/LB/test)
  - mysql and postgresql databases, with sufficient access
    pg_hba.conf content for example:
      local all all trust all
  - created user \$RTM_NAME (rtm by default) in postgres DB
  - proxy certificate
  - writable working directory with harvester built with -DRTM_SQL_STORAGE_ENABLED=1
    (or via 'make test')

 non-default configuration is possible via following env variables or
 using ~/.glite.conf:
   GLITE_LB_LOCATION...............path to glite SW
   GLITE_LOCATION..................path to glite SW
   GLITE_HOST_CERT.................path to host certificate
   GLITE_HOST_KEY..................path to host key
   GLITE_LB_TEST_DB................L&B connection string 
                                   (default lbserver/@localhost:lbserver20test,
                                    autocreating the database when empty)
   GLITE_RTM_TEST_DB...............L&B connection string, existing user with
                                   privileges required
                                   (default rtm/@localhost:rtmtest,
                                    autocreating the database when empty)
   GLITE_MYSQL_ROOT_USER...........mysql root user (default: root)
   GLITE_MYSQL_ROOT_PASSWORD.......mysql root password (default: none)
   GLITE_PG_ROOT_USER..............postgres root user (default: postgres)
   GLITE_LB_TEST_SERVER_PORT.......(default 10000)
   GLITE_LB_TEST_PIDFILE...........(default /tmp/glite-lb-test.pid)
   GLITE_LB_NOTIF_IL_PIDFILE.......(default /tmp/glite-lb-notif-il-test.pid)
   GLITE_LB_PROXY_IL_PIDFILE.......(default /tmp/glite-lb-proxy-il-test.pid)
   GLITE_RTM_TEST_PIDFILE..........(default /tmp/glite-rtm-test.pid)
   GLITE_RTM_TEST_TTL..............notif validity (default 60 seconds)
   GLITE_RTM_TEST_ADDITIONAL_ARGS..additional arguments for harvester

EOF
}


init() {
	# get the configuration
	GLITE_LOCATION=${GLITE_LB_LOCATION:-$GLITE_LOCATION}
	GLITE_LOCATION=${GLITE_LOCATION:-'/opt/glite'}
	for dir in ${GLITE_LOCATION} ${GLITE_LOCATION}/lib64/glite-lb ${GLITE_LOCATION}/lib/glite-lb; do
		GLITE_LB_LOCATION_EXAMPLES="$dir/examples"
		if [ -d "$dir/examples" ]; then break; fi
	done
	for dir in "$GLITE_LOCATION_ETC" "$GLITE_LB_LOCATION_ETC" "$GLITE_LOCATION/etc" '/etc'; do
		GLITE_LOCATION_ETC="$dir";
		if [ -d "$dir" ]; then break; fi
	done

	[ -f /etc/glite.conf ] && . /etc/glite.conf
	[ -f /etc/default/glite-lb ] && . /etc/default/glite-lb
	[ -f /etc/sysconfig/glite-lb ] && . /etc/sysconfig/glite-lb
	[ -f $HOME/.glite.conf ] && . $HOME/.glite.conf
	
        if [ -n "$GLITE_HOST_CERT" -a -n "$GLITE_HOST_KEY" ] ;then
		X509_USER_CERT="$GLITE_HOST_CERT"
		X509_USER_KEY="$GLITE_HOST_KEY"
	fi
	
#	if [ -z "$X509_USER_CERT" -o -z "$X509_USER_KEY" ] ; then
		if [ -e "`which grid-proxy-info`" ] ; then
			timeleft=`grid-proxy-info 2>&1| \
				grep timeleft| sed 's/^.* //'`
			if [ "$timeleft" = "0:00:00" -o -z "$timeleft" ]; then 
				echo "Proxy certificate check failed."\
				" Aborting."
				exit 1
			fi
		else
			echo "Can't check proxy cert (grid-proxy-info not found). If you do not have valid proxy certificate, set GLITE_HOST_KEY/GLITE_HOST_KEY - otherwise tests will fail!"
		fi
#	fi
	identity=`X509_USER_KEY=${X509_USER_KEY} X509_USER_CERT=${X509_USER_CERT} grid-proxy-info 2>&1| \
		grep identity| sed 's/^[^/]*//'`

	if [ -z "$GLITE_LB_TEST_DB" ]; then
		GLITE_LB_TEST_DB="lbserver/@localhost:lbserver20test"
       		need_new_lb_db=1;
	fi
	DB_USER=`echo $GLITE_LB_TEST_DB| sed 's!/.*$!!'`
	DB_HOST=`echo $GLITE_LB_TEST_DB| sed 's!^.*@!!' | sed 's!:.*!!'`
	DB_NAME=`echo $GLITE_LB_TEST_DB| sed 's!^.*:!!'`
	MYSQL_ARGS="-u ${GLITE_MYSQL_ROOT_USER:-root}"
	[ -z "$GLITE_MYSQL_ROOT_PASSWORD" ] || MYSQL_ARGS="--password=${GLITE_MYSQL_ROOT_PASSWORD} $MYSQL_ARGS"

	if [ -z "$GLITE_RTM_TEST_DB" ]; then
		GLITE_RTM_TEST_DB="rtm/@localhost:rtmtest"
       		need_new_rtm_db=1;
	fi
	RTM_USER=`echo $GLITE_RTM_TEST_DB| sed 's!/.*$!!'`
	RTM_HOST=`echo $GLITE_RTM_TEST_DB| sed 's!^.*@!!' | sed 's!:.*!!'`
	RTM_NAME=`echo $GLITE_RTM_TEST_DB| sed 's!^.*:!!'`
	PG_ARGS="-U ${GLITE_PG_ROOT_USER:-postgres}"

	#other stuff
	GLITE_LB_TEST_SERVER_PORT=${GLITE_LB_TEST_SERVER_PORT:-"10000"}
	GLITE_LB_TEST_PIDFILE=${GLITE_LB_TEST_PIDFILE:-"/tmp/glite-lb-test.pid"}
	GLITE_LB_NOTIF_IL_PIDFILE=${GLITE_LB_NOTIF_IL_PIDFILE:-"/tmp/glite-lb-notif-il-test.pid"}
	GLITE_LB_PROXY_IL_PIDFILE=${GLITE_LB_PROXY_IL_PIDFILE:-"/tmp/glite-lb-proxy-il-test.pid"}
	GLITE_RTM_TEST_PIDFILE=${GLITE_RTM_TEST_PIDFILE:-"/tmp/glite-rtm-test.pid"}
	GLITE_RTM_TEST_TTL=${GLITE_RTM_TEST_TTL:-"60"}

	jobreg="$GLITE_LB_LOCATION_EXAMPLES/glite-lb-job_reg -m `hostname -f`:${GLITE_LB_TEST_SERVER_PORT} -s UserInterface"
	logev="$GLITE_LOCATION/bin/glite-lb-logevent -x -S `pwd`/LB/proxy.sockstore.sock -U localhost"
	purge="$GLITE_LOCATION/bin/glite-lb-purge"
	[ -x "$purge" ] || purge="$GLITE_LOCATION/sbin/glite-lb-purge"
	for dir in "$GLITE_LB_LOCATION_EXAMPLES" "`pwd`/../build" "`pwd`"; do
		if [ -x "$dir/glite-lb-harvester-dbg" ]; then
			rtm="$dir/glite-lb-harvester-dbg"
		fi
		if [ -x "$dir/harvester-dbg" ]; then
			rtm="$dir/harvester-dbg"
		fi
	done
	if [ -z "$rtm" ]; then
		echo "glite-lb-harvester-dbg not found"
		return 1
	fi

	if echo "$GLITE_RTM_TEST_ADDITIONAL_ARGS" | grep -- '[^-]\?\(--old\>\|-o\>\)' >/dev/null; then
		n_notifs=1
	else
		n_notifs=2
	fi

	rm -f log
}


drop_db() {
return 0
	[ -z "$lb_db_created" ] || mysqladmin -f $MYSQL_ARGS drop "$DB_NAME"
	[ -z "$rtm_db_created" ] || dropdb $PG_ARGS "$RTM_NAME"
}


create_db() {
	echo -n "mysql."
	# create database when needed
	if [ "x$need_new_lb_db" = "x1" ]; then
		mysqladmin -f $MYSQL_ARGS drop $DB_NAME > /dev/null 2>&1
		echo -n "."
		mysqladmin -f $MYSQL_ARGS create $DB_NAME && \
		echo -n "."
		mysql $MYSQL_ARGS -e "GRANT ALL on $DB_NAME.* to $DB_USER@$DB_HOST" && \
		echo -n "."
		mysql -u $DB_USER $DB_NAME -h $DB_HOST < $GLITE_LOCATION_ETC/glite-lb/glite-lb-dbsetup.sql || return $?
		echo -n "."
		mkdir -p `pwd`/LB
		cat > `pwd`/LB/glite-lb-index.conf << EOF
[
	JobIndices = {
		[ type = "system"; name = "lastUpdateTime" ]
	}
]
EOF
		LBDB="$GLITE_LB_TEST_DB" $GLITE_LOCATION/bin/glite-lb-bkindex -r `pwd`/LB/glite-lb-index.conf || return $?
		lb_db_created="1"
		echo -n "."
	else
		cleanup_mysql || return $?
	fi
	echo -n "OK psql."
	rm -f 'test.sql'
	if [ -f "$GLITE_LOCATION_ETC/glite-lb/harvester-test-dbsetup.sql" ]; then
		ln -s "$GLITE_LOCATION_ETC/glite-lb/harvester-test-dbsetup.sql" test.sql
	else
		wget --quiet -O 'test.sql' 'http://jra1mw.cvs.cern.ch/cgi-bin/jra1mw.cgi/org.glite.lb.harvester/examples/test.sql?revision=HEAD'
	fi
	if [ "x$need_new_rtm_db" = "x1" ]; then
		dropdb $PG_ARGS "$RTM_NAME" >/dev/null 2>&1
		echo -n "."
#		createuser $PG_ARGS -A -D "$RTM_NAME" >/dev/null 2>&1
#		echo -n "."
		createdb $PG_ARGS --encoding "UTF-8" --template template0 --owner "$RTM_USER" "$RTM_NAME" >psql-create.log 2>&1 || return $?
		rm psql-create.log
		echo -n "."
		rtm_db_created="1"
		echo "\i test.sql" | psql -AtF ',' -U "$RTM_USER" "$RTM_NAME" >/dev/null || return $?
		echo -n "."
	else
		cleanup_pg || return $?
	fi
	echo "OK"
}


cleanup_mysql() {
	cat << EOF | mysql -u $DB_USER $DB_NAME -h $DB_HOST || return $?
DELETE FROM acls;
DELETE FROM events;
DELETE FROM events_flesh;
DELETE FROM jobs;
DELETE FROM long_fields;
DELETE FROM notif_jobs;
DELETE FROM notif_registrations;
DELETE FROM server_state;
DELETE FROM short_fields;
DELETE FROM states;
DELETE FROM status_tags;
DELETE FROM users;
DELETE FROM zombie_jobs;
DELETE FROM zombie_prefixes;
DELETE FROM zombie_suffixes;
EOF
	echo -n "."
}


cleanup_pg() {
		cat << EOF | psql -AtF ',' -U "$RTM_USER" "$RTM_NAME" >/dev/null || return $?
DELETE FROM jobs;
DELETE FROM notifs;
EOF
	echo -n "."
}


run_daemons() {
	mkdir -p LB/dump LB/purge LB/voms 2>/dev/null

	# checks
	if [ -f "${GLITE_LB_TEST_PIDFILE}" ]; then
		echo "L&B server already running (${GLITE_LB_TEST_PIDFILE}, `cat ${GLITE_LB_TEST_PIDFILE}`)"
		quit=1
	fi
	if [ -f "${GLITE_RTM_TEST_PIDFILE}" ]; then
		echo "L&B harvester already running (${GLITE_RTM_TEST_PIDFILE}, `cat ${GLITE_RTM_TEST_PIDFILE}`)"
		quit=1
	fi
	if [ -e "`pwd`/LB/notif.sock" ]; then
		if [ "`lsof -t $(pwd)/LB/notif.sock | wc -l`" != "0" ]; then
			echo "Notification interlogger already running (using LB/notif.sock, `lsof -t $(pwd)/LB/notif.sock`)"
			quit=1
		fi
	fi
	if [ -e "`pwd`/LB/proxy-il.sock" ]; then
		if [ "`lsof -t $(pwd)/LB/proxy-il.sock | wc -l`" != "0" ]; then
			echo "Proxy interlogger already running (using LB/proxy-il.sock, `lsof -t $(pwd)/LB/proxy-il.sock`)"
			quit=1
		fi
	fi
	[ -z "$quit" ] || exit 1

	# run L&B server
	echo -n "L"
	X509_USER_KEY=${X509_USER_KEY} X509_USER_CERT=${X509_USER_CERT} \
	$GLITE_LOCATION/bin/glite-lb-bkserverd \
	  -m $GLITE_LB_TEST_DB \
	  -p $GLITE_LB_TEST_SERVER_PORT -w $(($GLITE_LB_TEST_SERVER_PORT + 3))\
	  -i ${GLITE_LB_TEST_PIDFILE} \
	  --withproxy -o `pwd`/LB/proxy.sock\
	  --proxy-il-sock `pwd`/LB/proxy-il.sock --proxy-il-fprefix `pwd`/LB/proxy-data \
	  -D `pwd`/LB/dump -S `pwd`/LB/purge \
	  -V `pwd`/LB/voms \
	  --notif-il-sock `pwd`/LB/notif.sock --notif-il-fprefix `pwd`/LB/notif-data \
	> `pwd`/LB/glite-lb-test-pre.log 2>&1
	if [ x"$?" != x"0" ]; then
		cat `pwd`/LB/glite-lb-test-pre.log
		echo FAILED
		drop_db;
		exit 1
	fi
	echo -n "B "

	# run L&B interlogger
	echo -n "L"
	X509_USER_KEY=${X509_USER_KEY} X509_USER_CERT=${X509_USER_CERT} \
	$GLITE_LOCATION/bin/glite-lb-interlogd \
	  --file-prefix `pwd`/LB/proxy-data --socket `pwd`/LB/proxy-il.sock \
	  --pidfile $GLITE_LB_PROXY_IL_PIDFILE > `pwd`/LB/glite-interlog-test-pre.log 2>&1
	if [ x"$?" != x"0" ]; then
		cat `pwd`/LB/glite-interlog-test-pre.log
		echo FAILED
		kill_bkserver
		drop_db;
		exit 1
	fi
	echo -n "I "

	# run L&B notification interlogger
	echo -n "N"
	X509_USER_KEY=${X509_USER_KEY} X509_USER_CERT=${X509_USER_CERT} \
	$GLITE_LOCATION/bin/glite-lb-notif-interlogd \
	  --file-prefix `pwd`/LB/notif-data --socket `pwd`/LB/notif.sock \
	  --pidfile $GLITE_LB_NOTIF_IL_PIDFILE > `pwd`/LB/glite-notif-test-pre.log 2>&1
	if [ x"$?" != x"0" ]; then
		cat `pwd`/LB/glite-notif-test-pre.log
		echo FAILED
		kill_daemons
		drop_db;
		exit 1
	fi
	echo -n "I "

	if ! start_harvester; then
		kill_daemons;
		drop_db;
		exit 1
	fi

	# wait for pidfiles
	i=0
	while [ ! -s "${GLITE_LB_TEST_PIDFILE}" -a $i -lt 20 ]; do
		sleep 0.1
		i=$(($i+1))
	done
	if [ ! -s "${GLITE_LB_TEST_PIDFILE}" ]; then
		echo "Can't startup L&B server."
		kill_daemons;
		drop_db;
		exit 1
	fi

	echo -n "notifs."
	pg_wait 20 "SELECT refresh FROM notifs WHERE notifid IS NOT NULL" $n_notifs || return $?
	refresh=`echo "$result" | head -n 1`
	if [ -z "$refresh" ]; then
		echo "FAIL"
		return 1
	fi
}


start_harvester() {
	# run L&B harvester server
	echo -n "R"
	rm -Rf RTM
	mkdir RTM 2>/dev/null
	echo "`hostname -f`:${GLITE_LB_TEST_SERVER_PORT}" > `pwd`/RTM/config.txt
	X509_USER_KEY=${X509_USER_KEY} X509_USER_CERT=${X509_USER_CERT} \
	${rtm} \
	  -m $GLITE_RTM_TEST_DB \
	  --pidfile ${GLITE_RTM_TEST_PIDFILE} \
	  --ttl ${GLITE_RTM_TEST_TTL} \
	  --history $((GLITE_RTM_TEST_TTL / 2)) \
	  --debug 12 \
	  --config `pwd`/RTM/config.txt \
	  --daemonize ${GLITE_RTM_TEST_ADDITIONAL_ARGS} 2>`pwd`/RTM/glite-rtm-test-pre.log >`pwd`/RTM/notifs.log
	if [ x"$?" != x"0" ]; then
		cat `pwd`/RTM/glite-rtm-test-pre.log
		echo FAILED
		return 1
	fi

	i=0
	while [ ! -s "${GLITE_RTM_TEST_PIDFILE}" -a $i -lt 20 ]; do
		sleep 0.1
		i=$(($i+1))
	done
	if [ ! -s "${GLITE_RTM_TEST_PIDFILE}" ]; then
		echo "Can't startup L&B harvester."
		kill_daemons;
		drop_db;
		exit 1
	fi

	echo -n "M "
}


cleanup_harvester() {
	echo -n "cleaning up..."
	X509_USER_KEY=${X509_USER_KEY} X509_USER_CERT=${X509_USER_CERT} \
	${rtm} \
	  -m $GLITE_RTM_TEST_DB \
	  --cleanup \
	  --debug 12 ${GLITE_RTM_TEST_ADDITIONAL_ARGS} >`pwd`/RTM/glite-rtm-test-cleanup.log 2>&1
	if [ x"$?" != x"0" ]; then
		cat `pwd`/RTM/glite-rtm-test-cleanup.log
		echo FAILED
		return 1
	fi
	echo -n "OK "
}


kill_daemons() {
	pid1=`cat ${GLITE_LB_TEST_PIDFILE} 2>/dev/null`
	[ -f "${GLITE_RTM_TEST_PIDFILE}" ] && pid2=`cat ${GLITE_RTM_TEST_PIDFILE}`
	pid3=`lsof -t $(pwd)/LB/notif.sock 2>/dev/null`
	pid4=`lsof -t $(pwd)/LB/proxy-il.sock 2>/dev/null`
	[ ! -z "$pid1" ] && kill $pid1
	[ ! -z "$pid2" ] && kill -2 $pid2
	[ ! -z "$pid3" ] && kill $pid3
	[ ! -z "$pid4" ] && kill $pid4
	sleep 1;
	[ ! -z "$pid1" ] && kill -9 $pid1 2>/dev/null
	[ ! -z "$pid2" ] && kill -9 $pid2 2>/dev/null
	[ ! -z "$pid3" ] && kill -9 $pid3 2>/dev/null
	[ ! -z "$pid4" ] && kill -9 $pid4 2>/dev/null
	rm -f "${GLITE_LB_TEST_PIDFILE}" "${GLITE_RTM_TEST_PIDFILE}" "${GLITE_LB_NOTIF_IL_PIDFILE}" "${GLITE_LB_PROXY_IL_PIDFILE}"
	rm -f `pwd`/LB/*.sock
}


kill_bkserver() {
	pid=`cat ${GLITE_LB_TEST_PIDFILE} 2>/dev/null`
	if [ ! -z "$pid1" ]; then
		kill $pid;
		sleep 1;
		kill -9 $pid
	fi
	rm -f "${GLITE_LB_TEST_PIDFILE}"
}


kill_harvester() {
	pid=`cat ${GLITE_RTM_TEST_PIDFILE} 2>/dev/null`
	if [ ! -z "$pid1" ]; then
		kill $pid
		sleep 1;
		kill -9 $pid 2>/dev/null
	fi
	rm -f "${GLITE_RTM_TEST_PIDFILE}"
}


reg() {
	echo -n "R"
	echo $jobreg $@ >> log
	$jobreg $@ > jobreg.tmp
	if [ $? -ne 0 ]; then
		cat jobreg.tmp
		rm -f jobreg.tmp
		echo " FAIL!"
		return 1;
	fi
	script=`cat jobreg.tmp | tail -n 2`
	rm -f jobreg.tmp
	EDG_JOBID=
	EDG_WL_SEQUENCE=
	eval $script
	if [ -z "$EDG_JOBID" -o -z "$EDG_WL_SEQUENCE" ]; then
		echo " FAIL!"
		return 1;
	fi
	echo -n "G "
}


ev() {
	echo -n "E"
	echo $logev -j "$EDG_JOBID" -c "$EDG_WL_SEQUENCE" "$@" >> log
	$logev -j "$EDG_JOBID" -c "$EDG_WL_SEQUENCE" "$@" 2> logev-err.tmp >logev.tmp
	if [ $? -ne 0 ]; then
		echo " FAIL!"
		return 2;
	fi
	EDG_WL_SEQUENCE=`cat logev.tmp`
	rm logev.tmp logev-err.tmp
	echo -n "V "
}


pg_get() {
	result=
	lines=
	echo "$1" | psql -AtF ',' -U "$RTM_USER" "$RTM_NAME" > psql.tmp
	if [ $? != 0 ]; then
		return $?
	fi
	result="`cat psql.tmp`"
	lines=`wc -l psql.tmp | sed 's/^[ ]*//' | cut -f1 -d' '`
#	rm psql.tmp
	return 0
}


pg_wait() {
	timeout=$(($1*2))
	sql="$2"
	n="$3"

	i=0
	found=0
	result=
	echo -n "S"
	echo "`date '+%Y-%m-%d %H:%M:%S'` $sql" >> log
	while [ "$found" = "0" -a $i -lt $timeout ]; do
		pg_get "$sql" || return $?
		echo -n "."
		if [ -z "$n" ]; then
			if [ "$lines" != "0" ]; then found=1; fi
		else
			if [ "$lines" = "$n" ]; then found=1; fi
		fi
		if [ "$found" = "0" ]; then sleep 0.5; fi
		i=$(($i+1))
	done
	echo -n "Q "
	result="$result"
	echo "`date '+%Y-%m-%d %H:%M:%S'` $lines lines" >> log
	if [ ! -z "$result" ]; then
		echo "$result" | sed -e 's/\(.*\)/\t\1/' >> log
	fi
	return 0
}


my_get() {
	result=
	lines=
	echo "`date '+%Y-%m-%d %H:%M:%S'` $1" >> log
	echo "$1" | mysql -B -u "$DB_USER" "$DB_NAME" > mysql.tmp
	if [ $? != 0 ]; then
		return $?
	fi
	result=`cat mysql.tmp | tail -n +2`
	lines=`echo "$result" | grep -v '^$' | wc -l | sed 's/^[ ]*//'`
	echo "`date '+%Y-%m-%d %H:%M:%S'` $lines lines" >> log
	if [ ! -z "$result" ]; then
		echo "$result" | sed -e 's/\(.*\)/\t\1/' >> log
	fi
#	rm -f mysql.tmp
	return 0
}


# notif propagation
test_basic() {
	ok=0

	# submited
	echo -n "submitted..."
	reg || return $?
	pg_wait 10 "SELECT jobid, state FROM jobs WHERE state='Submitted'" || return $?
	if [ -z "$result" ]; then
		echo "FAIL"
		return 0
	fi

	# waiting
	echo -n "waiting..."
	ev -s NetworkServer -e Accepted --from='UserInterface' --from_host=`hostname -f` --from_instance="pid$$" || return $?
	pg_wait 10 "SELECT jobid, state FROM jobs WHERE state='Waiting'" || return $?
	if [ -z "$result" ]; then
		echo "FAIL"
		return 0
	fi

	# running
	echo -n "running..."
	ev -s LogMonitor -e Running --node="worker node" || return $?
	pg_wait 10 "SELECT jobid, state FROM jobs WHERE state='Running'" || return $?
	if [ -z "$result" ]; then
		echo "FAIL"
		return 0
	fi

	ok=1
	echo "OK"
}


# proper notif registration cleanup
test_rebind() {
	ok=0

	# ---- active ---
	echo -n "$n_notifs notifications "
	my_get "SELECT notifid FROM notif_registrations" || return $?
	# STATUS and JDL
	if [ "$lines" != "$n_notifs" ]; then
		echo "FAIL"
		return 0
	fi
	echo -n "OK "

	# ---- store & stop ---
	echo -n "store&quit"
	pid=`cat ${GLITE_RTM_TEST_PIDFILE}`
	kill $pid
	i=0
	while [ -s "${GLITE_RTM_TEST_PIDFILE}" -a $i -lt 200 ]; do
		echo -n "."
		sleep 0.5
		i=$(($i+1))
	done
	if [ -s "${GLITE_RTM_TEST_PIDFILE}" ]; then
		echo "FAIL"
		return 0
	fi
	echo -n "OK notifs "
	my_get "SELECT notifid FROM notif_registrations" || return $?
	if [ "$lines" != "$n_notifs" ]; then
		echo "FAIL"
		return 0
	fi
	echo -n "OK "

	# ---- launch & rebind ---
	if ! start_harvester; then
		kill_daemons;
		drop_db;
		exit 1
	fi

	echo -n "bind"
	pg_wait 20 "SELECT notifid FROM notifs WHERE notifid IS NOT NULL" $n_notifs || return $?
	if [ x"$lines" != x"$n_notifs" ]; then
		echo "FAIL"
		return 0
	fi

	echo -n "Done "
	ev -s LogMonitor -e Done --status_code=OK --reason="Finished, yeah!" --exit_code=0 || return $?
	pg_wait 20 "SELECT jobid, state FROM jobs WHERE state='Done'"
	if [ -z "$result" ]; then
		echo "FAIL"
		return 0
	fi

	ok=1
	echo "OK"
}


test_cleanup() {
	ok=0

	# ---- deep stop ---
	echo -n "deep quit"
	pid=`cat ${GLITE_RTM_TEST_PIDFILE}`
	kill -2 $pid
	i=0
	while [ -s "${GLITE_RTM_TEST_PIDFILE}" -a $i -lt 200 ]; do
		echo -n "."
		sleep 0.5
		i=$(($i+1))
	done
	if [ -s "${GLITE_RTMTESTPIDFILE}" ]; then
		echo "FAIL"
		return 0
	fi

	echo -n "$n_notifs notifications..."
	my_get "SELECT notifid FROM notif_registrations" || return 1
	if [ "$lines" != "$n_notifs" ]; then
		echo "FAIL"
		return 0
	fi

	cleanup_harvester || return $?
	echo -n "0 notifications..."
	my_get "SELECT notifid FROM notif_registrations" || return 1
	if [ "$lines" != "0" ]; then
		echo "FAIL"
		return 0
	fi

	echo -n "cleandb."
	cleanup_pg || return $?
	start_harvester || return $?

	echo -n "notifs."
	pg_wait 20 "SELECT refresh FROM notifs WHERE notifid IS NOT NULL" $n_notifs || return $?
	refresh=`echo "$result" | head -n 1`
	if [ -z "$refresh" ]; then
		echo "FAIL"
		return 0
	fi

	ok=1
	echo "OK"
}


test_refresh() {
	ok=0

	echo -n "refresh."
	pg_wait $((GLITE_RTM_TEST_TTL * 3 / 4)) "SELECT notifid FROM notifs WHERE notifid IS NOT NULL AND refresh>'$refresh'" $n_notifs || return $?
	if [ -z "$result" ]; then
		echo "FAIL"
		return 0
	fi

	ok=1
	echo "OK"
}


test_jdl() {
	ok=0

#	kill_daemons
#	cleanup_mysql && cleanup_pg || return $?
#	run_daemons || return $?

	# need to wait for notifications to avoid bootstrap
	echo -n "notifs."
	pg_wait 20 "SELECT refresh FROM notifs WHERE notifid IS NOT NULL" $n_notifs || return $?
	refresh=`echo "$result" | head -n 1`
	if [ -z "$refresh" ]; then
		echo "FAIL"
		return 0
	fi
	echo -n "OK "

	echo -n "submitted..."
	reg || return $?
	pg_wait 10 "SELECT jobid, state FROM jobs WHERE state='Submitted'" || return $?
	if [ -z "$result" ]; then
		echo "FAIL"
		return 0
	fi
	echo -n "OK "

	echo -n "waiting..."
	cat > jdl.txt << EOF
[
 VirtualOrganisation = "TestingVO";
]
EOF
	ev -s NetworkServer -e Accepted --from='UserInterface' --from_host=`hostname -f` --from_instance="pid$$" || return $?
	ev -s NetworkServer -e EnQueued --queue "very long and chaotic queue" --job=`pwd`/jdl.txt --result START || return $?
	ev -s NetworkServer -e EnQueued --queue "very long and chaotic queue" --job="`cat jdl.txt`" --result OK || return $?
	pg_wait 10 "SELECT jobid, state FROM jobs WHERE state='Waiting'" || return $?
	if [ -z "$result" ]; then
		echo "FAIL"
		return 0
	fi
	echo -n "OK "

	echo -n "waiting and VO..."
	pg_wait 10 "SELECT jobid, state FROM jobs WHERE state='Waiting' AND vo='TestingVO'" || return $?
	if [ -z "$result" ]; then
		echo "FAIL"
		return 0
	fi
	echo -n "OK "

	#
	# test JDL via VO change
	#
	# never do it at home ;-)
	#

	echo -n "changed JDL..."
	ev -s NetworkServer -e EnQueued --queue "very long and chaotic queue" --job="[ VirtualOrganisation=\"TestingVO2\";]" --result OK || return $?
	pg_wait 10 "SELECT jobid, state FROM jobs WHERE state='Waiting' AND vo='TestingVO2'" || return $?
	if [ -z "$result" ]; then
		echo "FAIL"
		return 0
	fi
	echo -n "OK "

	echo -n "changed after waiting..."
	ev -s WorkloadManager -e EnQueued --queue "very long and chaotic queue" --destination LogMonitor --dest_host localhost --dest_instance pid$$ --job "(car 'testing=true)"  --result=OK || return $?
	pg_wait 10 "SELECT jobid, state FROM jobs WHERE state='Ready' AND vo='TestingVO2'" || return $?
	if [ -z "$result" ]; then
		echo "FAIL"
		return 0
	fi
	echo -n "ready..."
	ev -s NetworkServer -e EnQueued --queue "very long and chaotic queue" --job="[ VirtualOrganisation=\"TestingVO3\";]" --result OK || return $?
	pg_wait 10 "SELECT jobid, state FROM jobs WHERE state='Waiting' AND vo='TestingVO3'" || return $?
	if [ -z "$result" ]; then
		echo "FAIL"
		return 0
	fi

	ok=1
	echo "OK"
}


test_purge() {
	ok=0

	echo -n "purge."
	pg_get "SELECT jobid FROM jobs" || return $?
	if [ -z "$lines" -o $lines -le 0  ]; then
		echo "no jobs! FAIL"
		return 0
	fi
	echo -n "P"
	jobunique=`echo "$result" | head -n 1 | tr -d '\n'`
	jobid="https://`hostname -f`:${GLITE_LB_TEST_SERVER_PORT}/$jobunique"
	echo $jobid > jobs
	echo "${purge} -a1 -c1 -n1 -e1 -o1 -m "`hostname -f`:${GLITE_LB_TEST_SERVER_PORT}" -j jobs" >> log
	echo "  jobs = `cat jobs` | tr -d '\n'" >> log
	X509_USER_KEY=${X509_USER_KEY} X509_USER_CERT=${X509_USER_CERT} ${purge} -l -a1 -c1 -n1 -e1 -o1 -m "`hostname -f`:${GLITE_LB_TEST_SERVER_PORT}" -j jobs 2> purge-err.tmp >purge.tmp
	if [ $? -ne 0 ]; then
		echo " FAIL!"
		return 2;
	fi
	rm -f jobs
	echo -n "R "

	pg_wait 10 "SELECT * FROM jobs WHERE jobid='$jobunique'" 0 || return $?
	if [ x"$lines" != x"0" ]; then
		echo "FAIL"
		return 0
	fi

	ok=1
	echo "OK"
}


quit() {
	if [ x"$started" = x"" ]; then
		kill_daemons
		drop_db
	fi
	exit 1
}


fatal() {
	echo "Fatal error, end"
	quit
}


start() {
	echo -n "Launch: "
	create_db || fatal
	run_daemons || fatal
	echo "OK"
	started=1
}


stop() {
	kill_daemons
	drop_db
}


test() {
	echo -n "Basic: "
	test_basic || fatal
	if [ $ok != 1 ]; then quit; fi

	echo -n "Rebind: "
	test_rebind || fatal
	if [ $ok != 1 ]; then quit; fi

	echo -n "Cleanup: "
	test_cleanup || fatal
	if [ $ok != 1 ]; then quit; fi

	echo -n "Refresh: "
	test_refresh || fatal
	if [ $ok != 1 ]; then quit; fi

	echo -n "JDL: "
	test_jdl || fatal
	if [ $ok != 1 ]; then quit; fi

#	echo -n "Purge: "
#	test_purge || fatal
#	if [ $ok != 1]; then quit; fi
}


case x"$1" in
xstart)
	init
	start
	;;

xstop)
	init
	stop
	;;

xtest)
	init
	test
	;;

x)
	init
	start
	test
	stop
	;;

*)
	usage
	exit 1
esac
