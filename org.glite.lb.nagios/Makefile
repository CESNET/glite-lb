top_srcdir=..
stagedir=.
package=emi-nagios-lb
version=0.0.0
prefix=
INSTALL=install

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

install:
	mkdir -p ${DESTDIR}${PREFIX}/var/lib/grid-monitoring/emi.lb
	chmod 750 ${DESTDIR}${PREFIX}/var/lib/grid-monitoring/emi.lb
	-chown nagios:nagios ${DESTDIR}${PREFIX}/var/lib/grid-monitoring/emi.lb
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/libexec/grid-monitoring/probes/emi.lb
	${INSTALL} -m 0755 src/LB-probe ${DESTDIR}${PREFIX}${prefix}/libexec/grid-monitoring/probes/emi.lb

stage:
	$(MAKE) install PREFIX=${stagedir}

check:
	@echo "No test"

clean:

.PHONY: all install stage clean check
