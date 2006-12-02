#
# common setting
#

#vdt_location=/home/honik/egee/vdt-1.5.0
vdt_location=${VDT_LOCATION}

# missing packages could be for example downloaded to repository from
# http://eticssoft.web.cern.ch/eticssoft/repository/externals

repository=/home/honik/egee/repository/externals/
platform=slc3_ia32_gcc323


#
# external dependencies that are already part of VDT (vdt-1.5.0):
#

#classads_prefix=${repository}/classads/0.9.8/${platform}
classads_prefix=${vdt_location}/classads

#globus_prefix=${repository}/globus/2.4.3-VDT-1.2.5/${platform}
globus_prefix=${vdt_location}/globus

#expat_prefix=/usr
expat_prefix=${vdt_location}/expat

#mysql_prefix=${repository}/mysql-devel/4.1.11/${platform}
mysql_prefix=${vdt_location}/mysql
mysql_version=4.1.21

voms_prefix=${vdt_location}/glite
#voms_prefix=/home/honik/egee/glite/stage


#
# external dependencies that are NOT (yet) part of VDT (vdt-1.5.0):
#

#cares_prefix=${repository}/c-ares/1.3.0/${platform}
cares_prefix=/software/cares-1.3

# probably not needed (used "only" for unit tests - 'make check'):
#cppunit_prefix=${repository}/cppunit/1.10.2/${platform}
cppunit_prefix=/software/cppunit-1.10.2

# probably not needed:
#gridsite_prefix=${stagedir}

gsoap_default_version=2.7.6b
gsoap_versions=${gsoap_default_version}
#gsoap_prefix=${repository}/gsoap/${gsoap_default_version}/${platform}
gsoap_prefix=/software/gsoap-${gsoap_default_version}


#
# some other defaults:
#

#PREFIX=${vdt_location}/glite
PREFIX=/tmp/lb4vdt
globalprefix=glite
lbprefix=lb

builddir=build
distdir=${STAGEDIR}/../dist
stagedir=${STAGEDIR}
top_srcdir=..

#thrflavour=gcc64dbgpthr
#nothrflavour=gcc64dbg
thrflavour=gcc32dbgpthr
nothrflavour=gcc32dbg

# needed by org.glite.lb.client:
glite_location=${stagedir}

