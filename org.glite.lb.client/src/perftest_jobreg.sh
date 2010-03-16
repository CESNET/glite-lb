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


test_glite_location()
{
	GLITE_LOCATION_VAR=${GLITE_LOCATION_VAR:-/tmp}

	[ -n "$GLITE_LB_PROXY_SOCK_PREFIX" ] && \
		export GLITE_WMS_LBPROXY_STORE_SOCK=$GLITE_LB_PROXY_SOCK_PREFIX\store.sock

	if [ -z "$GLITE_LOCATION" ];
	then
        	echo "GLITE_LOCATION not set! Exiting.";
	        exit 1;
	fi
}

test_credentials()
{
	[ -n "$GLITE_HOST_CERT" -a -n "$GLITE_HOST_KEY" ] &&
                creds="-c $GLITE_HOST_CERT -k $GLITE_HOST_KEY"

        if test -z "$creds"; then
                if [ -r /etc/grid-security/hostkey.pem -a -r /etc/grid-security/hostcert.pem ]; then
                        creds="-c /etc/grid-security/hostcert.pem -k /etc/grid-security/hostkey.pem -C /etc/grid-security/certificates"
                fi
        fi
        [ -z "$creds" ] && \
                echo WARNING: No credentials specified. Using default lookup which is dangerous. >&2
}

my_echo()
{
	echo $1 1>&2
}


start_bkserver()
{
	# production version - usable only when starting as root 
	# $GLITE_LOCATION/etc/init.d/glite-lb-bkserverd start

	# workaround
	[ -n "$GLITE_LB_SERVER_PIDFILE" ] && pidfile=$GLITE_LB_SERVER_PIDFILE ||
	        pidfile=$GLITE_LOCATION_VAR/glite-lb-bkserverd.pid

	[ -z "$GLITE_LB_EXPORT_JPREG_MAILDIR" ] && \
		GLITE_LB_EXPORT_JPREG_MAILDIR=/tmp/jpreg
        maildir="--jpreg-dir $GLITE_LB_EXPORT_JPREG_MAILDIR"
        [ -d "$GLITE_LB_EXPORT_JPREG_MAILDIR" ] || \
		mkdir -p "$GLITE_LB_EXPORT_JPREG_MAILDIR" 

	[ -n "$GLITE_LB_SERVER_PORT" ] && port="-p $GLITE_LB_SERVER_PORT"
        [ -n "$GLITE_LB_SERVER_WPORT" ] && wport="-w $GLITE_LB_SERVER_WPORT"
        [ -n "$GLITE_LB_SERVER_TRANSACTION" ] && trans="-b $GLITE_LB_SERVER_TRANSACTION"

	if [ -n "$1" ]; then
		sink="--perf-sink $1"
	else
		sink=""
	fi 	

	echo -n "Starting glite-lb-bkserver ..."
	$GLITE_LOCATION/bin/glite-lb-bkserverd \
		$creds -i $pidfile $port $wport $maildir $sink $trans\
		--purge-prefix /tmp/purge_new --dump-prefix /tmp/dump_new -s 1\
		&& echo " done" || echo " FAILED"
	echo
}


stop_bkserver()
{
	echo
	if [ -f $pidfile ]; then
                pid=`cat $pidfile`
                kill $pid
                echo -n Stopping glite-lb-bkserverd \($pid\) ...
                try=0
                while ps p $pid >/dev/null 2>&1; do
                        sleep 1;
			kill $pid 2>/dev/null
                        try=`expr $try + 1`
			if [ $try = 19 ]; then
                                echo "bkserver jammed - sending kill -9"
                                kill -9 $pid 2>/dev/null
                        fi
                        if [ $try = 20 ]; then
                                echo " giving up after $try retries"
                                return 1
                        fi
                done
                echo " done"
                rm -f $pidfile
		rm -rf $GLITE_LB_EXPORT_JPREG_MAILDIR
        else
                echo $pidfile does not exist - glite-lb-bkserverd not running? >&2 
                return 1
        fi

}


start_proxy()
{
	[ -n "$GLITE_LB_PROXY_PIDFILE" ] && proxy_pidfile=$GLITE_LB_PROXY_PIDFILE ||
		proxy_pidfile=$GLITE_LOCATION_VAR/glite-lb-proxy.pid

	[ -n "$GLITE_LB_PROXY_SOCK_PREFIX" ] && proxy_sock_prefix="-p $GLITE_LB_PROXY_SOCK_PREFIX" ||
		proxy_sock_prefix=""
	
	[ -n "$1" ] && sink="-K $1" || sink_mode=

	echo -n Starting glite-lb-proxy ...
	$GLITE_LOCATION/bin/glite-lb-proxy \
                -i $proxy_pidfile $proxy_sock_prefix $sink \
		&& echo " done" || echo " FAILED"	
	echo
}

