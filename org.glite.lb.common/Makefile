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

VPATH=${top_srcdir}/src
AT3=perl -I${top_srcdir}/project ${top_srcdir}/project/at3

SUFFIXES = .T

DEBUG:=-g -O0 -Wall
GLOBUSINC:= -I${globus_prefix}/include/${nothrflavour} \
	-I${globus_prefix}/include/${nothrflavour}/openssl
GLOBUSTHRINC:= -I${globus_prefix}/include/${thrflavour} \
	-I${globus_prefix}/include/${thrflavour}/openssl

CFLAGS:=${DEBUG} -I${top_srcdir}/src -I${top_srcdir}/interface \
	-I${stagedir}/include \
	-I${glite_location}/include \
	-I${expat_prefix}/include \
	-I${ares_prefix}/include \
	-DDATAGRID_EXTENSION

GLITE_LIBS:=-L${glite_location}/lib
GLOBUS_LIBS:=-L${globus_prefix}/lib
EXPAT_LIBS:=-L${expat_prefix}/lib
ARES_LIBS:=-L${ares_prefix}/lib
LDFLAGS:=-L${stagedir}/lib \
    ${GLITE_LIBS} \
    ${GLOBUS_LIBS} \
    ${EXPAT_LIBS} \
    ${ARES_LIBS}
    
COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/lib ${LDFLAGS} 
INSTALL:=libtool --mode=install install

OBJS:=dgssl.o escape.o events.o mini_http.o query_rec.o status.o \
	xml_conversions.o xml_parse.o ulm_parse.o param.o \
	events_parse.o il_string.o il_int.o notifid.o \
	il_log.o il_msg.o context.o trio.o strio.o
LOBJS:=${OBJS:.o=.lo}

THROBJS:=${OBJS:.o=.thr.o}
THRLOBJS:=${OBJS:.o=.thr.lo}

HDRS:=context-int.h  dgssl.h  mini_http.h authz.h xml_parse.h \
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
		-lglobus_ssl_utils -lglite_wmsutils_cjobid \
		-lm

${THRLTLIB}: ${THROBJS}
	${LINK} -o $@ ${THRLOBJS} \
		-lglobus_ssl_utils -lglite_wmsutils_cjobid \
		-lm

stage: compile
	$(MAKE) install PREFIX=${stagedir}

check:
	echo Unit tests missing

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
