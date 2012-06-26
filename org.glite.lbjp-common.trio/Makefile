# Default values
top_srcdir=..
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
package=glite-lbjp-common-trio
version=0.0.0
PREFIX=/opt/glite
lbuprefix=lbu
prefix=

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/test::${top_srcdir}/doc

CC=gcc

DEBUG:=-g -O0 -Wall

CFLAGS:=${DEBUG} -I${top_srcdir}/src -I${top_srcdir}/interface -I. -DDATAGRID_EXTENSION ${CFLAGS}

COMPILE:=libtool --mode=compile ${CC}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

SOURCES=\
	interface/*.h \
	project/.post* \
	src/*.c src/*.h \
	test/*.cpp \
	LICENSE Makefile

os=${shell uname}
DL_LIBS:=
ifeq (${os},Linux)
  DL_LIBS:=-ldl
endif

REPORTS:=${top_srcdir}/reports
TEST_LIBS:=-L${cppunit_prefix}/${libdir} -lcppunit ${DL_LIBS}
TEST_INC:=-I${cppunit_prefix}/include


OBJS:=trio.o strio.o escape.o
LOBJS:=${OBJS:.o=.lo}

HDRS:=trio.h escape.h

STATICLIB:=libglite_lbu_trio.a
LTLIB:=libglite_lbu_trio.la

default: all

all compile: ${STATICLIB} ${LTLIB}

# to use libtool versioning correcty, we should have:
#
# current = major + minor + offset
# revision = patch
# age = minor
#
# where offset is a sum of maximal released minor's of all previous major's
# 
# version_info=-version-info `echo ${version} | cut -d. -f1,2 | tr . :`

# counted minors: n/a
offset=0

version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

${STATICLIB}: ${OBJS}
	ar crv $@ ${OBJS}
	ranlib $@

${LTLIB}: ${OBJS}
	${LINK} ${version_info} -o $@ ${LOBJS} -lm

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

check: compile mkreports runtest

mkreports:
	-mkdir ${REPORTS}

runtest: trio_test
	./trio_test ${REPORTS}/trio.xml

trio_test: trio_test.cpp
	${CXX} -c ${CFLAGS} ${TEST_INC} $<
	${LINKXX} -o $@ trio_test.o ${LTLIB} ${TEST_LIBS}

install: all
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbutilsprefix}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbuprefix}
	${INSTALL} -m 755 ${LTLIB} ${DESTDIR}${PREFIX}${prefix}/${libdir}
	${INSTALL} -m 644 ${STATICLIB} ${DESTDIR}${PREFIX}${prefix}/${libdir}
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	(cd ${top_srcdir}/interface && ${INSTALL} -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbuprefix})

clean:
	rm -rvf *.o *.lo .libs lib* *.c *.h *.dox C/ CPP/
	rm -vf trio_test
	rm -rvf dist ${package}-*.tar.gz
	
distclean:
	rm -rvf Makefile.inc *.spec debian/

%.lo %.o: %.c
	${COMPILE} ${CFLAGS} -c $<

.PHONY: default all compile stage check mkreports runtest install clean distclean dist distcheck
