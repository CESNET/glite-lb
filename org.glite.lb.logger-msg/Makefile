# defaults
top_srcdir=..
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-logger-msg
version=0.0.0
PREFIX=/opt/glite

nothrflavour=gcc32
thrflavour=gcc32pthr

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

CC=gcc
CXX=gcc

VPATH:=${top_srcdir}/src:${top_srcdir}/interface:${top_srcdir}/test:${top_srcdir}/doc:${top_srcdir}/examples

VERSION=-DVERSION=\"GLite-${version}\"

ACTIVEMQ_CFLAGS?=`PKG_CONFIG_PATH=${activemq_prefix}/${libdir}/pkgconfig pkg-config --cflags activemq-cpp`
ACTIVEMQ_LIBS?=-L${activemq_prefix}/${libdir} -lactivemq-cpp

CPPUNIT_LIBS?=-L${cppunit_prefix}/${libdir} -lcppunit
CPPUNIT_CFLAGS?=-I${cppunit_prefix}/include

DEBUG:=-g -O0
CFLAGS:=${CFLAGS} ${DEBUG} \
	-I${stagedir}${prefix}/include -I${top_srcdir}/src \
	${ACTIVEMQ_CFLAGS} \
	-D_GNU_SOURCE \
	${COVERAGE_FLAGS} \
	${VERSION} ${LB_STANDALONE_FLAGS} ${LB_PERF_FLAGS} 

LDFLAGS:=${LDFLAGS} -L${stagedir}${prefix}/${libdir} \
	${COVERAGE_FLAGS} 

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
COMPILEXX:=libtool --mode=compile ${CXX} ${CFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
LINKXX:=libtool --mode=link ${CXX} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS} 
INSTALL:=libtool --mode=install install
SOLINK:=libtool --mode=link ${CXX} -module ${LDFLAGS} -rpath ${stagedir}${prefix}/${libdir}

#ifneq (${expat_prefix},/usr)
#	EXPAT_LIBS:=-L${expat_prefix}/lib
#endif
#EXPAT_LIBS:=${EXPAT_LIBS} -lexpat
#
#EXT_LIBS:= ${EXPAT_LIBS}

HDRS:=

COMMON_LIB:=

EXT_LIB:=${ACTIVEMQ_LIBS}

GLITE_GSS_LIB:=

PLUGIN_OBJS:=activemq_cpp_plugin.o
PLUGIN_LOBJS:=${PLUGIN_OBJS:.o=.lo}
PLUGIN_LIB:=activemq_cpp_plugin.la

EXAMPLES=glite-lb-cmsclient

default: all 

all compile: ${PLUGIN_LIB} ${EXAMPLES}

${PLUGIN_LIB}: ${PLUGIN_LOBJS}
	${SOLINK} -o $@ ${PLUGIN_LOBJS} ${EXT_LIB}

${MAN_GZ}: ${MAN}
	rm -f ${MAN_GZ} ${MAN}
	cp $? .
	gzip -f $(notdir $?)

glite-lb-cmsclient: cmsclient.o
	$(CC) $< ${EXT_LIB} -o $@

man: ${MAN_GZ}

stage: compile
	$(MAKE) install PREFIX=${stagedir} 

check: 
# do nothing until test/ is really added to CVS
# check.ll check.il

#check.ll: logd_proto_test.o ll_test.o
#	${LINKXX} -o $@ ${COMMON_LIB}_${nothrflavour} ${EXT_LIBS} ${TEST_LIBS} $+
#	./check.ll

install:
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/man/man8
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/etc/glite-lb
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	${INSTALL} -m 755 ${PLUGIN_LIB} ${DESTDIR}${PREFIX}${prefix}/${libdir}
	${INSTALL} -m 755 ${EXAMPLES} ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples
	( cd ${top_srcdir}/project && ${INSTALL} -m 644 ChangeLog package.description package.summary ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version} )
	${INSTALL} -m 644 ${top_srcdir}/config/msg.conf.example ${DESTDIR}${PREFIX}${prefix}/etc/glite-lb
#	${INSTALL} -m 644 ${MAN_GZ} ${DESTDIR}${PREFIX}${prefix}/share/man/man8

%.lo %.o: %.cpp
	${COMPILEXX} -c $< -o $@


clean:
	rm -rvf .libs/ *.o *.lo ${PLUGIN_LIB} ${MAN_GZ} ${EXAMPLES}
	rm -rvf log.xml rpmbuild/ RPMS/ tgz/
