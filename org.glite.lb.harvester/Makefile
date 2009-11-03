top_srcdir=..
stagedir=.
package=glite-lb-harvester
version=0.0.0
PREFIX=/opt/glite

archlib:=lib
thrflavour:=gcc32dbgpthr
host_cpu:=${shell uname -m}
ifeq (${host_cpu},x86_64) 
        archlib:=lib64
	thrflavour:=gcc64dbgpthr
endif   

-include Makefile.inc
-include ../project/version.properties

CC=gcc
VPATH=${top_srcdir}/src

CPPFLAGS:=-I${stagedir}/include -D_GNU_SOURCE -D_REENTRANT ${CPPFLAGS}
CFLAGS:=-W -Wall -g -O2 ${CFLAGS}
LDFLAGS:=${LDFLAGS} 
LIBS:=-L${stagedir}/${archlib} -L${stagedir}/lib \
	-lglite_lb_common_${thrflavour} \
	-lglite_lb_client_${thrflavour} \
	-lglite_lbu_db \
	-lpthread

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
	-mkdir -p ${PREFIX}/bin ${PREFIX}/doc/${package}
	${INSTALL} -m 755 harvester ${PREFIX}/bin/glite-lb-harvester
	${INSTALL} -m 444 README ${PREFIX}/doc/${package}

clean:
	rm -rfv *.o *.lo .libs/ harvester harvester-dbg
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

harvester: harvester.o
	${LINK} -o $@ $+ ${LDFLAGS} ${LIBS}

harvester-dbg: harvester-dbg.o
	${LINK} -o $@ $+ ${LDFLAGS} ${LIBS}

harvester-dbg.o: harvester.c
	${COMPILE} -Werror -DLOG=1 -DRTM_SQL_STORAGE_ENABLED=1 -c $< -o $@

%.o: %.c
	${COMPILE} -c $<

.PHONY: default all compike debug check doc stage install clean
