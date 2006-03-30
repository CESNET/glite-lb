#! /bin/bash

#
# script for exporing jobs from bkserver which should be periodically run
# together with running jp-importer
#
# it uses configuration from enviroment ==> may require a configuration wrapper
#

#autodetect the prefix
PREFIX=${GLITE_LOCATION:-`dirname $0`/..}

if [ -n "$GLITE_HOST_CERT" -a -n "$GLITE_HOST_KEY" ] ;then
	creds="-c '$GLITE_HOST_CERT' -k '$GLITE_HOST_KEY'"
	X509_USER_CERT="$GLITE_HOST_CERT"
	X509_USER_KEY="$GLITE_HOST_KEY"
fi
if test -z "$creds"; then
	if su - $GLITE_USER -c "test -r /etc/grid-security/hostkey.pem -a -r /etc/grid-security/hostcert.pem"; then
		echo "$0: WARNING: /etc/grid-security/hostkey.pem readable by $GLITE_USER"
		creds="-c /etc/grid-security/hostcert.pem -k /etc/grid-security/hostkey.pem"
		X509_USER_CERT=/etc/grid-security/hostcert.pem
		X509_USER_KEY=/etc/grid-security/hostkey.pem
	fi
fi


[ -z "$creds" ] && echo $0: WARNING: No credentials specified. Using default lookup which is dangerous. >&2



# dump directory of bkserver
GLITE_LB_EXPORT_DUMPDIR=${GLITE_LB_EXPORT_DUMPDIR:-/tmp/dump}
GLITE_LB_EXPORT_DUMPDIR_OLD=${GLITE_LB_EXPORT_DUMPDIR_OLD:-$GLITE_LB_EXPORT_DUMPDIR.old}
# maildir dump directory for jp importer
GLITE_LB_EXPORT_JPDUMP_MAILDIR=${GLITE_LB_EXPORT_JPDUMP_MAILDIR:-/tmp/jpdump}
# directory with exported data (file per job)
GLITE_LB_EXPORT_EXPORTDIR=${GLITE_LB_EXPORT_EXPORTDIR:-/tmp/lbexport}
# purge args (timeouts)
GLITE_LB_EXPORT_PURGE_ARGS=${GLITE_LB_EXPORT_PURGE_ARGS:--a 1h -c 1h -n 1h -o 1d}
# Book Keeping Server
GLITE_LB_EXPORT_BKSERVER=${GLITE_LB_EXPORT_BKSERVER:-localhost:9000}

[ -d $GLITE_LB_EXPORT_JPDUMP_MAILDIR ] || mkdir -p $GLITE_LB_EXPORT_JPDUMP_MAILDIR
[ -d $GLITE_LB_EXPORT_DUMPDIR ] || mkdir -p $GLITE_LB_EXPORT_DUMPDIR
[ -d $GLITE_LB_EXPORT_DUMPDIR_OLD ] || mkdir -p $GLITE_LB_EXPORT_DUMPDIR_OLD
[ -d $GLITE_LB_EXPORT_EXPORTDIR ] || mkdir -p $GLITE_LB_EXPORT_EXPORTDIR

X509_USER_CERT="$X509_USER_CERT" X509_USER_KEY="$X509_USER_KEY" $PREFIX/sbin/glite-lb-purge $GLITE_LB_EXPORT_PURGE_ARGS -l -m $GLITE_LB_EXPORT_BKSERVER

for file in $GLITE_LB_EXPORT_DUMPDIR/*; do
  if [ -s $file ]; then
    $PREFIX/sbin/glite-lb-lb_dump_exporter -d $file -s $GLITE_LB_EXPORT_EXPORTDIR -m $GLITE_LB_EXPORT_JPDUMP_MAILDIR
    mv $file $GLITE_LB_EXPORT_DUMPDIR_OLD
  else
    rm $file
  fi
done
