# external dependencies:

# missing packages could be downloaded to repository from
# http://egee-jra1-integration.web.cern.ch/egee-jra1-integration/repository/
#repository=/home/honik/egee/repository

#vdt_location=/home/honik/egee/vdt-1.3.11
vdt_location=${VDT_LOCATION}


#cares_prefix=${repository}/c-ares/1.3.0/rhel30_gcc32
cares_prefix=/software/cares-1.3

#classads_prefix=${repository}/classads/0.9.6/rhel30_gcc32
classads_prefix=${vdt_location}/classads

# not needed (used "only" for unit tests - 'make check'):
#cppunit_prefix=${repository}/cppunit/1.10.2/rhel30_gcc32
cppunit_prefix=/software/cppunit-1.10.2

#expat_prefix=/usr
expat_prefix=${vdt_location}/expat

#globus_prefix=${repository}/globus/2.4.3-VDT-1.2.2/rhel30_gcc32
globus_prefix=${vdt_location}/globus

#gridsite_prefix=${stagedir}

gsoap_default_version=2.7.6b
gsoap_versions=${gsoap_default_version}
#gsoap_name=gsoap
#gsoap_platform=rhel30_gcc32
#gsoap_prefix=${repository}/${gsoap_name}/${gsoap_default_version}/${gsoap_platform}
gsoap_prefix=/software/gsoap-${gsoap_default_version}

#mysql_prefix=${repository}/mysql/4.1.11/rhel30_gcc32
mysql_prefix=${vdt_location}/mysql
mysql_version=4.1.11

voms_prefix=${vdt_location}/glite
#voms_prefix=/home/honik/egee/glite/stage


# some defaults:

#PREFIX=${vdt_location}/glite
PREFIX=/tmp/lb4vdt
globalprefix=glite
lbprefix=lb

builddir=build
distdir=${STAGEDIR}/../dist
stagedir=${STAGEDIR}
top_srcdir=..

thrflavour=gcc32dbgpthr
nothrflavour=gcc32dbg

# needed by org.glite.lb.client:
glite_location=${stagedir}

