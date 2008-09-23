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
GLITE_LB_EXPORT_PURGEDIR=${GLITE_LB_EXPORT_PURGEDIR:-$GLITE_LOCATION_VAR/purge}
GLITE_LB_EXPORT_DUMPDIR=${GLITE_LB_EXPORT_DUMPDIR:-$GLITE_LOCATION_VAR/dump}
GLITE_LB_EXPORT_PURGEDIR_KEEP=${GLITE_LB_EXPORT_PURGEDIR_KEEP:-""}
GLITE_LB_EXPORT_DUMPDIR_KEEP=${GLITE_LB_EXPORT_DUMPDIR_KEEP:-""}
# maildir dump directory for jp importer
GLITE_LB_EXPORT_JPDUMP_MAILDIR=${GLITE_LB_EXPORT_JPDUMP_MAILDIR:-$GLITE_LOCATION_VAR/jpdump}
# directory with exported data (file per job)
GLITE_LB_EXPORT_JOBSDIR=${GLITE_LB_EXPORT_JOBSDIR:-$GLITE_LOCATION_VAR/lbexport}
# purge args (timeouts)
GLITE_LB_EXPORT_PURGE_ARGS=${GLITE_LB_EXPORT_PURGE_ARGS:---cleared 2d --aborted 2w --cancelled 2w --other 60d}
# Book Keeping Server
GLITE_LB_SERVER_PORT=${GLITE_LB_SERVER_PORT:-9000}
GLITE_LB_EXPORT_BKSERVER=${GLITE_LB_EXPORT_BKSERVER:-localhost:$GLITE_LB_SERVER_PORT}
GLITE_LB_PURGE_ENABLED=${GLITE_LB_PURGE_ENABLED:-true}
GLITE_LB_EXPORT_ENABLED=${GLITE_LB_EXPORT_ENABLED:-false}

[ -d $GLITE_LB_EXPORT_JPDUMP_MAILDIR ] || mkdir -p $GLITE_LB_EXPORT_JPDUMP_MAILDIR
[ -d $GLITE_LB_EXPORT_DUMPDIR ] || mkdir -p $GLITE_LB_EXPORT_DUMPDIR
[ -d $GLITE_LB_EXPORT_PURGEDIR ] || mkdir -p $GLITE_LB_EXPORT_PURGEDIR
[ -d $GLITE_LB_EXPORT_DUMPDIR_KEEP ] || mkdir -p $GLITE_LB_EXPORT_DUMPDIR_KEEP
[ -d $GLITE_LB_EXPORT_PURGEDIR_KEEP ] || mkdir -p $GLITE_LB_EXPORT_PURGEDIR_KEEP
[ -d $GLITE_LB_EXPORT_JOBSDIR ] || mkdir -p $GLITE_LB_EXPORT_JOBSDIR

if [ x"$GLITE_LB_EXPORT_ENABLED" = x"true"  -o -d "$GLITE_LB_EXPORT_PURGEDIR_KEEP" ]
then
	GLITE_LB_EXPORT_PURGE_ARGS="$GLITE_LB_EXPORT_PURGE_ARGS -s"
fi

if [ x"$GLITE_LB_PURGE_ENABLED" = x"true" ]; then
	X509_USER_CERT="$X509_USER_CERT" X509_USER_KEY="$X509_USER_KEY" $PREFIX/bin/glite-lb-purge $GLITE_LB_EXPORT_PURGE_ARGS -l -m $GLITE_LB_EXPORT_BKSERVER $GLITE_LB_PURGE_OTHER_OPTIONS
fi

  find  $GLITE_LB_EXPORT_PURGEDIR/ -type f -name purge_\* 2>/dev/null | \
  while read file; do
    if [ -s $file ]; then
	if [ x"$GLITE_LB_EXPORT_ENABLED" = x"true" ]; then
      		$PREFIX/bin/glite-lb-dump_exporter -d $file -s $GLITE_LB_EXPORT_JOBSDIR -m $GLITE_LB_EXPORT_JPDUMP_MAILDIR $GLITE_LB_DUMP_EXPORTER_OTHER_OPTIONS
	fi
      if [ -n "$GLITE_LB_EXPORT_PURGEDIR_KEEP" ]; then
        mv $file $GLITE_LB_EXPORT_PURGEDIR_KEEP
      else
        rm $file
      fi
    else
      rm $file
    fi
  done

if [ x"$GLITE_LB_EXPORT_ENABLED" = x"true" ]; then
  if [ -n "$GLITE_LB_EXPORT_DUMPDIR_KEEP" ]; then
    ls $GLITE_LB_EXPORT_DUMPDIR | xargs  -i'{}' cp $GLITE_LB_EXPORT_DUMPDIR/'{}' $GLITE_LB_EXPORT_DUMPDIR_KEEP;
  else
    ls $GLITE_LB_EXPORT_DUMPDIR | xargs -i'{}' rm -f $GLITE_LB_EXPORT_DUMPDIR/'{}'
  fi
fi
