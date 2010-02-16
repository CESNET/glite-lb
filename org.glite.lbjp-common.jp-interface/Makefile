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

globus_prefix=/opt/globus
nothrflavour=gcc32
thrflavour=gcc32pthr
expat_prefix=/opt/expat
gsoap_prefix=/software/gsoap-2.6

CC=gcc

-include Makefile.inc


VPATH=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/project:${jpproject}

DEBUG:=-g -O0 -W -Wno-sign-compare
CFLAGS:=${DEBUG} -D_GNU_SOURCE -I. -I${top_srcdir}/interface -I${stagedir}/include

offset=0
version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }


LINK:=libtool --mode=link ${CC} ${LDFLAGS} -rpath ${stagedir}/${libdir} ${version_info}
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install
COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}

HDRS:=types.h context.h attr.h known_attr.h backend.h builtin_plugins.h file_plugin.h indexdb.h type_plugin.h

SRCS:=context.c attr.c utils.c indexdb.c
OBJS:=${SRCS:.c=.lo}
THROBJS:=${OBJS:.o=.thr.lo}
LIBS:=-L${stagedir}/${libdir} -lglite_jobid -lglite_lbu_db
THRLIBS:=${LIBS}

commonlib:= libglite_jp_common_${nothrflavour}.la
commonlib_thr:= libglite_jp_common_${thrflavour}.la

TEST_LIBS:=-L${cppunit_prefix}/${libdir} -lcppunit -ldl
TEST_INC:=-I${cppunit_prefix}/include


default all: compile

compile: ${commonlib} ${commonlib_thr}

${commonlib}: ${OBJS}
	${LINK} -o $@ ${OBJS} ${LIBS}

${commonlib_thr}: ${THROBJS}
	${LINK} -o $@ ${THROBJS} ${THRLIBS}

check: type_test
	./type_test type_test.xml

type_test: %: %.cpp compile
	${CXX} -c ${CFLAGS} ${TEST_INC} $<
	${LINKXX} -o $@ $@.o ${commonlib} ${TEST_LIBS} 

doc:

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install:
	-mkdir -p ${PREFIX}/include/${globalprefix}/${jpprefix}
	cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${jpprefix}
	-mkdir -p ${PREFIX}/${libdir}
	${INSTALL} -m 755 ${commonlib} ${commonlib_thr} ${PREFIX}/${libdir}

clean:
	rm -rvf *.o *.lo .libs lib*
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/
	rm -f glite jp

%.thr.lo: %.c
	${COMPILE} -o $@ -c $<

%.lo: %.c
	${COMPILE} -o $@ -c $< 
