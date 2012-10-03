# defaults
top_srcdir=..
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-logger
version=0.0.0
PREFIX=/opt/glite
prefix=
sysconfdir=/opt/glite/etc

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

CC=gcc

VPATH:=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/doc

VERSION=-DVERSION=\"GLite-${version}\"

SUFFIXES=.no

ifdef LB_STANDALONE
	LB_STANDALONE_FLAGS:=-DLB_STANDALONE
endif

ifdef LB_PERF
	LOGD:=glite-lb-logd-perf glite-lb-logd-perf-nofile
	INTERLOGD:=glite-lb-interlogd-perf \
		   glite-lb-interlogd-perf-empty 
#		   glite-lb-interlogd-perf-inline-empty
	NOTIF_INTERLOGD:=glite-lb-notif-interlogd
	LB_PERF_FLAGS:=-DLB_PERF
else
	LOGD:=glite-lb-logd
	INTERLOGD:=glite-lb-interlogd
	NOTIF_INTERLOGD:=glite-lb-notif-interlogd
	WS_INTERLOGD:=glite-lb-ws-interlogd
endif

ifeq (${thrflavour},)
default_thrflavour=
default_nothrflavour=
else
default_thrflavour=_${thrflavour}
default_nothrflavour=_${nothrflavour}
endif

os=${shell uname}
DL_LIBS=
ifeq (${os},Linux)
       DL_LIBS=-ldl
endif
GLITE_LBJP_COMMON_GSS_CFLAGS?=`PKG_CONFIG_PATH=$$PKG_CONFIG_PATH:${PREFIX}${prefix}/${libdir}/pkgconfig pkg-config glite-security-gss --cflags`

DEBUG:=-g -O0
CFLAGS:=${DEBUG} \
	-I${stagedir}${prefix}/include -I${top_srcdir}/src -I${top_srcdir}/interface \
	-D_GNU_SOURCE \
	${COVERAGE_FLAGS} \
	${VERSION} ${LB_STANDALONE_FLAGS} ${LB_PERF_FLAGS} \
	${GLITE_LBJP_COMMON_GSS_CFLAGS} ${CFLAGS}

LDFLAGS:=${LDFLAGS} -L${stagedir}${prefix}/${libdir} \
	${COVERAGE_FLAGS} 

COMPILE:=libtool --mode=compile ${CC} 
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS}
LINKXX:=libtool --mode=link ${CXX} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

#ifneq (${expat_prefix},/usr)
#	EXPAT_LIBS:=-L${expat_prefix}/lib
#endif
#EXPAT_LIBS:=${EXPAT_LIBS} -lexpat
#
#EXT_LIBS:= ${EXPAT_LIBS}

HDRS:=interlogd.h il_error.h

COMMON_LIB:=-lglite_lb_common
GLITE_GSS_LIB:=-lglite_security_gss

EXT_LIB:=-lglite_lbu_trio -lglite_lbu_log ${GLITE_GSS_LIB} -lglite_jobid

CPPUNIT_LIBS?=-L${cppunit_prefix}/${libdir} -lcppunit
CPPUNIT_CFLAGS?=-I${cppunit_prefix}/include

LOGD_OBJS:= logd_proto.o logd.o

LOGD_NOBJS:=${LOGD_OBJS:.o=.no}

INTERLOG_OBJS:=il_error.o input_queue_socket.o \
	recover.o send_event.o plugin_mgr.o \
	event_queue.o event_store.o il_master.o interlogd.o \
	queue_mgr.o server_msg.o queue_thread.o \

WS_INTERLOG_OBJS:=il_error.o input_queue_socket_http.o \
	recover.o http.o send_event_http.o plugin_mgr.o \
	event_queue.o event_store_http.o il_master.o interlogd.o \
	queue_mgr_http.o server_msg_http.o queue_thread.o

INTERLOG_NOTIF_OBJS:=${INTERLOG_OBJS:.o=.notif.lo}
INTERLOG_WS_OBJS:=${WS_INTERLOG_OBJS:.o=.ws.o}
INTERLOG_PERF_OBJS:=${INTERLOG_OBJS:.o=.perf.o}
INTERLOG_EMPTY_OBJS:=${INTERLOG_OBJS:.o=.empty.o}
#INTERLOG_INLINE_EMPTY_OBJS:=${INTERLOG_OBJS:.o=.io}

INTERLOG_TEST_OBJS:= \
	il_error.o \
	server_msg.o \
	server_msgTest.o \
	queue_thread.o \
	event_store.o \
	event_storeTest.o \
	queue_mgr.o \
	il_master.o \
	input_queue_socket.o \
	input_queue_socketTest.o \
	send_event.o \
	plugin_mgr.o \
	event_queue.o \
	event_queueTest.o \
	IlTestBase.o \
	il_test.o 

MAN_GZ:=glite-lb-interlogd.8.gz glite-lb-logd.8.gz
MAN = $(MAN_GZ:.gz=)

default: all 

all compile: startup $(LOGD) $(INTERLOGD) $(NOTIF_INTERLOGD) ${MAN_GZ}

startup: ${top_srcdir}/config/startup
	glite_var="${localstatedir}/glite"; \
	if echo "${localstatedir}" | grep 'glite'>/dev/null; then \
		glite_var="${localstatedir}"; \
	fi; \
	sed -e 's:@glite_prefix@:${sysroot}${prefix}:' -e 's:@glite_etc@:${sysconfdir}:' -e "s:@glite_var@:$$glite_var:" $< > $@
	chmod +x $@

glite-lb-logd: ${LOGD_OBJS}
	${LINK} -o $@ ${LOGD_OBJS} ${COMMON_LIB}${default_nothrflavour} ${EXT_LIB}

glite-lb-logd-perf: ${LOGD_OBJS}
	${LINK} -o $@ ${LOGD_OBJS} ${COMMON_LIB}${default_nothrflavour} ${EXT_LIB}

glite-lb-logd-nofile: ${LOGD_NOBJS}
	${LINK} -o $@ ${LOGD_NOBJS} ${COMMON_LIB}${default_nothrflavour} ${EXT_LIB}

glite-lb-logd-perf-nofile: ${LOGD_NOBJS}
	${LINK} -o $@ ${LOGD_NOBJS} ${COMMON_LIB}${default_nothrflavour} ${EXT_LIB}

glite-lb-interlogd: ${INTERLOG_OBJS}
	${LINK} -o $@ ${INTERLOG_OBJS} ${COMMON_LIB}${default_thrflavour} ${EXT_LIB} -lpthread ${DL_LIBS}

glite-lb-notif-interlogd: ${INTERLOG_NOTIF_OBJS}
	${LINK} -export-dynamic -o $@ ${INTERLOG_NOTIF_OBJS} ${COMMON_LIB}${default_thrflavour} ${EXT_LIB} -lpthread ${DL_LIBS}

glite-lb-interlogd-perf: ${INTERLOG_PERF_OBJS}
	${LINK} -o $@ ${INTERLOG_PERF_OBJS} ${COMMON_LIB}${default_thrflavour} ${EXT_LIB} -lpthread

glite-lb-interlogd-perf-empty: ${INTERLOG_EMPTY_OBJS}
	${LINK} -o $@ ${INTERLOG_EMPTY_OBJS} ${COMMON_LIB}${default_thrflavour} ${EXT_LIB} -lpthread

glite-lb-ws-interlogd: ${INTERLOG_WS_OBJS}
	${LINK} -o $@ ${INTERLOG_WS_OBJS} ${COMMON_LIB}${default_thrflavour} ${EXT_LIB} -lpthread

#glite-lb-interlogd-perf-inline-empty: ${INTERLOG_INLINE_EMPTY_OBJS}
#	${LINK} -o $@ ${INTERLOG_INLINE_EMPTY_OBJS} \
#		${COMMON_LIB}${default_thrflavour}

${MAN_GZ}: ${MAN}
	rm -f ${MAN_GZ} ${MAN}
	cp $? .
	gzip -9 -f $(notdir $?)

man: ${MAN_GZ}

stage: compile
	$(MAKE) install PREFIX=${stagedir} 

check: 
# do nothing until test/ is really added to CVS
# check.ll check.il

#check.ll: logd_proto_test.o ll_test.o
#	${LINKXX} -o $@ ${COMMON_LIB}${default_nothrflavour} ${EXT_LIBS} ${CPPUNIT_LIBS} $+
#	./check.ll

check.ll:
	-echo commented out -- fix needed

check.il: ${INTERLOG_TEST_OBJS}
	${LINKXX} -o $@ ${COMMON_LIB}${default_thrflavour} ${CPPUNIT_LIBS} ${GLITE_GSS_LIB} -lglite_jobid -lpthread ${DL_LIBS} $+

install:
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/bin
	-mkdir -p ${DESTDIR}${PREFIX}${sysconfdir}/init.d
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/man/man8
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/glite/lb
	${INSTALL} -m 755 ${LOGD} ${DESTDIR}${PREFIX}${prefix}/bin
	${INSTALL} -m 755 ${INTERLOGD} ${DESTDIR}${PREFIX}${prefix}/bin
	${INSTALL} -m 755 ${NOTIF_INTERLOGD} ${DESTDIR}${PREFIX}${prefix}/bin
ifdef LB_PERF
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/sbin
	${INSTALL} -m 755 ${top_srcdir}/src/perftest_ll.sh ${DESTDIR}${PREFIX}${prefix}/sbin
	${INSTALL} -m 755 ${top_srcdir}/src/perftest_il.sh ${DESTDIR}${PREFIX}${prefix}/sbin
endif
	${INSTALL} -m 755 startup ${DESTDIR}${PREFIX}${sysconfdir}/init.d/glite-lb-locallogger
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	( cd ${top_srcdir}/interface && ${INSTALL} -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix} )
	( cd ${top_srcdir}/project && ${INSTALL} -m 644 ChangeLog package.description package.summary ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version} )
	${INSTALL} -m 644 ${MAN_GZ} ${DESTDIR}${PREFIX}${prefix}/share/man/man8

${INTERLOG_NOTIF_OBJS}: %.notif.lo: %.c
	${COMPILE} ${CFLAGS} -DIL_NOTIFICATIONS -c $< -o $@

${INTERLOG_OBJS}: %.o: %.c
	${COMPILE} ${CFLAGS} -c $< -o $@

${INTERLOG_WS_OBJS}: %.ws.o: %.c
	${CC} ${CFLAGS} -DIL_WS -c $< -o $@

${INTERLOG_EMPTY_OBJS}: %.empty.o: %.c
	${CC} ${CFLAGS} -DPERF_EMPTY -c $< -o $@

${INTERLOG_PERF_OBJS}: %.perf.o: %.c
	${CC} ${CFLAGS} -c $< -o $@

#${INTERLOG_INLINE_EMPTY_OBJS}: %.io: %.c
#	${CC} ${CFLAGS} -DLB_PERF -DPERF_EMPTY -DPERF_EVENTS_INLINE -c $< -o $@

${LOGD_NOBJS}: %.no: %.c
	${CC} ${CFLAGS} -DLOGD_NOFILE -c $< -o $@

${LOGD_OBJS}: %.o: %.c
	${CC} ${CFLAGS} -c $< -o $@

logd_proto_test.o: %.o: %.c
	${CC} ${CFLAGS} -c $< -o $@

ll_test.o: %.o: %.cpp
	${CXX} ${CFLAGS} ${CPPUNIT_CFLAGS} -c $< -o $@

il_test.o IlTestBase.o server_msgTest.o event_queueTest.o input_queue_socketTest.o event_storeTest.o: %.o: %.cpp
	${CXX} ${CFLAGS} ${CPPUNIT_CFLAGS} -c $< -o $@

clean:
	rm -rvf .libs/ *.o *.lo *.no ${LOGD} ${INTERLOGD} ${NOTIF_INTERLOGD} ${MAN_GZ} startup
	rm -rvf check.ll check.il

distclean:
	rm -rvf Makefile.inc *.spec debian/

.PHONY: default all compile man stage check install clean distclean dist distcheck
