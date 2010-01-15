top_srcdir=..
stagedir=.
package=glite-lb-harvester
module.version=0.0.0
PREFIX=/opt/glite
globus_prefix=/opt/globus

archlib:=lib
thrflavour:=gcc32dbgpthr
host_cpu:=${shell uname -m}
ifeq (${host_cpu},x86_64) 
        archlib:=lib64
	thrflavour:=gcc64dbgpthr
endif   

-include Makefile.inc
-include ../project/version.properties
version:=${module.version}

CC=gcc
VPATH=${top_srcdir}/src:${top_srcdir}/doc

GLOBUS_CPPFLAGS:=-I${globus_prefix}/include/${thrflavour}
CPPFLAGS:=-I${stagedir}/include -D_GNU_SOURCE -D_REENTRANT ${CPPFLAGS}
CFLAGS:=-W -Wall -g -O2 ${CFLAGS}
LDFLAGS:=${LDFLAGS} 
LIBS:=-L${stagedir}/${archlib} -L${stagedir}/lib \
	-lglite_lb_common_${thrflavour} \
	-lglite_lb_client_${thrflavour} \
	-lpthread -lglite_security_gss_${thrflavour}

ifneq ($(GLITE_LB_HARVESTER_WITH_LBU_DB),no)
CPPFLAGS:=$(CPPFLAGS) -DWITH_LBU_DB=1
LIBS:=$(LIBS) -lglite_lbu_db
endif
ifeq ($(GLITE_LB_HARVESTER_WITH_OLD_LB),yes)
CPPFLAGS:=${GLOBUS_CPPFLAGS} $(CPPFLAGS) -DWITH_OLD_LB=1
LIBS:=$(LIBS) -lglite_wmsutils_cjobid
else
LIBS:=$(LIBS) -lglite_jobid -lglite_lbu_trio
endif

COMPILE:=libtool --mode=compile ${CC} ${CPPFLAGS} ${CFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
INSTALL:=libtool --mode=install install

default: all

compile all: harvester doc

check:

debug: harvester-dbg

doc: glite-lb-harvester.8

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install: compile
	-mkdir -p ${PREFIX}/bin ${PREFIX}/share/doc/${package}-${version} ${PREFIX}/share/man/man8
	${INSTALL} -m 755 harvester ${PREFIX}/bin/glite-lb-harvester
	${INSTALL} -m 444 ../doc/README ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 444 glite-lb-harvester.8 ${PREFIX}/share/man/man8

clean:
	rm -rfv *.o *.lo *.loT .libs/ manpage.links manpage.refs
	rm -rvf harvester harvester-dbg glite-lb-harvester.*
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

harvester: harvester.o
	${LINK} -o $@ $+ ${LIBS}

harvester-dbg: harvester-dbg.o
	${LINK} -o $@ $+ ${LIBS}

harvester-dbg.o: harvester.c
	${COMPILE} -Werror -DLOG=1 -DWITH_RTM_SQL_STORAGE=1 -c $< -o $@

%.o: %.c
	${COMPILE} -c $<

%.8: %.sgml
	docbook2man $<
	mv $(@:.8=.1) $@

.PHONY: default all compile debug check doc stage install clean
