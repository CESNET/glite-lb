# defaults
top_srcdir=..
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-logger-msg
version=0.0.0
PREFIX=/opt/glite
sysconfdir=/opt/glite/etc

nothrflavour=gcc32
thrflavour=gcc32pthr

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}
docdir?=${prefix}/share/doc/${package}-${version}

CC=gcc
CXX=g++

VPATH:=${top_srcdir}/src:${top_srcdir}/interface:${top_srcdir}/test:${top_srcdir}/doc:${top_srcdir}/examples:${top_srcdir}/config

SOURCES=\
	config/msg.* \
	examples/*.cpp \
	project/il.conf \
	src/*.cpp src/msg-* \
	Makefile
SOURCES_EXEC=src/msg-brokers

VERSION=-DVERSION=\"GLite-${version}\"

ACTIVEMQ_CPP_CFLAGS?=`${activemq_prefix}/bin/activemqcpp-config --cflags`
ACTIVEMQ_CPP_LIBS?=`${activemq_prefix}/bin/activemqcpp-config --libs`

CPPUNIT_LIBS?=-L${cppunit_prefix}/${libdir} -lcppunit
CPPUNIT_CFLAGS?=-I${cppunit_prefix}/include

GLITE_LBJP_COMMON_GSS_CFLAGS?=`PKG_CONFIG_PATH=$$PKG_CONFIG_PATH:${PREFIX}${prefix}/${libdir}/pkgconfig pkg-config glite-security-gss --cflags`

DEBUG:=-g -O0
CFLAGS:=${DEBUG} \
	-I${stagedir}${prefix}/include -I${top_srcdir}/src \
	${ACTIVEMQ_CPP_CFLAGS} \
	-D_GNU_SOURCE \
	${COVERAGE_FLAGS} \
	${VERSION} ${LB_STANDALONE_FLAGS} ${LB_PERF_FLAGS} \
	${GLITE_LBJP_COMMON_GSS_CFLAGS} ${CFLAGS}

LDFLAGS:=${LDFLAGS} -L${stagedir}${prefix}/${libdir} \
	${COVERAGE_FLAGS} 

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
COMPILEXX:=libtool --mode=compile ${CXX} ${CFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
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

EXT_LIB:=${ACTIVEMQ_CPP_LIBS}

GLITE_GSS_LIB:=

PLUGIN_OBJS:=activemq_cpp_plugin.o
PLUGIN_LOBJS:=${PLUGIN_OBJS:.o=.lo}
PLUGIN_LIB:=activemq_cpp_plugin.la

EXAMPLES=glite-lb-cmsclient

SCRIPTS_SRC=msg.cron.in msg-config.in
SCRIPTS=${SCRIPTS_SRC:.in=.new}

ifeq (${thrflavour},)
default_flavour=
else
default_flavour=_${nothrflavour}
endif

sysdefaultdir?=${sysconfdir}/default

all compile: ${PLUGIN_LIB} ${EXAMPLES} ${SCRIPTS}

${PLUGIN_LIB}: ${PLUGIN_LOBJS}
	${SOLINK} -o $@ ${PLUGIN_LOBJS} ${EXT_LIB} -lglite_lb_common${default_flavour} -lglite_jobid -lglite_lbu_log

%.1.gz: %.1
	gzip -9 -f -c $< > $@

glite-lb-cmsclient: cmsclient.o
	$(CXX) $< ${EXT_LIB} -o $@

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
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples
	-mkdir -p ${DESTDIR}${PREFIX}${docdir}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/sbin
	-mkdir -p ${DESTDIR}${PREFIX}${sysconfdir}/glite-lb
	-mkdir -p ${DESTDIR}${PREFIX}${sysconfdir}/cron.d
	${INSTALL} -m 644 ${top_srcdir}/config/msg.conf.example ${DESTDIR}${PREFIX}${docdir}
	${INSTALL} -m 755 ${PLUGIN_LIB} ${DESTDIR}${PREFIX}${prefix}/${libdir}
	${INSTALL} -m 755 ${EXAMPLES} ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples
	${INSTALL} -m 755 ${top_srcdir}/src/msg-brokers ${DESTDIR}${PREFIX}${prefix}/sbin/glite-lb-msg-brokers
	${INSTALL} -m 755 msg-config.new ${DESTDIR}${PREFIX}${prefix}/sbin/glite-lb-msg-config
	${INSTALL} -m 644 msg.cron.new ${DESTDIR}${PREFIX}${sysconfdir}/cron.d/glite-lb-logger-msg

%.lo %.o: %.cpp
	${COMPILEXX} -c $< -o $@

%.new: %.in
	sed -e 's:@glite_prefix@:${sysroot}${prefix}:g' -e 's:@glite_etc@:${sysconfdir}:g' -e 's:@defaultdir@:${sysdefaultdir}:g' $< > $@

clean:
	rm -rvf .libs/ *.o *.lo ${PLUGIN_LIB} ${MAN_GZ} ${EXAMPLES} ${SCRIPTS}
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

.PHONY: default all compile man stage check install clean dist distclean distcheck
