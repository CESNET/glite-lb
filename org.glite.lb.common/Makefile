# Default values
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-common
version=0.0.0
PREFIX=/opt/glite

glite_location=/opt/glite
globus_prefix=/opt/globus
nothrflavour=gcc32
thrflavour=gcc32pthr
expat_prefix=/opt/expat
ares_prefix=/opt/ares

-include Makefile.inc

VPATH=${top_srcdir}/src:${top_srcdir}/test
AT3=perl -I${top_srcdir}/project ${top_srcdir}/project/at3


SUFFIXES = .T

DEBUG:=-g -O0 -Wall
GLOBUSINC:= -I${globus_prefix}/include/${nothrflavour}
GLOBUSTHRINC:= -I${globus_prefix}/include/${thrflavour}

CFLAGS:=${DEBUG} -I${top_srcdir}/src -I${top_srcdir}/interface \
	-I${stagedir}/include \
	-I${glite_location}/include \
	-I${expat_prefix}/include \
	-I${ares_prefix}/include \
	${COVERAGE_FLAGS} \
	-DDATAGRID_EXTENSION

GLOBUS_LIBS:=-L${globus_prefix}/lib \
        -lglobus_common_${nothrflavour} \
        -lglobus_gssapi_gsi_${nothrflavour} \

GLITE_LIBS:=-L${glite_location}/lib
EXPAT_LIBS:=-L${expat_prefix}/lib -lexpat
ARES_LIBS:=-L${ares_prefix}/lib -lares
LDFLAGS:=-L${stagedir}/lib \
    ${GLITE_LIBS} \
    ${GLOBUS_LIBS} \
    ${EXPAT_LIBS} \
    ${ARES_LIBS} \
    ${COVERAGE_FLAGS}

TEST_LIBS:=-L${cppunit}/lib -lcppunit
TEST_INC:=-I${cppunit}/include

    
COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/lib ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} -rpath ${stagedir}/lib ${LDFLAGS} 
INSTALL:=libtool --mode=install install

OBJS:=lb_gss.o escape.o events.o mini_http.o query_rec.o status.o \
	xml_conversions.o xml_parse.o ulm_parse.o param.o \
	events_parse.o il_string.o il_int.o notifid.o \
	il_log.o il_msg.o context.o trio.o strio.o
LOBJS:=${OBJS:.o=.lo}

THROBJS:=${OBJS:.o=.thr.o}
THRLOBJS:=${OBJS:.o=.thr.lo}

HDRS:=context-int.h  lb_gss.h  mini_http.h authz.h xml_parse.h \
	xml_conversions.h log_proto.h events_parse.h il_string.h escape.h \
	ulm_parse.h trio.h

STATICLIB:=libglite_lb_common_${nothrflavour}.a
THRSTATICLIB:=libglite_lb_common_${thrflavour}.a
LTLIB:=libglite_lb_common_${nothrflavour}.la
THRLTLIB:=libglite_lb_common_${thrflavour}.la

default: all

all compile: ${STATICLIB} ${LTLIB} ${THRSTATICLIB} ${THRLTLIB}

${STATICLIB}: ${OBJS}
	ar crv $@ ${OBJS}
	ranlib $@

${THRSTATICLIB}: ${THROBJS}
	ar crv $@ ${THROBJS}
	ranlib $@

${LTLIB}: ${OBJS}
	${LINK} -o $@ ${LOBJS} \
		-lglite_wmsutils_cjobid \
		-lm

${THRLTLIB}: ${THROBJS}
	${LINK} -o $@ ${THRLOBJS} \
		-lglite_wmsutils_cjobid \
		-lm

stage: compile
	$(MAKE) install PREFIX=${stagedir}

check: check.parse

check.parse: parse.cpp
	${CXX} -c ${CFLAGS} ${TEST_INC} $<
	${LINKXX} -o $@ parse.o ${LTLIB} ${TEST_LIBS}

dist: distsrc distbin

distsrc:
	mkdir -p ${top_srcdir}/${package}-${version}
	cd ${top_srcdir} && GLOBIGNORE="${package}-${version}" && cp -Rf * ${package}-${version}
	cd ${top_srcdir} && tar -czf ${distdir}/${package}-${version}_src.tar.gz --exclude-from=project/tar_exclude ${package}-${version}
	rm -rf ${top_srcdir}/${package}-${version}

distbin:
	$(MAKE) install PREFIX=`pwd`/tmpbuilddir${stagedir}
	save_dir=`pwd`; cd tmpbuilddir${stagedir} && tar -czf $$save_dir/${top_srcdir}/${distdir}/${package}-${version}_bin.tar.gz *; cd $$save_dir
	rm -rf tmpbuilddir
	
install:
	mkdir -p ${PREFIX}/include/${globalprefix}/${lbprefix}
	mkdir -p ${PREFIX}/lib
	${INSTALL} -m 644 ${STATICLIB} ${THRSTATICLIB} ${PREFIX}/lib
	${INSTALL} -m 644 ${LTLIB} ${THRLTLIB} ${PREFIX}/lib
	cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${lbprefix}

clean:


%.o: %.c
	${COMPILE} ${GLOBUSINC} -c $<

%.thr.o: %.c
	${COMPILE} ${GLOBUSTHRINC} -o $@ -c $<

%.h: %.h.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

%.cpp: %.cpp.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null
