#! /bin/sh -e

#
# the latest numbered release from:
#   koji list-targets
#
TARGET="f21"

if [ -z "$1" ]; then
	cat << EOF
Helper script preparing RPM packages for releasing to Fedora/EPEL.

Known differences are applied and upstream packaging is compared with the
current version in Fedora.

Manual editing of the spec file is still needed:
- remove --thrflavour= --nothrflavour= --project
- update changelog
- remove Obsoletes, Provides not relevant for Fedora
- check comments with "EMI"

Usage:
  cd MODULE_DIR
  `basename $0` SUBSYTEM.MODULE [AGE]
EOF
	exit 0
fi
if [ ! -x ./configure ]; then
	$0
	echo
	echo "./configure not found!"
	exit 1
fi
which vimdiff >/dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "vimdiff not found"
	exit 1
fi

if [ -z "$2" ]; then
	age=1
else
	age=$2
fi
short=$1
moduledir="emi.$short"

make clean distclean
./configure --module=$short

p_base=`ls -1 *.spec | head -n 1 | sed 's/\.spec//'`
p_ver=`grep 'version=' project/version.properties | cut -f2 -d=`
SOURCE_URL=http://scientific.zcu.cz/emi/$moduledir
FEDORA_GIT_URL=http://pkgs.fedoraproject.org/cgit/${p_base}.git/plain/${p_base}.spec
DOWNSTREAM_SUF=".fedora"
RPM_DIR=`rpm --eval='%_topdir'`

rm -f $SOURCE_URL/${p_base}-${p_ver}.tar.gz
wget -nv $SOURCE_URL/${p_base}-${p_ver}.tar.gz

mv ${p_base}.spec ${p_base}.spec.orig
cat ${p_base}.spec.orig | awk '/^%clean/ {getline; getline; getline; next;} {print $0;}' | grep -v '^rm -rf $RPM_BUILD_ROOT' | grep -v '^rm -rf %{buildroot}' | grep -v '^BuildRoot:' | grep -v ^\%defattr | grep -v '^Vendor:' | grep -v ^Group | grep -v '\<chrpath\>' > ${p_base}.spec

if [ ! -f ${p_base}.spec${DOWNSTREAM_SUF} ]; then
	echo "${p_base}.spec${DOWNSTREAM_SUF} not found"
	echo "Download from $FEDORA_GIT_URL?"
	echo "(Y/N) (CTRL-C for quit)"
	read x
	if [ x"$x" = x"Y" -o x"$x" = x"y" ]; then
		wget -nv $FEDORA_GIT_URL -O ${p_base}.spec${DOWNSTREAM_SUF} || :
		# spec file or not found error page?
		grep '^Summary:' ${p_base}.spec${DOWNSTREAM_SUF} || exit 1
		grep '^%changelog' ${p_base}.spec${DOWNSTREAM_SUF} || exit 1
	fi
fi
if [ -f ${p_base}.spec${DOWNSTREAM_SUF} ]; then
	# copy original changelog
	(echo; cat ${p_base}.spec${DOWNSTREAM_SUF} | grep  '%changelog' -A9999 | tail -n +2) >> ${p_base}.spec
	# compare + edit
	vimdiff ${p_base}.spec${DOWNSTREAM_SUF} ${p_base}.spec
	# show results
	(echo "previous downstream -> new"; diff -up ${p_base}.spec${DOWNSTREAM_SUF} ${p_base}.spec) | less -I
else
	vim ${p_base}.spec
fi

rm -f $RPM_DIR/SOURCES/${p_base}-${p_ver}.tar.gz
cp -p `pwd`/${p_base}-${p_ver}.tar.gz $RPM_DIR/SOURCES/

cat << EOF
# Now launch:

rpmbuild --nodeps -bs ${p_base}.spec

#rpmbuild --rebuild $RPM_DIR/SRPMS/${p_base}-${p_ver}-${age}*.src.rpm
mock --rebuild  $RPM_DIR/SRPMS/${p_base}-${p_ver}-${age}*.src.rpm

koji build --scratch $TARGET $RPM_DIR/SRPMS/${p_base}-${p_ver}-${age}*.src.rpm

# And update package according to:
# http://fedoraproject.org/wiki/Join_the_package_collection_maintainers#Check_out_the_module
EOF
