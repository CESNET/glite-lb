top_srcdir=..
stagedir=.
package=emi-lb-nagios-plugins
version=0.0.0
prefix=
INSTALL=install

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

SOURCES=src/LB-probe Makefile
SOURCES_EXEC=src/LB-probe

all:

install:
	mkdir -p ${DESTDIR}${PREFIX}/var/lib/grid-monitoring/emi.lb
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	chmod 750 ${DESTDIR}${PREFIX}/var/lib/grid-monitoring/emi.lb
	-chown nagios:nagios ${DESTDIR}${PREFIX}/var/lib/grid-monitoring/emi.lb
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/libexec/grid-monitoring/probes/emi.lb
	${INSTALL} -m 0755 src/LB-probe ${DESTDIR}${PREFIX}${prefix}/libexec/grid-monitoring/probes/emi.lb
	( cd ${top_srcdir}/project && ${INSTALL} -m 644 ChangeLog package.description package.summary ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version} )

stage:
	$(MAKE) install PREFIX=${stagedir}

check:
	@echo "No test"

clean:
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf Makefile.inc *.spec debian/

.PHONY: all install stage clean check distclean dist distcheck
