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
-include ../project/version.properties

version=${module.version}

CC=gcc
CXX=gcc

VPATH:=${top_srcdir}/src:${top_srcdir}/interface:${top_srcdir}/test:${top_srcdir}/doc

VERSION=-DVERSION=\"GLite-${version}\"

DEBUG:=-g -O0
CFLAGS:=${DEBUG} \
	-I${stagedir}/include -I${top_srcdir}/src \
	-I${activemq_prefix}/include/activemq-cpp-3.1.1 \
	-I${apr_prefix}/include \
	-I${aprutil_prefix}/include \
	-D_GNU_SOURCE \
	${COVERAGE_FLAGS} \
	${VERSION} ${LB_STANDALONE_FLAGS} ${LB_PERF_FLAGS} 

LDFLAGS:=-L${stagedir}/${libdir} \
	-L${activemq_prefix}/${libdir} \
	-L${aprutil_prefix}/${libdir} \
	-L${apr_prefix}/${libdir} \
	${COVERAGE_FLAGS} 

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
COMPILEXX:=libtool --mode=compile ${CXX} ${CFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
LINKXX:=libtool --mode=link ${CXX} -rpath ${stagedir}/${libdir} ${LDFLAGS} 
INSTALL:=libtool --mode=install install
SOLINK:=libtool --mode=link ${CXX} -module ${LDFLAGS} -rpath ${stagedir}/${libdir}

#ifneq (${expat_prefix},/usr)
#	EXPAT_LIBS:=-L${expat_prefix}/lib
#endif
#EXPAT_LIBS:=${EXPAT_LIBS} -lexpat
#
#EXT_LIBS:= ${EXPAT_LIBS}

HDRS:=

COMMON_LIB:=

EXT_LIB:=-lactivemq-cpp -lapr-1 -laprutil-1

GLITE_GSS_LIB:=

TEST_LIBS:=-L${cppunit_prefix}/${libdir} -lcppunit
TEST_INC:=-I${cppunit_prefix}/include

PLUGIN_OBJS:=activemq_cpp_plugin.o
PLUGIN_LOBJS:=${PLUGIN_OBJS:.o=.lo}
PLUGIN_LIB:=activemq_cpp_plugin.la

default: all 

all compile: ${PLUGIN_LIB}

${PLUGIN_LIB}: ${PLUGIN_LOBJS}
	${SOLINK} -o $@ ${PLUGIN_LOBJS} ${EXT_LIB}

${MAN_GZ}: ${MAN}
	rm -f ${MAN_GZ} ${MAN}
	cp $? .
	gzip -f $(notdir $?)

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
	-mkdir -p ${PREFIX}/${libdir}
	-mkdir -p ${PREFIX}/share/man/man8
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 755 ${PLUGIN_LIB} ${PREFIX}/${libdir}
	( cd ${top_srcdir}/project && ${INSTALL} -m 644 ChangeLog package.description package.summary ${PREFIX}/share/doc/${package}-${version} )
	${INSTALL} -m 644 ${MAN_GZ} ${PREFIX}/share/man/man8

%.lo %.o: %.cpp
	${COMPILEXX} -c $< -o $@


clean:
	rm -rvf .libs/ *.o *.lo ${PLUGIN} ${MAN_GZ}
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/
