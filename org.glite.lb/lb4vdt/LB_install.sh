#!/bin/sh

set -e

#OFFLINE=true
TOPDIR=${PWD}

export LB4VDTDIR=${TOPDIR}/org.glite.lb/lb4vdt
export STAGEDIR=${TOPDIR}/stage

if [ ! -f ${LB4VDTDIR}/Makefile.inc ]; then
   echo "Error: There is no ${LB4VDTDIR}/Makefile.inc. Exiting."
   exit 1
fi

if [ -z "${CVSROOT}" ]; then
	export CVSROOT=:pserver:anonymous@jra1mw.cvs.cern.ch:/cvs/jra1mw
#	export CVSROOT=:ext:jpospi@jra1mw.cvs.cern.ch:/cvs/jra1mw
	echo "Using CVSROOT=${CVSROOT}"
fi

dep_modules="org.glite.wms-utils.jobid
org.gridsite.core"

modules="org.glite.security.gsoap-plugin
org.glite.lb.client-interface
org.glite.lb.common
org.glite.lb.client
org.glite.lb.logger
org.glite.lb.ws-interface
org.glite.lb.server-bones
org.glite.lb.server
org.glite.lb.proxy"
#org.glite.lb.utils

for i in $dep_modules; 
do 
    echo "*********************************************************"
    echo "*  Module $i"
    echo "*********************************************************"
    cd ${TOPDIR}
    if [ -n "${OFFLINE}" ]; then
        echo "Working offline"
    else
        echo "Getting sources from CVS"
        cvs co -A $i;
    fi 
    if [ -d $i -a -f ${LB4VDTDIR}/patches/$i.patch -a ! -f .$i.patched ]; then
        echo "Patching $i"
        patch -p0 < ${LB4VDTDIR}/patches/$i.patch
	touch .$i.patched
    fi
    if [ -d $i ]; then
	touch .$i.timestamp
        if  [ -f ${LB4VDTDIR}/scripts/$i.build ]; then
    	echo "Building"
            sh -x ${LB4VDTDIR}/scripts/$i.build 
        fi
	cd ${TOPDIR}
	find ${STAGEDIR} -newer .$i.timestamp > .$i.filelist
    else
        echo "WARNING: directory $i not found"
    fi
done

for i in $modules; 
do 
    echo "*********************************************************"
    echo "*  Module $i"
    echo "*********************************************************"
    cd ${TOPDIR}
    if [ -n "${OFFLINE}" ]; then
        echo "Working offline"
    else
        echo "Getting sources from CVS"
        cvs co -A $i; 
    fi 
    if [ -d $i -a -f ${LB4VDTDIR}/patches/$i.patch -a ! -f .$i.patched ]; then
        echo "Patching $i"
        patch -p0 < ${LB4VDTDIR}/patches/$i.patch
	touch .$i.patched
    fi
    if [ -d $i ]; then
	touch .$i.timestamp
        echo "Entering directory ${TOPDIR}/$i"
        cd ${TOPDIR}/$i
        echo "Copying supporting files"
        cp -rv ${TOPDIR}/org.glite.lb/project/{at3,*.T,*.pm} ./project/    
        mkdir -p build
        echo "Entering directory ${TOPDIR}/$i/build"
        cd build 
        ln -fsv ../Makefile 
#        ln -fsv ../../Makefile.inc Makefile.inc
        ln -fsv ${LB4VDTDIR}/Makefile.inc 
        echo "Building"    
	make LB_STANDALONE=yes
        make stage LB_STANDALONE=yes
	cd ${TOPDIR}
	find ${STAGEDIR} -newer .$i.timestamp > .$i.filelist
    else
        echo "WARNING: directory $i not found"
    fi
    echo "Done"    
done

cd ${TOPDIR}
echo "Creating filelists"
cat .org.glite.wms-utils.jobid.filelist .org.gridsite.core.filelist .org.glite.security.gsoap-plugin.filelist .org.glite.lb.common.filelist | sort | uniq > LB-common.filelist
cat .org.glite.lb.client-interface.filelist .org.glite.lb.client.filelist | sort | uniq > LB-client.filelist
cat .org.glite.lb.logger.filelist | sort | uniq > LB-logger.filelist
cat .org.glite.lb.logger.filelist .org.glite.lb.server-bones.filelist .org.glite.lb.proxy.filelist | sort | uniq > LB-proxy.filelist
cat .org.glite.lb.ws-interface.filelist .org.glite.lb.server-bones.filelist .org.glite.lb.server.filelist | sort | uniq > LB-server.filelist
