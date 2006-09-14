#! /bin/bash

#
# script for exporing jobs from bkserver which should be periodically run
# together with running jp-importer
#
# it uses configuration from enviroment ==> may require a configuration wrapper
#

#autodetect the prefix
PREFIX=${GLITE_LOCATION:-`dirname $0`/..}

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

$PREFIX/sbin/glite-lb-purge $GLITE_LB_EXPORT_PURGE_ARGS -l -m $GLITE_LB_EXPORT_BKSERVER

for file in $GLITE_LB_EXPORT_DUMPDIR/*; do
  if [ -s $file ]; then
    $PREFIX/sbin/glite-lb-lb_dump_exporter -d $file -s $GLITE_LB_EXPORT_EXPORTDIR -m $GLITE_LB_EXPORT_JPDUMP_MAILDIR
    mv $file $GLITE_LB_EXPORT_DUMPDIR_OLD
  else
    rm $file
  fi
done
