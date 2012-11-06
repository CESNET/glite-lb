top_srcdir=..
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
lbutilsprefix=lbu
package=glite-lbjp-common-log
version=0.0.0
PREFIX=/opt/glite
prefix=
sysconfdir=/etc
log4c_prefix=/usr

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

CC=gcc
VPATH=${top_srcdir}/interface:${top_srcdir}/src:${top_srcdir}/tests

SOURCES=\
	config/log4crc* \
	project/.post* \
	src/*.c src/*.h \
	tests/*.c tests/log4crc \
	Makefile README

LOG4C_CFLAGS:=-I${log4c_prefix}/include
LOG4C_LIBS:=-L${log4c_prefix}/${libdir} -L${log4c_prefix}/lib -llog4c

DEBUG:=-g -O0 -W -Wall

CFLAGS:= ${CFLAGS} \
	${DEBUG} \
	-I${stagedir}${prefix}/include -I${top_srcdir}/src \
	-I${top_srcdir}/interface \
	${LOG4C_CFLAGS}

LDFLAGS:=${LDFLAGS} -L${stagedir}${prefix}/${libdir}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

OBJS:=log.o
LOBJS:=${OBJS:.o=.lo}
LTLIB:=libglite_lbu_log.la

# In order to use libtool versioning correcty, we must have:
#
# current = major + minor + offset
# revision = patch
# age = minor
#
# where offset is a sum of maximal released minor's of all previous major's
# 

# counted minors: n/a
offset=0

version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

all: compile

compile: ${LTLIB}

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install: compile
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbutilsprefix} 
	${INSTALL} -m 644 ${top_srcdir}/src/log.h ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbutilsprefix} 
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${top_srcdir}/README ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	mkdir -p ${DESTDIR}${PREFIX}${sysconfdir}/glite-lb
	${INSTALL} -m 644 ${top_srcdir}/config/log4crc ${DESTDIR}${PREFIX}${sysconfdir}/glite-lb/log4crc
	${INSTALL} -m 644 ${top_srcdir}/config/log4crc ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}/log4crc.example-production
	${INSTALL} -m 644 ${top_srcdir}/config/log4crc.debugging ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}/log4crc.example-debugging
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	${INSTALL} -m 755 ${LTLIB} ${DESTDIR}${PREFIX}${prefix}/${libdir}/${LTLIB}

clean:
	rm -rvf *.o *.lo .libs lib*
	rm -rvf test
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf Makefile.inc *.spec debian/

check: test
	LOG4C_RCPATH=${top_srcdir}/tests ./test

${LTLIB}: ${LOBJS}
	${LINK} ${version_info} -o $@ $+ ${LOG4C_LIBS}

test: test.o
	${LINK} -o $@ $+ ${LTLIB} ${LOG4C_LIBS}

%.o %.lo: %.c
	${COMPILE} -c $<

.PHONY: default all compile stage install check clean distclean dist distcheck
