top_srcdir=..
stagedir=.
package=emi-lb-nagios-plugins
version=0.0.0
prefix=
INSTALL=install

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

SOURCES=src/*-probe Makefile
SOURCES_EXEC=src/*-probe

all:

install:
	mkdir -p ${DESTDIR}${PREFIX}/var/lib/grid-monitoring/emi.lb
	chmod 750 ${DESTDIR}${PREFIX}/var/lib/grid-monitoring/emi.lb
	-chown nagios:nagios ${DESTDIR}${PREFIX}/var/lib/grid-monitoring/emi.lb
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/libexec/grid-monitoring/probes/emi.lb
	${INSTALL} -m 0755 src/LB-probe ${DESTDIR}${PREFIX}${prefix}/libexec/grid-monitoring/probes/emi.lb
	${INSTALL} -m 0755 src/IL-probe ${DESTDIR}${PREFIX}${prefix}/libexec/grid-monitoring/probes/emi.lb

stage:
	$(MAKE) install PREFIX=${stagedir}

check:
	@echo "No test"

clean:
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

.PHONY: default all install stage clean check distclean dist distcheck
