include Makefile.inc

VPATH:=${src}

# XXX
VERSION=-DVERSION=\"glite-test\"

SUFFIXES=.no

DEBUG:=-g -O0
CFLAGS:=${DEBUG} \
	-I${stageinc} -I${src} \
	-I${repository}/${globus}/include/${globusflavour} \
	-I${repository}/${globus}/include/${globusflavour}/openssl \
	-D_GNU_SOURCE \
	${VERSION}

LDFLAGS:=-L${stagelib}
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
INSTALL:=libtool --mode=install install

GLOBUS_LIBS:= -L${repository}/${globus}/lib \
	-lglobus_common_${globusflavour} \
	-lssl_${globusflavour}

EXT_LIBS:= ${GLOBUS_LIBS} \
	-L${repository}/${ares}/lib -lares \
	-L${repository}/${expat}/lib -lexpat

HELPERS:=-lglite_wms_tls_ssl_helpers
PHELPERS:=-lglite_wms_tls_ssl_pthr_helpers
	
COMMON_LIB:=-lglite_lb_common

LOGD_OBJS:= logd_proto.o logd.o

INTERLOG_OBJS:=il_error.o input_queue_socket.o \
	recover.o send_event.o \
	event_queue.o event_store.o il_master.o interlogd.o \
	queue_mgr.o server_msg.o queue_thread.o

INTERLOG_NOBJS:=${INTERLOG_OBJS:.o=.no}

default: all

compile: glite_lb_logd glite_lb_interlogd glite_lb_notif_interlogd

stage export all: compile
	${INSTALL} -m 755 glite_lb_logd glite_lb_interlogd glite_lb_notif_interlogd ${stagebin}

check:
	echo No unit tests so far

glite_lb_logd: ${LOGD_OBJS}
	${LINK} -o $@ ${LOGD_OBJS} ${COMMON_LIB} ${HELPERS} ${EXT_LIBS} 

glite_lb_interlogd: ${INTERLOG_OBJS}
	${LINK} -o $@ ${INTERLOG_OBJS} \
		${COMMON_LIB} ${HELPERS} ${PHELPERS} ${EXT_LIBS} -lpthread

glite_lb_notif_interlogd: ${INTERLOG_NOBJS}
	${LINK} -o $@ ${INTERLOG_NOBJS} \
		${COMMON_LIB} ${HELPERS} ${PHELPERS} ${EXT_LIBS} -lpthread

%.no: %.c
	${CC} ${CFLAGS} -DIL_NOTIFICATIONS -c $< -o $@
