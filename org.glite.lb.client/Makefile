include Makefile.inc


VPATH:=${src} 
AT3:=perl -I${lbconfig} ${lbproject}/at3

SUFFIXES = .T 

DEBUG:=-g -O0 
CFLAGS:=${DEBUG} \
	-I${stageinc} -I${src} -I${interface}\
	-I${repository}/${globus}/include/${globusflavour} \
        -I${repository}/${globus}/include/${globusflavour}/openssl \
	-I${repository}/${expat}/include

CXXFLAGS:=${CFLAGS}
LDFLAGS:=-L${stagelib}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

GLOBUS_LIBS:= -L${repository}/${globus}/lib \
	-lglobus_common_${globusflavour} \
	-lssl_${globusflavour}

EXT_LIBS:= ${GLOBUS_LIBS} \
	-L${repository}/${ares}/lib -lares \
	-L${repository}/${expat}/lib -lexpat \

LIBOBJS:=connection.o consumer.o notification.o prod_proto.o \
	producer.o uiwrap.o

PLUSOBJS:=Event.o Job.o JobStatus.o Notification.o ServerConnection.o
PUB_HDRS:=CountRef.h Event.h JobJobStatus.h Notification.h ServerConnection.h \
	LoggingExceptions.h

LIBLOBJS:=`echo ${LIBOBJS} | sed 's/\.o/\.lo/g'`
PLUSLOBJS:=`echo ${PLUSOBJS} | sed 's/\.o/\.lo/g'`
HELPERS:=-L${stagelib} -lglite_wms_tls_ssl_helpers

LIB:=libglite_lb_client.la
PLUSLIB:=libglite_lb_clientpp.la

TOOLS:=dump load purge

default: all

compile: ${LIB} ${TOOLS} logevent

check:
	echo No unit tests so far.

${LIB}: ${LIBOBJS}
	${LINK} -o $@ ${LIBLOBJS} -rpath ${stagelib} -lglite_lb_common ${HELPERS}
# ${EXT_LIBS}

${PLUSLIB}: ${PLUSOBJS}
	${LINK} -o $@ ${PLUSLOBJS} -rpath ${stagelib} ${LIB}

stage export all: compile
	${INSTALL} -m 644 ${LIB} ${stagelib}
	for p in ${TOOLS} logevent; do \
		${INSTALL} -m 755 "$$p" "${stagebin}/glite-lb-$$p"; \
	done

logevent: logevent.o args.o
	${LINK} -o $@ logevent.o args.o ${LIB} ${EXT_LIBS}

${TOOLS}: %: %.o
	${LINK} -o $@ $< ${LIB} ${EXT_LIBS}

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


