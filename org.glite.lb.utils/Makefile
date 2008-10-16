# defaults
top_srcdir=.
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-utils
version=0.0.0
PREFIX=/opt/glite

nothrflavour=gcc32
thrflavour=gcc32pthr
expat_prefix=/opt/expat
gsoap_prefix=/opt/gsoap

-include Makefile.inc
-include ../project/version.properties

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/doc

CC=gcc
AT3=${stagedir}/sbin/glite-lb-at3

SUFFIXES = .T

DEBUG:=-g -O0 -Wall

CFLAGS:= \
	${WS_CFLAGS} ${DEBUG} \
	-DVERSION=\"${version}\" \
	-I${stagedir}/include -I${top_srcdir}/src -I. \
	-I${top_srcdir}/interface \
	-I${expat_prefix}/include \
	-I${gsoap_prefix}/include -I${gsoap_prefix}/ \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE
LDFLAGS:=-L${stagedir}/lib

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/lib  ${LDFLAGS} 
SOLINK:=libtool --mode=link ${CC} -module ${LDFLAGS} -rpath ${stagedir}/lib
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install
LINKXX:=libtool --mode=link  ${CXX} -rpath ${stagedir}/lib ${LDFLAGS}
XSLTPROC:=xsltproc --novalid

ifneq (${expat_prefix},/usr)
	expatlib := -L${expat_prefix}/lib
endif

EXT_LIBS:= \
	${expatlib} -lexpat
COMMON_LIB:=-lglite_lb_common_${nothrflavour}
CLIENT_LIB:=-lglite_lb_client_${nothrflavour}
PLUS_LIB:=-lglite_lb_clientpp_${nothrflavour}
JP_LIBS:=-lglite_jp_common_${nothrflavour}

UTILS:=mon purge dump load dump_exporter
ALLUTILS:=${UTILS} state_history statistics
STATISTICS_OBJS:=statistics.o process_attrs.o process_attrs2.o

MAN_GZ:=glite-lb-mon.1.gz
MAN = $(MAN_GZ:.gz=)


default: all

compile all: ${ALLUTILS} ${MAN_GZ}

${UTILS}: %: %.o
	${LINK} -o $@ $< ${COMMON_LIB} ${CLIENT_LIB} ${EXT_LIBS} -lglite_lbu_maildir

statistics: ${STATISTICS_OBJS}
	${LINK} -rdynamic -o $@ ${STATISTICS_OBJS} ${COMMON_LIB} ${JP_LIBS} ${EXT_LIBS} 

state_history: state_history.o
	${LINK} -o $@ state_history.o -L${stagedir}/lib -lglite_lb_statemachine ${PLUS_LIB}

check: compile 

doc: ${MAN_GZ}

${MAN_GZ}: ${MAN}
	cp $? .
	gzip -f $(notdir $?)

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

install:
	-mkdir -p ${PREFIX}/bin 
	-mkdir -p ${PREFIX}/lib
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}
	-mkdir -p ${PREFIX}/share/man/man1
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	( cd ${top_srcdir}/project && ${INSTALL} -m 644 ChangeLog package.description package.summary ${PREFIX}/share/doc/${package}-${version} )
	${INSTALL} -m 644 ${top_srcdir}/doc/README* ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${MAN_GZ} ${PREFIX}/share/man/man1
	${INSTALL} -m 755 ${top_srcdir}/src/glite-lb-bkpurge-offline.sh ${PREFIX}/bin/glite-lb-bkpurge-offline

	for p in ${ALLUTILS} ; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/bin/glite-lb-$$p"; \
	done

clean:
	rm -rfv ${UTILS} statistics ${MAN_GZ} *.{lo,o} .libs/
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

%.o: %.c
	${COMPILE} -c $<

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

