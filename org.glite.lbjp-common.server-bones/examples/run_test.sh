#! /bin/bash

./srv_example > /tmp/log.$$ &
SRV_PID=$!
disown $SRV_PID

sleep 1

a=`./cnt_example -p 9999 -m "Applejack
"`
ret1=$?

b=`./cnt_example -p 9998 -m "Applejack
"`
ret2=$?

kill -SIGTERM $SRV_PID

n=`grep 'disconnect handler' /tmp/log.$$ | wc -l | sed 's/ //g'`
rm -f /tmp/log.$$

if [ $ret1 -ne 0 -o $ret2 -ne 0 ]; then
	echo "$0: error launching cnt_example"
	exit 1
fi

if [ "$a" != "reply: Applejack" -o "$b" != "reply: APPLEJACK" ]; then
	echo "$0: error in reply"
	exit 2
fi

if [ "$n" != "2" ]; then
	echo "$0: error running server"
	exit 3
fi

exit 0
