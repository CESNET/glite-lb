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
VPATH=${top_srcdir}/src

GLOBUS_CPPFLAGS:=-I${globus_prefix}/include/${thrflavour}
CPPFLAGS:=${GLOBUS_CPPFLAGS} -I${stagedir}/include -D_GNU_SOURCE -D_REENTRANT ${CPPFLAGS}
CFLAGS:=-W -Wall -g -O2 ${CFLAGS}
LDFLAGS:=${LDFLAGS} 
LIBS:=-L${stagedir}/${archlib} -L${stagedir}/lib \
	-lglite_lb_common_${thrflavour} \
	-lglite_lb_client_${thrflavour} \
	-lpthread

ifneq ($(GLITE_LB_HARVESTER_WITH_LBU_DB),no)
CPPFLAGS:=$(CPPFLAGS) -DWITH_LBU_DB=1
LIBS:=$(LIBS) -lglite_lbu_db
endif
ifeq ($(GLITE_LB_HARVESTER_WITH_OLD_LB),yes)
CPPFLAGS:=$(CPPFLAGS) -DWITH_OLD_LB=1
endif

COMPILE:=libtool --mode=compile ${CC} ${CPPFLAGS} ${CFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
INSTALL:=libtool --mode=install install

default: all

compile all: harvester

check:

debug: harvester-dbg

doc:

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install: compile
	-mkdir -p ${PREFIX}/bin ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 755 harvester ${PREFIX}/bin/glite-lb-harvester
	${INSTALL} -m 444 ../doc/README ${PREFIX}/share/doc/${package}-${version}

clean:
	rm -rfv *.o *.lo .libs/ harvester harvester-dbg
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

harvester: harvester.o
	${LINK} -o $@ $+ ${LDFLAGS} ${LIBS}

harvester-dbg: harvester-dbg.o
	${LINK} -o $@ $+ ${LDFLAGS} ${LIBS}

harvester-dbg.o: harvester.c
	${COMPILE} -Werror -DLOG=1 -DWITH_RTM_SQL_STORAGE=1 -c $< -o $@

%.o: %.c
	${COMPILE} -c $<

.PHONY: default all compike debug check doc stage install clean
