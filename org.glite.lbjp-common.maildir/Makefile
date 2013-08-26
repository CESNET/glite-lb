# defaults
top_srcdir=..
builddir=build
stagedir=.
distdir=.
globalprefix=glite
lbutilsprefix=lbu
package=glite-lbjp-common-maildir
version=1.0.0
PREFIX=/opt/glite
prefix=

glite_location=/opt/glite
cppunit_prefix=/opt/cppunit
thrflavour=gcc32pthr
nothrflavour=gcc32

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

CC=gcc

VPATH=${top_srcdir}/interface:${top_srcdir}/src:${top_srcdir}/examples

SOURCES=interface/*.h project/.post* src/*.c Makefile

DEBUG:=-g -O0 -W -Wall

CFLAGS:= \
	${DEBUG} \
	-DVERSION=\"${version}\" \
	-I${stagedir}${prefix}/include -I${top_srcdir}/src -I. \
	-I${top_srcdir}/interface \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE ${CFLAGS}

LDFLAGS:=${LDFLAGS} -L${stagedir}${prefix}/${libdir} ${COVERAGE_FLAGS}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

EXT_LIBS:=
OBJS:=maildir.o
HDRS:=maildir.h
LOBJS:=${OBJS:.o=.lo}

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

all: compile doc

libglite_lbu_maildir.la: ${LOBJS}
	${LINK} ${version_info} -o $@ $< ${EXT_LIBS}

compile: libglite_lbu_maildir.la

check:
	-echo No checks here yet.

test_coverage:
	-mkdir coverage
	cd coverage && $(MAKE) -f ../Makefile top_srcdir=../../ COVERAGE_FLAGS="-fprofile-arcs -ftest-coverage" check
	cd coverage && for i in `echo ${OBJS} | tr ' ' '\012' | sort -u`; do gcov $$i ; done

examples:

#doc:
#	cp ${top_srcdir}/doc/*.dox .
#	echo "PROJECT_NUMBER = ${version}" >> C.dox
#	doxygen C.dox

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

install: all
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbutilsprefix}
	${INSTALL} -m 755 "libglite_lbu_maildir.la" "${DESTDIR}${PREFIX}${prefix}/${libdir}/libglite_lbu_maildir.la"; \
	${INSTALL} -m 644 ${top_srcdir}/interface/${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbutilsprefix}

clean:
	rm -rvf *.o *.lo .libs lib* *.c *.h *.dox C/ CPP/
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

%.o %.lo: %.c
	${COMPILE} -c $<

maildir.lo: maildir.c maildir.h

.PHONY: default all compile check examples doc stage dist distsrc distbin install clean distclean test_coverage dist distcheck
