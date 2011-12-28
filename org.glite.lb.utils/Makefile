# defaults
top_srcdir=..
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-utils
version=0.0.0
PREFIX=/opt/glite
prefix=

expat_prefix=/opt/expat
gsoap_prefix=/opt/gsoap

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/doc

CC=gcc
AT3=${stagedir}${prefix}/sbin/glite-lb-at3

SUFFIXES = .T

DEBUG:=-g -O0 -Wall

ifneq (${expat_prefix},/usr)
	expatlib := -L${expat_prefix}/${libdir} -L${expat_prefix}/lib
endif
EXPAT_CFLAGS?=-I${expat_prefix}/include
EXPAT_LIB?=${expatlib} -lexpat
GSOAP_CFLAGS?=-I${gsoap_prefix}/include -I${gsoap_prefix}/

CFLAGS:= ${CFLAGS} \
	${WS_CFLAGS} ${DEBUG} \
	-DVERSION=\"${version}\" \
	-I${stagedir}${prefix}/include -I${top_srcdir}/src -I. \
	-I${top_srcdir}/interface \
	${EXPAT_CFLAGS} \
	${GSOAP_CFLAGS} \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE
LDFLAGS:=${LDFLAGS} -L${stagedir}${prefix}/${libdir}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}${prefix}/${libdir}  ${LDFLAGS} 
SOLINK:=libtool --mode=link ${CC} -module ${LDFLAGS} -rpath ${stagedir}${prefix}/${libdir}
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install
LINKXX:=libtool --mode=link  ${CXX} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS}
XSLTPROC:=xsltproc --novalid

EXT_LIBS:= ${EXPAT_LIBS}

ifeq (${thrflavour},)
COMMON_LIB:=-lglite_lb_common
CLIENT_LIB:=-lglite_lb_client
PLUS_LIB:=-lglite_lb_clientpp
JP_LIBS:=-lglite_jp_common
else
COMMON_LIB:=-lglite_lb_common_${nothrflavour}
CLIENT_LIB:=-lglite_lb_client_${nothrflavour}
PLUS_LIB:=-lglite_lb_clientpp_${nothrflavour}
JP_LIBS:=-lglite_jp_common_${nothrflavour}
endif

UTILS:=mon purge dump load dump_exporter
ALLUTILS:=${UTILS} state_history statistics
STATISTICS_OBJS:=statistics.o process_attrs.o process_attrs2.o

MAN_GZ:=glite-lb-mon.1.gz
MAN8_GZ:=glite-lb-purge.8.gz
MAN = $(MAN_GZ:.gz=)
MAN8 = $(MAN8_GZ:.gz=)

default: all

compile all: ${ALLUTILS} ${MAN_GZ} ${MAN8_GZ}

${UTILS}: %: %.o
	${LINK} -o $@ $< ${COMMON_LIB} ${CLIENT_LIB} ${EXT_LIBS} -lglite_lbu_maildir

statistics: ${STATISTICS_OBJS}
	${LINK} -rdynamic -o $@ ${STATISTICS_OBJS} ${COMMON_LIB} ${JP_LIBS} ${EXT_LIBS} 

state_history: state_history.o
	${LINK} -o $@ state_history.o -lglite_lb_statemachine ${PLUS_LIB}

check: compile 

doc: ${MAN_GZ} ${MAN8_GZ}

${MAN_GZ}: ${MAN}
	cp $? .
	gzip -9 -f $(notdir $?)
${MAN8_GZ}: ${MAN8}
	cp $? .
	gzip -9 -f $(notdir $?)

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

install:
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/bin 
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/man/man1
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/man/man8
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	( cd ${top_srcdir}/project && ${INSTALL} -m 644 ChangeLog package.description package.summary ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version} )
	${INSTALL} -m 644 ${top_srcdir}/doc/README* ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${MAN_GZ} ${DESTDIR}${PREFIX}${prefix}/share/man/man1
	${INSTALL} -m 644 ${MAN8_GZ} ${DESTDIR}${PREFIX}${prefix}/share/man/man8
	${INSTALL} -m 755 ${top_srcdir}/src/glite-lb-bkpurge-offline.sh ${DESTDIR}${PREFIX}${prefix}/bin/glite-lb-bkpurge-offline

	for p in ${ALLUTILS} ; do \
		${INSTALL} -m 755 "$$p" "${DESTDIR}${PREFIX}${prefix}/bin/glite-lb-$$p"; \
	done

clean:
	rm -rfv ${ALLUTILS} ${MAN_GZ} ${MAN8_GZ} *.lo *.o .libs/

distclean:
	rm -rvf Makefile.inc *.spec debian/

%.o: %.c
	${COMPILE} -c $<

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

