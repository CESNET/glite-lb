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

export CVSROOT=:pserver:anonymous@jra1mw.cvs.cern.ch:/cvs/jra1mw
#export CVSROOT=:ext:jpospi@jra1mw.cvs.cern.ch:/cvs/jra1mw

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
        if  [ -f ${LB4VDTDIR}/scripts/$i.build ]; then
    	echo "Building"
            sh -x ${LB4VDTDIR}/scripts/$i.build 
        fi
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
        echo "Entering directory ${TOPDIR}/$i"
        cd ${TOPDIR}/$i
        echo "Copying supporting files"
        cp -rv ${TOPDIR}/org.glite.lb/project/{at3,*.T,*.pm} ./project/    
        mkdir -p build
        echo "Entering directory ${TOPDIR}/$i/build"
        cd build 
        ln -fsv ../Makefile Makefile
#        ln -fsv ../../Makefile.inc Makefile.inc
        ln -fsv ${LB4VDTDIR}/Makefile.inc 
        echo "Building"    
	make LB_STANDALONE=yes
        make stage LB_STANDALONE=yes
    else
        echo "WARNING: directory $i not found"
    fi
    echo "Done"    
done

cd ${TOPDIR}
