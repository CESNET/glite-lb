top_srcdir=..
stagedir=.
package=glite-lb-harvester
PREFIX=/opt/glite
prefix=
sysconfdir=/opt/glite/etc
globus_prefix=/opt/globus
libdir=lib

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

CC=gcc
VPATH=${top_srcdir}/src:${top_srcdir}/doc

SOURCES=\
	config/systemd.* config/startup config/startup.* \
	doc/* \
	examples/test.* \
	project/.post* project/.pre* \
	src/*.c \
	Makefile
SOURCES_EXEC=config/startup config/startup.* examples/*.sh

GLITE_LBJP_COMMON_GSS_CFLAGS?=`PKG_CONFIG_PATH=$$PKG_CONFIG_PATH:${PREFIX}${prefix}/${libdir}/pkgconfig pkg-config glite-security-gss --cflags`
CPPFLAGS:=-I${stagedir}${prefix}/include -D_GNU_SOURCE -D_REENTRANT ${GLITE_LBJP_COMMON_GSS_CFLAGS} ${CPPFLAGS}
CFLAGS:=-W -Wall -g -O2 ${CFLAGS}
LDFLAGS:=${LDFLAGS} 
ifeq (${thrflavour},)
LIBS:=-L${stagedir}${prefix}/${libdir} \
	-lglite_lb_common \
	-lglite_lb_client \
	-lpthread -lglite_security_gss
else
LIBS:=-L${stagedir}${prefix}/${libdir} \
	-lglite_lb_common_${thrflavour} \
	-lglite_lb_client_${thrflavour} \
	-lpthread -lglite_security_gss_${thrflavour}
endif

ifeq ($(os_type),debian)
sysdefaultdir=${sysconfdir}/default
else
sysdefaultdir=${sysconfdir}/sysconfig
endif

ifeq ($(os_type),fedora)
unitdir=`pkg-config systemd --variable=systemdsystemunitdir`
endif

ifneq ($(GLITE_LB_HARVESTER_WITH_LBU_DB),no)
CPPFLAGS:=$(CPPFLAGS) -DWITH_LBU_DB=1
LIBS:=$(LIBS) -lglite_lbu_db
endif
ifeq ($(GLITE_LB_HARVESTER_WITH_OLD_LB),yes)
GLOBUS_GSSAPI_GSI_CFLAGS?=-I${globus_prefix}/include/${thrflavour}
CPPFLAGS:=${GLOBUS_GSSAPI_GSI_CFLAGS} $(CPPFLAGS) -DWITH_OLD_LB=1
LIBS:=$(LIBS) -lglite_wmsutils_cjobid
else
LIBS:=$(LIBS) -lglite_jobid -lglite_lbu_trio -lglite_lbu_log
endif

COMPILE:=libtool --mode=compile ${CC} ${CPPFLAGS} ${CFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
INSTALL:=libtool --mode=install install

compile all: startup harvester doc debug

startup: ${top_srcdir}/config/startup
	glite_var="${localstatedir}/glite"; \
	if echo "${localstatedir}" | grep 'glite'>/dev/null; then \
		glite_var="${localstatedir}"; \
	fi; \
	sed -e 's:@glite_prefix@:${sysroot}${prefix}:g' -e 's:@glite_etc@:${sysconfdir}:g' -e "s:@glite_var@:$$glite_var:g" -e 's:@defaultdir@:${sysdefaultdir}:g' $< > $@
ifneq ($(os_type),debian)
	sed -i 's/# \(Default-Start\|Default-Stop\):.*/# \1:/' $@
endif
	chmod +x $@

check:

debug: harvester-dbg

doc: glite-lb-harvester.1

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install: compile
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/bin ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples \
		${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version} \
		${DESTDIR}${PREFIX}${prefix}/share/glite \
		${DESTDIR}${PREFIX}${prefix}/share/man/man1 \
		${DESTDIR}${PREFIX}${sysconfdir}/glite-lb
	${INSTALL} -m 755 harvester ${DESTDIR}${PREFIX}${prefix}/bin/glite-lb-harvester
	${INSTALL} -m 755 harvester-dbg ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples/glite-lb-harvester-dbg
	${INSTALL} -m 755 ${top_srcdir}/examples/test.sh ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples/glite-lb-harvester-test.sh
	${INSTALL} -m 444 ${top_srcdir}/examples/test.sql ${DESTDIR}${PREFIX}${prefix}/share/glite/glite-lb-harvester-test-dbsetup.sql
	${INSTALL} -m 444 ${top_srcdir}/doc/README ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	${INSTALL} -m 444 glite-lb-harvester.1 ${DESTDIR}${PREFIX}${prefix}/share/man/man1
ifeq ($(os_type),fedora)
	mkdir -p ${DESTDIR}${PREFIX}${unitdir}
	${INSTALL} -m 644 ${top_srcdir}/config/systemd.harvester ${DESTDIR}${PREFIX}${unitdir}/glite-lb-harvester.service
else
	mkdir -p ${DESTDIR}${PREFIX}${sysconfdir}/init.d
	${INSTALL} -m 755 startup ${DESTDIR}${PREFIX}${sysconfdir}/init.d/glite-lb-harvester
endif

clean:
	rm -rfv *.o *.lo *.loT .libs/ manpage.links manpage.refs *.log
	rm -rvf harvester harvester-dbg glite-lb-harvester.1 startup
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

harvester: harvester.o
	${LINK} -o $@ $+ ${LIBS}

harvester-dbg: harvester-dbg.o
	${LINK} -o $@ $+ ${LIBS}

harvester-dbg.o: harvester.c
	${COMPILE} -DLOG=1 -DWITH_RTM_SQL_STORAGE=1 -c $< -o $@

%.o: %.c
	${COMPILE} -c $<

%.1: %.sgml
	docbook2man $<

.PHONY: default all compile debug check doc stage install clean distclean dist distcheck