stop_proxy()
{
	echo 
	if [ -f $proxy_pidfile ]; then
                pid=`cat $proxy_pidfile`
                kill $pid
                echo -n Stopping glite-lb-proxy \($pid\) ...
                try=0
                while ps p $pid >/dev/null 2>&1; do
                        sleep 1;
			kill $pid 2>/dev/null
                        try=`expr $try + 1`
                        if [ $try = 20 ]; then
                                echo " giving up after $try retries"
                                return 1
                        fi
                done
                echo " done"
                rm -f $proxy_pidfile
		rm -rf $GLITE_LB_EXPORT_JPREG_MAILDIR
        else
                echo $proxy_pidfile does not exist - glite-lb-proxy not running? >&2
                return 1
        fi
}

start_il()
{
	[ -n "$GLITE_LB_IL_SOCK" ] && il_sock="-s $GLITE_LB_PROXY_SOCK" ||
		il_sock=""

	[ -n "$GLITE_LB_IL_PREFIX" ] && il_prefix="-f $GLITE_LB_IL_PREFIX" ||
		il_prefix=""
	
	echo -n Starting glite-lb-interlogger ...
	$GLITE_LOCATION/bin/glite-lb-interlogd \
                $creds $il_sock $il_prefix \
		&& echo " done" || echo " FAILED"	
	echo

}

stop_il()
{
	killall glite-lb-interlogger
}


#  - Test types -
#
#   i) normally procesed by server & proxy
#  ii) server replies immedia success
# iii) proxy replies immediate succes
#   a) current implementation
#   b) connection pool
#   c) parallel communication

test_ai()
{
	dest=
	[ -z "$1" ] && echo "test_ai() - wrong params" && return
	[ "$1" = "proxy" ] && dest="-x"
	[ "$1" = "server" ] && dest="-m $BKSERVER_HOST"
	[ -z "$dest" ] && echo "test_ai() - wrong params" && return
	
	my_echo "================================================================"
	my_echo "Testing LB $1 with sink_mode ${sink_mode[$2]}"

	# single registration
	#
	my_echo "-n single registration ..."
	ai_sr_lb=`$GLITE_LOCATION/sbin/glite-lb-perftest_jobreg $dest`
	mega_actions_per_day=`echo "scale=6; 86400/$ai_sr_lb/1000000" | bc`
	my_echo ". $ai_sr_lb seconds ($mega_actions_per_day GU)"

	# average single registration (100 samples)
	#
	my_echo "-n average single registration (100 samples) ..."
	ai_100sr_lb=`$GLITE_LOCATION/sbin/glite-lb-perftest_jobreg $dest -N 100`
	ai_avg_sr_lb=`echo "scale=6; $ai_100sr_lb/100" |bc`
	mega_actions_per_day=`echo "scale=6; 86400/$ai_avg_sr_lb/1000000" | bc`
	my_echo ". $ai_avg_sr_lb seconds ($mega_actions_per_day GU)"

	# 1000 nodes DAG registration
	#
	my_echo "-n 1000 nodes DAG registration ..."
	ai_dag1000_lb=`$GLITE_LOCATION/sbin/glite-lb-perftest_jobreg $dest -n 1000`
	mega_actions_per_day=`echo "scale=6; 86400/$ai_dag1000_lb/1000000*1001" | bc`
	my_echo ". $ai_dag1000_lb seconds ($mega_actions_per_day GU)"

	# 5000 nodes DAG registration
	#
	my_echo "-n 5000 nodes DAG registration ..."
	ai_dag5000_lb=`$GLITE_LOCATION/sbin/glite-lb-perftest_jobreg $dest -n 5000`
	mega_actions_per_day=`echo "scale=6; 86400/$ai_dag5000_lb/1000000*5001" | bc`
	my_echo ". $ai_dag5000_lb seconds ($mega_actions_per_day GU)"

	# 10000 nodes DAG registration
	#
	my_echo "-n 10000 nodes DAG registration ..."
	ai_dag10000_lb=`$GLITE_LOCATION/sbin/glite-lb-perftest_jobreg $dest -n 10000`
	mega_actions_per_day=`echo "scale=6; 86400/$ai_dag10000_lb/1000000*10001" | bc`
	my_echo ". $ai_dag10000_lb seconds ($mega_actions_per_day GU)"

}

quick_test()
{
        dest=
        [ -z "$1" ] && echo "test_ai() - wrong params" && return
        [ "$1" = "proxy" ] && dest="-x"
        [ "$1" = "server" ] && dest="-m $BKSERVER_HOST"
        [ -z "$dest" ] && echo "test_ai() - wrong params" && return


        # 1000 nodes DAG registration
        #
        my_echo "-n 1000 nodes DAG registration ..."
        ai_dag1000_lb=`$GLITE_LOCATION/sbin/glite-lb-perftest_jobreg $dest -n 1000`
        mega_actions_per_day=`echo "scale=6; 86400/$ai_dag1000_lb/1000000*1001" | bc`
        my_echo ". $ai_dag1000_lb seconds ($mega_actions_per_day GU)"

}

