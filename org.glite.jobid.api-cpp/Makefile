# Default values
top_srcdir=..
stagedir=.
globalprefix=glite
jobidprefix=jobid
package=glite-jobid-api-cpp
version=0.0.0

CC:=gcc
CXX:=g++

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/interface:${top_srcdir}/test

INSTALL:=install

SOURCES=interface/*.h Makefile

HDRS:=JobId.h

compile all: 


check: compile
#

clean:
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf Makefile.inc *.spec debian/

install: 
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${jobidprefix}
	(cd ${top_srcdir}/interface; ${INSTALL} -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${jobidprefix})

stage:
	${MAKE} install PREFIX=${stagedir} staging=1

.PHONY: default all compile check install clean distclean dist distcheck
