top_srcdir=..
stagedir=.
package=glite-lb-harvester
module.version=0.0.0
PREFIX=/opt/glite
prefix=
sysconfdir=/opt/glite/etc
globus_prefix=/opt/globus
libdir=lib

-include Makefile.inc
-include ${top_srcdir}/project/version.properties
version:=${module.version}

CC=gcc
VPATH=${top_srcdir}/src:${top_srcdir}/doc

CPPFLAGS:=-I${stagedir}${prefix}/include -D_GNU_SOURCE -D_REENTRANT ${CPPFLAGS}
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

default: all

compile all: startup harvester doc debug

startup: ${top_srcdir}/config/startup
	glite_var="${localstatedir}/glite"; \
	if echo "${localstatedir}" | grep 'glite'>/dev/null; then \
		glite_var="${localstatedir}"; \
	fi; \
	sed -e 's:@glite_prefix@:${sysroot}${prefix}:' -e 's:@glite_etc@:${sysconfdir}:' -e "s:@glite_var@:$$glite_var:" $< > $@
	chmod +x $@

check:

debug: harvester-dbg

doc: glite-lb-harvester.8

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install: compile
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/bin ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples \
		${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version} \
		${DESTDIR}${PREFIX}${prefix}/share/man/man8 \
		${DESTDIR}${PREFIX}${sysconfdir}/init.d \
		${DESTDIR}${PREFIX}${sysconfdir}/glite-lb
	${INSTALL} -m 755 harvester ${DESTDIR}${PREFIX}${prefix}/bin/glite-lb-harvester
	${INSTALL} -m 755 harvester-dbg ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples/glite-lb-harvester-dbg
	${INSTALL} -m 755 ${top_srcdir}/examples/test.sh ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples/glite-lb-harvester-test.sh
	${INSTALL} -m 444 ${top_srcdir}/examples/test.sql ${DESTDIR}${PREFIX}${sysconfdir}/glite-lb/harvester-test-dbsetup.sql
	${INSTALL} -m 444 ${top_srcdir}/doc/README ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	${INSTALL} -m 444 glite-lb-harvester.8 ${DESTDIR}${PREFIX}${prefix}/share/man/man8
	${INSTALL} -m 755 startup ${DESTDIR}${PREFIX}${sysconfdir}/init.d/glite-lb-harvester

clean:
	rm -rfv *.o *.lo *.loT .libs/ manpage.links manpage.refs
	rm -rvf harvester harvester-dbg glite-lb-harvester.* startup

distclean:
	rm -rvf Makefile.inc *.spec debian/

harvester: harvester.o
	${LINK} -o $@ $+ ${LIBS}

harvester-dbg: harvester-dbg.o
	${LINK} -o $@ $+ ${LIBS}

harvester-dbg.o: harvester.c
	${COMPILE} -DLOG=1 -DWITH_RTM_SQL_STORAGE=1 -c $< -o $@

%.o: %.c
	${COMPILE} -c $<

%.8: %.sgml
	docbook2man $<
	mv $(@:.8=.1) $@

.PHONY: default all compile debug check doc stage install clean distclean
