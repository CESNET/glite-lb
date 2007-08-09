# Default values
top_srcdir=.
stagedir=.
distdir=.
globalprefix=glite
jobidprefix=jobid
package=glite-jobid-api-c
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

LIBOBJS:=cjobid.o strmd5.o
HDRS:=cjobid.h

LIBLOBJS:=${LIBOBJS:.o=.lo}

LIB:=libglite_jobid.la

compile all: ${LIB}

${LIB}: ${LIBOBJS}
	${LINK} -o $@ ${LIBLOBJS} -rpath ${PREFIX}/lib 


check: compile
#

clean:
	rm -rvf *.o *.lo .libs lib* *.c *.cpp *.h
	rm -rvf log.xml project rpmbuild RPMS tgz

install: ${LIB}
	mkdir -p ${PREFIX}/lib
	mkdir -p ${PREFIX}/include/${globalprefix}/${jobidprefix}
	${INSTALL} -m 644 ${LIB} ${PREFIX}/lib
	${INSTALL} -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${jobidprefix}


%.o: %.c
	${COMPILE} -o $@ -c $< 


.PHONY: default all compile check install clean
