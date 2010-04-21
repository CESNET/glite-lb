#!/bin/sh

#
# an example script for getting LB dumps by gsiftp
# and removing them at by glite-gridftp-rm
#

GLITE_LOCATION=${GLITE_LOCATION:-/opt/glite}
GLITE_LOCATION_VAR=${GLITE_LOCATION_VAR:-${GLITE_LOCATION}/var}
GLITE_LB_DUMPDIR=${GLITE_LB_DUMPDIR:-${GLITE_LOCATION_VAR}/dump}

GSIFTP_SERVER=scientific.civ.zcu.cz
GSIFTP_PORT=8911
GLITE_LB_SERVER=${GSIFTP_SERVER}:${GSIFTP_PORT}
GLITE_LB_EXPORT_DUMPDIR=/home/glite/LB/export

X509_USER_KEY=~/.cert/usercert.pem
X509_USER_CERT=~/.cert/userkey.pem

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

syslog "getting new dumps to ${GLITE_LB_DUMPDIR}"
globus-url-copy gsiftp://${GLITE_LB_SERVER}/${GLITE_LB_EXPORT_DUMPDIR}/ file://${GLITE_LB_DUMPDIR}/

syslog "NOT removing dumps on the server"
# glite-gridftp-rm ...

syslog "done"
