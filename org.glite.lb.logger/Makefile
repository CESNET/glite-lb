# defaults
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
nothrflavour=gcc32
thrflavour=gcc32pthr
expat_prefix=/opt/expat
ares_prefix=/opt/ares

-include Makefile.inc

VPATH:=${top_srcdir}/src

VERSION=-DVERSION=\"GLite-${version}\"

SUFFIXES=.no

GLOBUSINC=-I${globus_prefix}/include/${nothrflavour} \
	-I${globus_prefix}/include/${nothrflavour}/openssl

GLOBUSTHRINC=-I${globus_prefix}/include/${thrflavour} \
	-I${globus_prefix}/include/${thrflavour}/openssl

DEBUG:=-g -O0
CFLAGS:=${DEBUG} \
	-I${stagedir}/include -I${top_srcdir}/src \
	-D_GNU_SOURCE \
	${VERSION}

LDFLAGS:=-L${stagedir}/lib
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
INSTALL:=libtool --mode=install install

GLOBUS_LIBS:= -L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lssl_${nothrflavour}

GLOBUS_THRLIBS:= -L${globus_prefix}/lib \
	-lglobus_common_${thrflavour} \
	-lssl_${thrflavour}

EXT_LIBS:= -L${ares_prefix}/lib -lares \
	-L${expat_prefix}/lib -lexpat

HELPERS:=-lglite_wmsutils_tls_ssl_helpers
PHELPERS:=-lglite_wmsutils_tls_ssl_pthr_helpers
	
COMMON_LIB:=-lglite_lb_common

LOGD_OBJS:= logd_proto.o logd.o

INTERLOG_OBJS:=il_error.o input_queue_socket.o \
	recover.o send_event.o \
	event_queue.o event_store.o il_master.o interlogd.o \
	queue_mgr.o server_msg.o queue_thread.o

INTERLOG_NOBJS:=${INTERLOG_OBJS:.o=.no}

glite_lb_logd: ${LOGD_OBJS}
	${LINK} -o $@ ${LOGD_OBJS} ${COMMON_LIB}_${nothrflavour} ${HELPERS} ${EXT_LIBS} ${GLOBUS_LIBS}

glite_lb_interlogd: ${INTERLOG_OBJS}
	${LINK} -o $@ ${INTERLOG_OBJS} \
		${COMMON_LIB}_${thrflavour} ${HELPERS} ${PHELPERS} ${EXT_LIBS} ${GLOBUS_THRLIBS} -lpthread

glite_lb_notif_interlogd: ${INTERLOG_NOBJS}
	${LINK} -o $@ ${INTERLOG_NOBJS} \
		${COMMON_LIB}_${thrflavour} ${HELPERS} ${PHELPERS} ${EXT_LIBS} ${GLOBUS_THRLIBS} -lpthread

default: all

all compile: glite_lb_logd glite_lb_interlogd glite_lb_notif_interlogd

stage: compile
	$(MAKE) install PREFIX=${stagedir}

check:
	echo No unit tests so far

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
	${INSTALL} -m 755 glite_lb_logd glite_lb_interlogd glite_lb_notif_interlogd ${PREFIX}/bin

%.no: %.c
	${CC} ${CFLAGS} ${GLOBUSTHRINC} -DIL_NOTIFICATIONS -c $< -o $@

${INTERLOG_OBJS}: %.o: %.c
	${CC} ${CFLAGS} ${GLOBUSTHRINC} -c $< -o $@

${LOGD_OBJS}: %.o: %.c
	${CC} ${CFLAGS} ${GLOBUSINC} -c $< -o $@