proxy_test()
{
	my_echo "----------------------------------------------------------------"
	my_echo "Scenarios:"
	my_echo "0) registration only to bkserver (edg_wll_RegisterJobSync)"
	my_echo "1) registration only to lbproxy (edg_wll_RegisterJobProxyOnly)"
	my_echo "2) old (not dual) registration (edg_wll_RegisterJobProxyOld)"
	my_echo "3) dual registration (edg_wll_RegisterJobProxy)"
	my_echo ""

        if [ -n "$1" ]; then
                repeat="-N $1"
                repeated="repeated $1 times"
		scale=$1
        else
                repeat=""
                repeated=""
		scale=1
        fi

	# single registration
	#
	for i in 0 1 2 3; do
		dest="-m $BKSERVER_HOST -x $i"
		my_echo "-n single registration $repeated (scenario $i)..."
		ai_sr_lb=`$GLITE_LOCATION/sbin/glite-lb-perftest_jobreg $dest $repeat`
		mega_actions_per_day=`echo "scale=6; 86400/$ai_sr_lb/1000000*$scale" | bc`
		my_echo ". $ai_sr_lb seconds ($mega_actions_per_day GU)"
		sleep 3
		sync
		sleep 3
	done

	# 1 node DAG registration
	#
	for i in 0 1 2 3; do
		dest="-m $BKSERVER_HOST -x $i"
		my_echo "-n 1 node DAG registration $repeated (scenario $i)..."
		ai_dag1_lb=`$GLITE_LOCATION/sbin/glite-lb-perftest_jobreg $dest $repeat -n 1`
		mega_actions_per_day=`echo "scale=6; 86400/$ai_dag1_lb/1000000*2*$scale" | bc`
		my_echo ". $ai_dag1_lb seconds ($mega_actions_per_day GU)"
		sleep 3
		sync
		sleep 3
	done

	# 1000 nodes DAG registration
	#
	for i in 0 1 2 3; do
		dest="-m $BKSERVER_HOST -x $i"
		my_echo "-n 1000 nodes DAG registration $repeated (scenario $i)..."
		ai_dag1000_lb=`$GLITE_LOCATION/sbin/glite-lb-perftest_jobreg $dest $repeat -n 1000`
		mega_actions_per_day=`echo "scale=6; 86400/$ai_dag1000_lb/1000000*1001*$scale" | bc`
		my_echo ". $ai_dag1000_lb seconds ($mega_actions_per_day GU)"
		sleep 10
		sync
		sleep 10
	done

	# 10000 nodes DAG registration
	#
	for i in 0 1 2 3; do
		dest="-m $BKSERVER_HOST -x $i"
		my_echo "-n 10000 nodes DAG registration $repeated (scenario $i)..."
		ai_dag10000_lb=`$GLITE_LOCATION/sbin/glite-lb-perftest_jobreg $dest $repeat -n 10000`
		mega_actions_per_day=`echo "scale=6; 86400/$ai_dag10000_lb/1000000*10001*$scale" | bc`
		my_echo ". $ai_dag10000_lb seconds ($mega_actions_per_day GU)"
		sleep 10
		sync
		sleep 10
	done


}


################################################################################

unset creds port
if [ -z ${BKSERVER_HOST} ]; then
	HOST=`hostname -f`
	PORT=${GLITE_LB_SERVER_PORT:-9000}; 
	BKSERVER_HOST=$HOST:$PORT
fi
sink_mode[0]=GLITE_LB_SINK_NONE
sink_mode[1]=GLITE_LB_SINK_PARSE
sink_mode[2]=GLITE_LB_SINK_STORE
sink_mode[3]=GLITE_LB_SINK_STATE
sink_mode[4]=GLITE_LB_SINK_SEND

test_glite_location;
test_credentials;

#
# QUICK TEST
#
#start_bkserver 0;
#start_proxy 0;
#my_echo "================================================================"
#my_echo "Testing LB server with sink_mode ${sink_mode[0]}"
#my_echo "Testing LB proxy with sink_mode ${sink_mode[0]}"
#sleep 5
#sync
#sleep 5
#for i in `seq 1 10000`; do
#	quick_test server 0;
##	quick_test proxy 0;
#done
#stop_bkserver;
#stop_proxy;

#
# SINK TEST
#
#for i in 1 2 3 4; do
#	my_echo "================================================================"
#
#	my_echo "Testing LB proxy with sink_mode ${sink_mode[$i]}"
#	start_proxy $i
#	test_ai proxy $i;
#	stop_proxy
#
#	my_echo "Testing LB server with sink_mode ${sink_mode[$i]}"
#	start_bkserver $i;
#	test_ai server $i;
#	stop_bkserver;
#done

#
# PROXY TEST
#
#start_proxy 0;
#start_il;
my_echo "================================================================"
my_echo "Testing registrations to bkserver ($BKSERVER_HOST) and to lbproxy"
#sleep 5
for i in 1 10 100 1000; do
	proxy_test $i;
done
#stop_proxy;
#stop_il;

echo "__________"
echo "GU (goal units) are millons of registrations per day, where registration is"
echo "registration of job or subjob by client or server"
echo
