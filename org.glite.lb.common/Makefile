include Makefile.inc


VPATH:=${src}
AT3:=perl -I${lbconfig} ${lbproject}/at3

SUFFIXES = .T

DEBUG:=-g -O0
CFLAGS:=${DEBUG} -I${src} -I${repository}/${globus}/include/${globusflavour} \
	-I${repository}/${globus}/include/${globusflavour}/openssl \
	-I${repository}/${ares}/include -I${repository}/${expat}/include\
	-I${stageinc} -I${interface} \
	-DDATAGRID_EXTENSION

LDFLAGS:=-L${stagelib}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagelib} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

OBJS:=dgssl.o escape.o events.o mini_http.o query_rec.o status.o \
	xml_conversions.o xml_parse.o ulm_parse.o param.o \
	events_parse.o il_string.o il_int.o notifid.o \
	il_log.o il_msg.o context.o trio.o strio.o
LOBJS:=`echo ${OBJS} | sed 's/\.o/\.lo/g'`

HDRS:=context-int.h  dgssl.h  mini_http.h authz.h xml_parse.h \
	xml_conversions.h log_proto.h events_parse.h il_string.h escape.h \
	ulm_parse.h trio.h

STATICLIB:=libglite_lb_common.a
LTLIB:=libglite_lb_common.la

default: all

compile: ${STATICLIB} ${LTLIB}

${STATICLIB}: ${OBJS}
	ar crv $@ ${OBJS}
	ranlib $@

${LTLIB}: ${OBJS}
	${LINK} -o $@ ${LOBJS} \
		-L${stagelib} -lglobus_ssl_utils -lglite_wms_cjobid \
		-lm

stage export all: compile
	${INSTALL} -m 644 ${STATICLIB} ${stagelib}
	${INSTALL} -m 644 ${LTLIB} ${stagelib}
	cd ${interface} && install -m 644 ${HDRS} ${stageinc}/${globalprefix}/${lbprefix}


check:
	echo Unit tests missing

%.o: %.c
	${COMPILE} -c $<


%.h: %.h.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null
