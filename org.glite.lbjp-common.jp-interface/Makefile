# defaults
top_srcdir=..
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
globalprefix=glite
jpprefix=jp
package=glite-lbjp-common-jp-interface
version=0.0.0
PREFIX=/opt/glite
prefix=

globus_prefix=/opt/globus
expat_prefix=/opt/expat
gsoap_prefix=/software/gsoap-2.6

CC=gcc

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/project:${jpproject}

SOURCES=interface/*.h project/.post* src/*.c test/*.cpp Makefile

DEBUG:=-g -O0 -W -Wno-sign-compare
CFLAGS:=${DEBUG} -D_GNU_SOURCE -I. -I${top_srcdir}/interface -I${stagedir}${prefix}/include ${CFLAGS}

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


LINK:=libtool --mode=link ${CC} ${LDFLAGS} -rpath ${stagedir}${prefix}/${libdir} ${version_info}
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install
COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}

HDRS:=types.h context.h attr.h known_attr.h backend.h builtin_plugins.h file_plugin.h indexdb.h type_plugin.h

SRCS:=context.c attr.c utils.c indexdb.c
OBJS:=${SRCS:.c=.lo}
THROBJS:=${OBJS:.o=.thr.lo}
LIBS:=-L${stagedir}${prefix}/${libdir} -lglite_jobid -lglite_lbu_db
THRLIBS:=${LIBS}

CPPUNIT_LIBS?=-L${cppunit_prefix}/${libdir} -lcppunit -ldl
CPPUNIT_CFLAGS?=-I${cppunit_prefix}/include

commonlib_nothr:= libglite_jp_common_${nothrflavour}.la
commonlib_thr:= libglite_jp_common_${thrflavour}.la
commonlib:= libglite_jp_common.la
ifeq (${thrflavour},)
lib_LTLIBRARIES=${commonlib}
default_commonlib=${commonlib}
else
lib_LTLIBRARIES=${commonlib_nothr} ${commonlib_thr}
default_commonlib=${commonlib_nothr}
endif

all: compile

compile: ${lib_LTLIBRARIES}

ifeq (${thrflavour},)
${commonlib}: ${OBJS}
	${LINK} -o $@ ${OBJS} ${LIBS}
else
${commonlib_nothr}: ${OBJS}
	${LINK} -o $@ ${OBJS} ${LIBS}

${commonlib_thr}: ${THROBJS}
	${LINK} -o $@ ${THROBJS} ${THRLIBS}
endif

check: type_test
	LD_LIBRARY_PATH=${log4c_prefix}/${libdir}:${log4c_prefix}/lib:${LD_LIBRARY_PATH} ./type_test type_test.xml

type_test: %: %.cpp compile
	${CXX} -c ${CFLAGS} ${CPPUNIT_CFLAGS} $<
	${LINKXX} -o $@ $@.o ${default_commonlib} ${CPPUNIT_LIBS} 

doc:

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install:
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${jpprefix}
	cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${jpprefix}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	${INSTALL} -m 755 ${lib_LTLIBRARIES} ${DESTDIR}${PREFIX}${prefix}/${libdir}

clean:
	rm -rvf *.o *.lo .libs lib*
	rm -vf type_test type_test.xml
	rm -f glite jp
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

%.thr.lo: %.c
	${COMPILE} -o $@ -c $<

%.lo: %.c
	${COMPILE} -o $@ -c $<

.PHONY: default all compile check doc stage install clean distclean dist distcheck
