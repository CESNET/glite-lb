#!/bin/sh

#
# an example script for getting LB dumps using sftp
# (it does not remove files on the LB server!)
#

GLITE_LOCATION=${GLITE_LOCATION:-/opt/glite}
GLITE_LOCATION_VAR=${GLITE_LOCATION_VAR:-${GLITE_LOCATION}/var}
GLITE_LB_DUMPDIR=${GLITE_LB_DUMPDIR:-${GLITE_LOCATION_VAR}/dump}

GLITE_LB_SFTPBATCH=/tmp/glite-lb-statistics-sftp-batch

GLITE_LB_SERVER=scientific.civ.zcu.cz
GLITE_LB_EXPORT_DUMPDIR=/home/glite/LB/export

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

if [ -f ${GLITE_LB_SFTPBATCH} ]; then
        syslog "WARNING: File ${GLITE_LB_SFTPBATCH} already exists, will be overwritten"
fi

syslog "writing ${GLITE_LB_SFTPBATCH}"
echo "lcd ${GLITE_LB_DUMPDIR}
cd ${GLITE_LB_EXPORT_DUMPDIR}
get -P *" > ${GLITE_LB_SFTPBATCH} || exit 1

syslog "getting new dumps to ${GLITE_LB_DUMPDIR}"
sftp -b ${GLITE_LB_SFTPBATCH} ${GLITE_LB_SERVER}

syslog "done"
