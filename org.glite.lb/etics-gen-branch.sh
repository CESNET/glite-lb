#! /bin/sh

SUB=$1
BRANCH=${2:-'HEAD'}
PROJECT=${3:-'emi'}
URL=${URL:-"http://scientific.zcu.cz/emi/$BRANCH/\${moduleName}"}

if test -z "$1"; then
	cat <<EOF
Usage: $0 SUBSYSTEM [BRANCH] [PROJECT]

SUBSYSTEM .. jobid|lbjp-common|lb|canl|gridsite|px (required)
BRANCH ..... cvs branch (default: HEAD)
PROJECT .... glite|emi1|emi2|emi3|emi (default: emi)
EOF
	exit 0
fi

for c in `org.glite.lb/configure --project=$PROJECT --listmodules=$SUB | sed -e 's/\(emi\|org.glite\)\.//g' -e 's/org\.gridsite/gridsite/'`; do
	for m in $c `org.glite.lb/configure --project=$PROJECT --listmodules=$c`; do
		echo $m
		org.glite.lb/configure --project=$PROJECT --module=$m --mode=etics --branch=$BRANCH --url "$URL"
	done
done
