# defaults
top_srcdir=.
stagedir=.
distdir=.
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
DEBUG:=-g -O0 -Wall
CFLAGS:= \
	${WS_CFLAGS} ${DEBUG} \
	-DVERSION=\"${version}\" \
	-I${stagedir}/include -I${top_srcdir}/src -I. \
	-I${top_srcdir}/interface \
	-I${expat_prefix}/include \
	-I${gsoap_prefix}/include -I${gsoap_prefix}/ \
	${COVERAGE_FLAGS} \
	-I${mysql_prefix}/include -I${mysql_prefix}/include/mysql \
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
JP_LIBS:=-lglite_jp_common_${nothrflavour} -lglite_jp_trio

UTILS:=mon purge dump load dump_exporter

MAN_GZ:=glite-lb-mon.1.gz
MAN = $(MAN_GZ:.gz=)


default: all

compile all: ${UTILS} ${MAN_GZ} statistics

%.o: %.c 
	${CC} ${CFLAGS} -c $<

${UTILS}: %: %.o
	${LINK} -o $@ $< ${COMMON_LIB} ${CLIENT_LIB} ${EXT_LIBS}

statistics: statistics.o
	${LINK} -rdynamic -o $@ $< ${COMMON_LIB} ${JP_LIBS} ${EXT_LIBS} 

check: compile 

doc: ${MAN_GZ}

${MAN_GZ}: ${MAN}
	cp $? .
	gzip -f $(notdir $?)

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

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
	-mkdir -p ${PREFIX}/lib
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}
	-mkdir -p ${PREFIX}/share/man/man1
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${top_srcdir}/doc/README* ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${MAN_GZ} ${PREFIX}/share/man/man1

	for p in ${UTILS} ; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/bin/glite-lb-$$p"; \
	done

	-if [ x${DOSTAGE} != xyes ]; then \
		${INSTALL} -m 755 ${stagedir}/lib/glite_lb_plugin.so ${PREFIX}/lib; \
	fi

clean:
	rm -rfv ${UTILS} ${MAN_GZ} *.{lo,o}
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

%.o: %.c
	${COMPILE} -c $<

