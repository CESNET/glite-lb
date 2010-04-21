#!/bin/sh

#
# an example script for processing LB dumps for the statistics purposes
# suitable for running from crontab
#

GLITE_LOCATION=${GLITE_LOCATION:-/opt/glite}
GLITE_LOCATION_VAR=${GLITE_LOCATION_VAR:-${GLITE_LOCATION}/var}
GLITE_LB_DUMPDIR=${GLITE_LB_DUMPDIR:-${GLITE_LOCATION_VAR}/dump}
GLITE_LB_STATISTICS=${GLITE_LOCATION}/bin/glite-lb-statistics

progname=`basename $0`

function syslog () {
	if [ ! -z "$*" ]; then
		echo `date +'%b %d %H:%M:%S'` `hostname` $progname: $*
	fi
}

if [ ! -d ${GLITE_LB_DUMPDIR} ]; then
	syslog "Creating directory ${GLITE_LB_DUMPDIR}"
	mkdir -vp ${GLITE_LB_DUMPDIR} || exit 1
fi   

if [ ! -f ${GLITE_LB_STATISTICS} ]; then
	syslog "Program ${GLITE_LB_STATISTICS} is missing"
	exit 1
fi

syslog "processing new dumps in ${GLITE_LB_DUMPDIR}"
for file in ${GLITE_LB_DUMPDIR}/*[^xml,log] ; do
	if [ ! -s $file.xml ]; then
		if [ -s $file ]; then
			${GLITE_LB_STATISTICS} -v -f $file > $file.xml 2> $file.log
			syslog "processed $file"
			let num++
		else
			syslog `rm -v -f $file*`	
		fi
#	else
#		syslog "file $file.xml already exists"
	fi
done

if [ -z $num ]; then
	syslog "processed no files"
else
	syslog "processed $num files"
fi
