# Default values
top_srcdir=..
stagedir=.
globalprefix=glite
jobidprefix=jobid
package=glite-jobid-api-cpp
version=0.0.0

CC:=gcc
CXX:=g++

-include Makefile.inc

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/interface:${top_srcdir}/test

DEBUG:=-g -O0 -Wall

CFLAGS:=${DEBUG} \
	-I${top_srcdir}/interface -I${top_srcdir}/src \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
CXXCOMPILE:=libtool --mode=compile ${CXX} ${CXXFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
INSTALL:=libtool --mode=install install

LIBOBJS:=
HDRS:=JobId.h

LIBLOBJS:=${LIBOBJS:.o=.lo}

LIB:=

compile all: 


check: compile
#

clean:
	rm -rvf *.o *.lo .libs lib* *.c *.cpp *.h
	rm -rvf log.xml rpmbuild/ RPMS/ tgz/ debian/

install: 
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${jobidprefix}
	(cd ${top_srcdir}/interface; ${INSTALL} -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${jobidprefix})

stage:
	${MAKE} install PREFIX=${stagedir} staging=1

%.o: %.c
	${COMPILE} -o $@ -c $< 


.PHONY: default all compile check install clean
