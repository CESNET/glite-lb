top_srcdir=..
stagedir=.
package=emi-nagios-lb
version=0.0.0
prefix=
LOCATION=/lib64/nagios/plugins/contrib/
INSTALL=install

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

install:
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/lib64/nagios/plugins/contrib
	${INSTALL} -m 0755 src/LB-probe ${DESTDIR}${PREFIX}${prefix}/lib64/nagios/plugins/contrib/check_lb_server

stage:
	$(MAKE) install PREFIX=${stagedir}

clean:

.PHONY: all install stage clean
