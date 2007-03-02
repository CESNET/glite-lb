# defaults
top_srcdir=.
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

-include Makefile.inc
-include ../project/version.properties

version=${module.version}

CC=gcc

VPATH:=${top_srcdir}/src:${top_srcdir}/test

VERSION=-DVERSION=\"GLite-${version}\"

SUFFIXES=.no

GLOBUSINC=-I${globus_prefix}/include/${nothrflavour}

GLOBUSTHRINC=-I${globus_prefix}/include/${thrflavour}

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
endif

DEBUG:=-g -O0
CFLAGS:=${DEBUG} \
	-I${stagedir}/include -I${top_srcdir}/src \
	-D_GNU_SOURCE \
	${COVERAGE_FLAGS} \
	${VERSION} ${LB_STANDALONE_FLAGS} ${LB_PERF_FLAGS} 

LDFLAGS:=-L${stagedir}/lib \
	${COVERAGE_FLAGS} 
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
LINKXX:=libtool --mode=link ${CXX} -rpath ${stagedir}/lib ${LDFLAGS} 
INSTALL:=libtool --mode=install install

GLOBUS_LIBS:= -L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lglobus_gssapi_gsi_${nothrflavour}

GLOBUS_THRLIBS:= -L${globus_prefix}/lib \
	-lglobus_common_${thrflavour} \
	-lglobus_gssapi_gsi_${thrflavour}

#ifneq (${expat_prefix},/usr)
#	EXPAT_LIBS:=-L${expat_prefix}/lib
#endif
#EXPAT_LIBS:=${EXPAT_LIBS} -lexpat
#
#EXT_LIBS:= ${EXPAT_LIBS}

COMMON_LIB:=-lglite_lb_common

GLITE_GSS_LIB:=-lglite_security_gss

TEST_LIBS:=-L${cppunit_prefix}/lib -lcppunit
TEST_INC:=-I${cppunit_prefix}/include

LOGD_OBJS:= logd_proto.o logd.o

LOGD_NOBJS:=${LOGD_OBJS:.o=.no}

INTERLOG_OBJS:=il_error.o input_queue_socket.o \
	recover.o send_event.o \
	event_queue.o event_store.o il_master.o interlogd.o \
	queue_mgr.o server_msg.o queue_thread.o

INTERLOG_NOBJS:=${INTERLOG_OBJS:.o=.no}
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
	event_queue.o \
	event_queueTest.o \
	IlTestBase.o \
	il_test.o 

default: all 

all compile: $(LOGD) $(INTERLOGD) $(NOTIF_INTERLOGD)

glite-lb-logd: ${LOGD_OBJS}
	${LINK} -o $@ ${LOGD_OBJS} ${COMMON_LIB}_${nothrflavour} 

glite-lb-logd-perf: ${LOGD_OBJS}
	${LINK} -o $@ ${LOGD_OBJS} ${COMMON_LIB}_${nothrflavour} 

glite-lb-logd-nofile: ${LOGD_NOBJS}
	${LINK} -o $@ ${LOGD_NOBJS} ${COMMON_LIB}_${nothrflavour} 

glite-lb-logd-perf-nofile: ${LOGD_NOBJS}
	${LINK} -o $@ ${LOGD_NOBJS} ${COMMON_LIB}_${nothrflavour} 

glite-lb-interlogd: ${INTERLOG_OBJS}
	${LINK} -o $@ ${INTERLOG_OBJS} ${COMMON_LIB}_${thrflavour} 

glite-lb-notif-interlogd: ${INTERLOG_NOBJS}
	${LINK} -o $@ ${INTERLOG_NOBJS} ${COMMON_LIB}_${thrflavour}

glite-lb-interlogd-perf: ${INTERLOG_PERF_OBJS}
	${LINK} -o $@ ${INTERLOG_PERF_OBJS} ${COMMON_LIB}_${thrflavour}

glite-lb-interlogd-perf-empty: ${INTERLOG_EMPTY_OBJS}
	${LINK} -o $@ ${INTERLOG_EMPTY_OBJS} ${COMMON_LIB}_${thrflavour}

#glite-lb-interlogd-perf-inline-empty: ${INTERLOG_INLINE_EMPTY_OBJS}
#	${LINK} -o $@ ${INTERLOG_INLINE_EMPTY_OBJS} \
#		${COMMON_LIB}_${thrflavour}

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

check: 
# do nothing until test/ is really added to CVS
# check.ll check.il

#check.ll: logd_proto_test.o ll_test.o
#	${LINKXX} -o $@ ${COMMON_LIB}_${nothrflavour} ${EXT_LIBS} ${GLOBUS_LIBS} ${TEST_LIBS} $+
#	./check.ll

check.ll:
	-echo commented out -- fix needed

check.il: ${INTERLOG_TEST_OBJS}
	${LINKXX} -o $@ ${COMMON_LIB}_${thrflavour} ${GLITE_GSS_LIB}_${nothrflavour} ${TEST_LIBS} -lpthread $+

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
	-mkdir -p ${PREFIX}/bin
	-mkdir -p ${PREFIX}/etc/init.d
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 755 ${LOGD} ${PREFIX}/bin
	${INSTALL} -m 755 ${INTERLOGD} ${PREFIX}/bin
	if [ x${DOSTAGE} = xyes ]; then \
		${INSTALL} -m 755 ${NOTIF_INTERLOGD} ${PREFIX}/bin; \
	fi
ifdef LB_PERF
	${INSTALL} -m 755 ${top_srcdir}/src/perftest_ll.sh ${PREFIX}/sbin
	${INSTALL} -m 755 ${top_srcdir}/src/perftest_il.sh ${PREFIX}/sbin
endif
	${INSTALL} -m 755 ${top_srcdir}/config/startup ${PREFIX}/etc/init.d/glite-lb-locallogger
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}

${INTERLOG_NOBJS}: %.no: %.c
	${CC} ${CFLAGS} ${GLOBUSTHRINC} -DIL_NOTIFICATIONS -c $< -o $@

${INTERLOG_OBJS}: %.o: %.c
	${CC} ${CFLAGS} ${GLOBUSTHRINC} -c $< -o $@

${INTERLOG_EMPTY_OBJS}: %.empty.o: %.c
	${CC} ${CFLAGS} ${GLOBUSTHRINC} -DPERF_EMPTY -c $< -o $@

${INTERLOG_PERF_OBJS}: %.perf.o: %.c
	${CC} ${CFLAGS} ${GLOBUSTHRINC} -c $< -o $@

#${INTERLOG_INLINE_EMPTY_OBJS}: %.io: %.c
#	${CC} ${CFLAGS} ${GLOBUSTHRINC} -DLB_PERF -DPERF_EMPTY -DPERF_EVENTS_INLINE -c $< -o $@

${LOGD_NOBJS}: %.no: %.c
	${CC} ${CFLAGS} ${GLOBUSINC} -DLOGD_NOFILE -c $< -o $@

${LOGD_OBJS}: %.o: %.c
	${CC} ${CFLAGS} ${GLOBUSINC} -c $< -o $@

logd_proto_test.o: %.o: %.c
	${CC} ${CFLAGS} ${GLOBUSINC} -c $< -o $@

ll_test.o: %.o: %.cpp
	${CXX} ${CFLAGS} ${TEST_INC} -c $< -o $@

il_test.o IlTestBase.o server_msgTest.o event_queueTest.o input_queue_socketTest.o event_storeTest.o: %.o: %.cpp
	${CXX} ${CFLAGS} ${GLOBUSTHRINC} ${TEST_INC} -c $< -o $@
