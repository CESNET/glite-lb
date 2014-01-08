# defaults
top_srcdir=..
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-utils
version=0.0.0
PREFIX=/opt/glite
prefix=

gsoap_prefix=/opt/gsoap

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}
docdir?=${prefix}/share/doc/${package}-${version}

VPATH=${top_srcdir}/src:${top_srcdir}/doc

SOURCES=\
	doc/*.1 doc/README.* \
	examples/glite-lb-* \
	src/*.c src/*.c.T src/*.sh \
	Makefile
SOURCES_EXEC=examples/*.sh src/*.sh

CC=gcc
AT3=${stagedir}${prefix}/bin/glite-lb-at3

SUFFIXES = .T

DEBUG:=-g -O0 -Wall

GSOAP_CFLAGS?=-I${gsoap_prefix}/include -I${gsoap_prefix}/
GLITE_LBJP_COMMON_GSS_CFLAGS?=`PKG_CONFIG_PATH=$$PKG_CONFIG_PATH:${PREFIX}${prefix}/${libdir}/pkgconfig pkg-config glite-security-gss --cflags`

CFLAGS:= \
	${WS_CFLAGS} ${DEBUG} \
	-DVERSION=\"${version}\" \
	-I${stagedir}${prefix}/include -I${top_srcdir}/src -I. \
	-I${top_srcdir}/interface \
	${GSOAP_CFLAGS} \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE ${GLITE_LBJP_COMMON_GSS_CFLAGS} ${CFLAGS}

LDFLAGS:=${LDFLAGS} -L${stagedir}${prefix}/${libdir}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
INSTALL:=libtool --mode=install install
XSLTPROC:=xsltproc --novalid

EXT_LIBS:=

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

os=${shell uname}
DL_LIBS=
ifeq (${os},Linux)
	DL_LIBS=-ldl
endif

UTILS:=purge dump load
ALLUTILS:=${UTILS} mon dump_exporter state_history statistics
STATISTICS_OBJS:=statistics.o process_attrs.o process_attrs2.o

MAN_GZ:=glite-lb-dump.1.gz glite-lb-load.1.gz glite-lb-purge.1.gz glite-lb-mon.1.gz
MAN = $(MAN_GZ:.gz=)

compile all: ${ALLUTILS} ${MAN_GZ}

${UTILS}: %: %.o
	${LINK} -o $@ $< ${COMMON_LIB} ${CLIENT_LIB} ${EXT_LIBS} -lglite_lbu_maildir

mon: %: %.o
	${LINK} -o $@ $< ${COMMON_LIB} ${CLIENT_LIB} ${EXT_LIBS} -lglite_jobid

dump_exporter: %: %.o
	${LINK} -o $@ $< ${COMMON_LIB} ${CLIENT_LIB} ${EXT_LIBS} -lglite_jobid -lglite_lbu_maildir

statistics: ${STATISTICS_OBJS}
	${LINK} -rdynamic -o $@ ${STATISTICS_OBJS} ${COMMON_LIB} ${JP_LIBS} ${EXT_LIBS} -lglite_lbu_trio ${DL_LIBS}

state_history: state_history.o
	${LINK} -o $@ state_history.o -lglite_jobid ${COMMON_LIB} ${CLIENT_LIB} -lglite_lb_statemachine ${PLUS_LIB}

check: compile 

doc: ${MAN_GZ}

%.1.gz: %.1
	gzip -9 -f -c $< > $@

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

install:
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/bin 
	-mkdir -p ${DESTDIR}${PREFIX}${docdir}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/man/man1
	${INSTALL} -m 644 ${top_srcdir}/doc/README* ${DESTDIR}${PREFIX}${docdir}
	${INSTALL} -m 644 ${MAN_GZ} ${DESTDIR}${PREFIX}${prefix}/share/man/man1
	${INSTALL} -m 755 ${top_srcdir}/src/glite-lb-bkpurge-offline.sh ${DESTDIR}${PREFIX}${prefix}/bin/glite-lb-bkpurge-offline

	for p in ${ALLUTILS} ; do \
		${INSTALL} -m 755 "$$p" "${DESTDIR}${PREFIX}${prefix}/bin/glite-lb-$$p"; \
	done

clean:
	rm -rfv ${ALLUTILS} ${MAN_GZ} *.lo *.o .libs/
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

%.o: %.c
	${COMPILE} -c $<

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

