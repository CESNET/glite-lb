# Default values
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-client
version=0.0.0
PREFIX=/opt/glite

glite_location=/opt/glite
globus_prefix=/opt/globus
globusflavour=gcc32dbg
expat_prefix=/opt/expat
ares_prefix=/opt/ares

-include Makefile.inc

VPATH=${top_srcdir}/src
AT3=perl -I${top_srcdir}/project ${top_srcdir}/project/at3

SUFFIXES = .T 

DEBUG:=-g -O0 
CFLAGS:=${DEBUG} \
	-I${top_srcdir}/src -I${top_srcdir}/interface \
	-I${stagedir}/include \
	-I${glite_location}/include \
	-I${globus_prefix}/include/${globusflavour} \
    -I${globus_prefix}/include/${globusflavour}/openssl \
	-I${expat_prefix}/include \
	-I${ares_prefix}/include

CXXFLAGS:=${CFLAGS}

HELPERS:=-L${glite_location}/lib -lglite_wms_tls_ssl_helpers

GLOBUS_LIBS:=-L${globus_prefix}/lib \
	-lglobus_common_${globusflavour} \
	-lssl_${globusflavour}

EXPAT_LIBS:=-L${expat_prefix}/lib \
	-lexpat

ARES_LIBS:=-L${ares_prefix}/lib \
	-lares

LDFLAGS:=-L${stagedir}/lib

EXT_LIB:=${GLOBUS_LIBS} \
    ${EXPAT_LIBS} \
    ${ARES_LIBS}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

LIBOBJS:=connection.o consumer.o notification.o prod_proto.o \
	producer.o uiwrap.o

PLUSOBJS:=Event.o Job.o JobStatus.o Notification.o ServerConnection.o
PUB_HDRS:=CountRef.h Event.h JobJobStatus.h Notification.h ServerConnection.h \
	LoggingExceptions.h

LIBLOBJS:=${LIBOBJS:.o=.lo}
PLUSLOBJS:=${PLUSOBJS:.o=.lo}

LIB:=libglite_lb_client.la
PLUSLIB:=libglite_lb_clientpp.la

TOOLS:=dump load purge

${LIB}: ${LIBOBJS}
	${LINK} -o $@ ${LIBLOBJS} -rpath ${glite_location}/lib -lglite_lb_common ${HELPERS}

${PLUSLIB}: ${PLUSOBJS}
	${LINK} -o $@ ${PLUSLOBJS} -rpath ${glite_location}/lib ${LIB}
logevent: logevent.o args.o
	${LINK} -o $@ logevent.o args.o ${LIB} ${EXT_LIB}

${TOOLS}: %: %.o
	${LINK} -o $@ $< ${LIB} ${EXT_LIB}

${TOOLS}: ${LIB}

${LIBOBJS}: %.o: %.c
	${COMPILE} -c $<

%.o: %.c 
	${CC} ${CFLAGS} -c $<

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

%.cpp: %.cpp.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

default: all

compile all: ${LIB} ${TOOLS} logevent

check:
	echo No unit tests so far.

stage: compile
	$(MAKE) install PREFIX=${stagedir}

dist: distsrc distbin

distsrc:
	mkdir -p ${top_srcdir}/${package}-${version}
	cd ${top_srcdir} && GLOBIGNORE="${package}-${version}" && cp -Rf * ${package}-${version}
	cd ${top_srcdir} && tar -czf ${distdir}/${package}-${version}_src.tar.gz --exclude-from=project/tar_exclude ${package}-${version}
	rm -rf ${top_srcdir}/${package}-${version}

distbin:
	$(MAKE) install PREFIX=`pwd`/tmpbuilddir
	cd tmpbuilddir && tar -czf ../${top_srcdir}/${distdir}/${package}-${version}_bin.tar.gz *
	rm -rf tmpbuilddir
	
install:
	mkdir -p ${PREFIX}/bin
	mkdir -p ${PREFIX}/lib
	${INSTALL} -m 644 ${LIB} ${PREFIX}/lib
	for p in ${TOOLS} logevent; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/bin/glite-lb-$$p"; \
	done

clean:

