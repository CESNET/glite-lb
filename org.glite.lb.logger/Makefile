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

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

CC=gcc

VPATH:=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/doc

SOURCES=\
	config/startup config/startup.* config/systemd.* \
	doc/*.8 \
	interface/*.h \
	project/.post* project/.pre* \
	src/*.c src/*.cpp src/*.h src/*.sh \
	src-nt/*.cpp src-nt/*.H src-nt/Makefile src-nt/test/*.cpp \
	test/*.c test/*.cpp test/*.h \
	Makefile
SOURCES_EXEC=config/startup config/startup.* src/*.sh

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
	LB_PERF_FLAGS:=-DLB_PERF
else
	LOGD:=glite-lb-logd
	INTERLOGD:=glite-lb-interlogd
endif
NOTIF_INTERLOGD:=glite-lb-notif-interlogd
PROXY_INTERLOGD:=glite-lb-proxy-interlogd

ifeq (${thrflavour},)
default_thrflavour=
default_nothrflavour=
else
default_thrflavour=_${thrflavour}
default_nothrflavour=_${nothrflavour}
endif

GLITE_LBJP_COMMON_GSS_CFLAGS?=`PKG_CONFIG_PATH=$$PKG_CONFIG_PATH:${PREFIX}${prefix}/${libdir}/pkgconfig pkg-config glite-security-gss --cflags`

os=${shell uname}
DL_LIBS=
ifeq (${os},Linux)
       DL_LIBS=-ldl
endif

sysdefaultdir?=${sysconfdir}/default

ifeq ($(os_type),systemd)
unitdir=`pkg-config systemd --variable=systemdsystemunitdir`
endif

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
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
INSTALL:=libtool --mode=install install

#ifneq (${expat_prefix},/usr)
#	EXPAT_LIBS:=-L${expat_prefix}/lib
#endif
#EXPAT_LIBS:=${EXPAT_LIBS} -lexpat
#
#EXT_LIBS:= ${EXPAT_LIBS}

HDRS:=interlogd.h il_error.h

COMMON_LIB:=-lglite_lb_common
GLITE_GSS_LIB:=-lglite_security_gss${default_thrflavour}

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

SCRIPTS:=startup
SCRIPTS_SRC=$(addprefix ${top_srcdir}/config/,${SCRIPTS})

all compile: ${SCRIPTS} $(LOGD) $(INTERLOGD) $(NOTIF_INTERLOGD) ${MAN_GZ}

${SCRIPTS}: ${SCRIPTS_SRC}
	glite_var="${localstatedir}/glite"; \
	if echo "${localstatedir}" | grep 'glite'>/dev/null; then \
		glite_var="${localstatedir}"; \
	fi; \
	sed -e 's:@glite_prefix@:${sysroot}${prefix}:g' -e 's:@glite_etc@:${sysconfdir}:g' -e "s:@glite_var@:$$glite_var:g" -e 's:@defaultdir@:${sysdefaultdir}:g' ${top_srcdir}/config/$@ > $@
ifneq ($(os_type),debian)
	sed -i 's/# \(Default-Start\|Default-Stop\):.*/# \1:/' $@
endif
	sed "s/^@$(os_type)@//" $@ > $@.tmp
	grep -v '^@' $@.tmp > $@
	rm $@.tmp
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
	${LINK} -o $@ ${INTERLOG_PERF_OBJS} ${COMMON_LIB}${default_thrflavour} ${EXT_LIB} -lpthread ${DL_LIBS}

glite-lb-interlogd-perf-empty: ${INTERLOG_EMPTY_OBJS}
	${LINK} -o $@ ${INTERLOG_EMPTY_OBJS} ${COMMON_LIB}${default_thrflavour} ${EXT_LIB} -lpthread ${DL_LIBS}

glite-lb-ws-interlogd: ${INTERLOG_WS_OBJS}
	${LINK} -o $@ ${INTERLOG_WS_OBJS} ${COMMON_LIB}${default_thrflavour} ${EXT_LIB} -lpthread

#glite-lb-interlogd-perf-inline-empty: ${INTERLOG_INLINE_EMPTY_OBJS}
#	${LINK} -o $@ ${INTERLOG_INLINE_EMPTY_OBJS} \
#		${COMMON_LIB}${default_thrflavour}

%.8.gz: %.8
	gzip -9 -f -c $< > $@

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
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/sbin
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/man/man8
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/glite/lb
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/glite-lb-logger
	-mkdir -p -m 0775 ${DESTDIR}${PREFIX}/var/spool/glite/lb-locallogger
	-mkdir -p -m 0755 ${DESTDIR}${PREFIX}/var/spool/glite/lb-notif
	-mkdir -p -m 0755 ${DESTDIR}${PREFIX}/var/spool/glite/lb-proxy
	${INSTALL} -m 755 ${LOGD} ${DESTDIR}${PREFIX}${prefix}/sbin
	${INSTALL} -m 755 ${INTERLOGD} ${DESTDIR}${PREFIX}${prefix}/sbin
	${INSTALL} -m 755 ${NOTIF_INTERLOGD} ${DESTDIR}${PREFIX}${prefix}/sbin
	${INSTALL} -m 755 glite-lb-interlogd ${DESTDIR}${PREFIX}${prefix}/sbin/glite-lb-proxy-interlogd
	${INSTALL} -m 755 ${top_srcdir}/src/lb_krb_ticket.sh ${DESTDIR}${PREFIX}${prefix}/share/glite-lb-logger
ifdef LB_PERF
	${INSTALL} -m 755 ${top_srcdir}/src/perftest_ll.sh ${DESTDIR}${PREFIX}${prefix}/sbin
	${INSTALL} -m 755 ${top_srcdir}/src/perftest_il.sh ${DESTDIR}${PREFIX}${prefix}/sbin
endif
	( cd ${top_srcdir}/interface && ${INSTALL} -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix} )
	${INSTALL} -m 644 ${MAN_GZ} ${DESTDIR}${PREFIX}${prefix}/share/man/man8
ifeq ($(os_type),redhat)
	mkdir -p ${DESTDIR}${PREFIX}${sysconfdir}/rc.d/init.d
	${INSTALL} -m 755 startup ${DESTDIR}${PREFIX}${sysconfdir}/rc.d/init.d/glite-lb-locallogger
	for d in logd interlogd notif-interlogd proxy-interlogd; do \
		${INSTALL} -m 755 ${top_srcdir}/config/startup.redhat.$$d ${DESTDIR}${PREFIX}${sysconfdir}/rc.d/init.d/glite-lb-$$d; \
	done
endif
ifeq ($(os_type),systemd)
	mkdir -p ${DESTDIR}${PREFIX}${unitdir}
	for d in logd interlogd notif-interlogd proxy-interlogd; do \
		install -m 644 ${top_srcdir}/config/systemd.$$d ${DESTDIR}${PREFIX}${unitdir}/glite-lb-$$d.service ; \
	done
endif
ifeq ($(os_type),debian)
	mkdir -p ${DESTDIR}${PREFIX}${sysconfdir}/init.d
	${INSTALL} -m 755 startup ${DESTDIR}${PREFIX}${sysconfdir}/init.d/glite-lb-locallogger
	for d in logd interlogd notif-interlogd proxy-interlogd; do \
		${INSTALL} -m 755 ${top_srcdir}/config/startup.debian.$$d ${DESTDIR}${PREFIX}${sysconfdir}/init.d/glite-lb-$$d ; \
	done
endif

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
	rm -rvf .libs/ *.o *.lo *.no ${LOGD} ${INTERLOGD} ${NOTIF_INTERLOGD} ${MAN_GZ} ${SCRIPTS}
	rm -rvf check.ll check.il
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

.PHONY: default all compile man stage check install clean distclean dist distcheck
