stagedir=../stage
top_srcdir=.

-include Makefile.inc

globalprefix=glite
lbprefix=lb

PM=StructField.pm MultiStruct.pm
T=events.T status.T types.T
SCRIPTS=at3 check_version.pl
HDRS=lb_types.h

default: compile

compile:

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install: compile
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/perl/gLite/LB ${DESTDIR}${PREFIX}${prefix}/share/glite-lb/at3 ${DESTDIR}${PREFIX}${prefix}/sbin ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix}
	for p in ${PM} ; do \
		install -m 644 ${top_srcdir}/$$p ${DESTDIR}${PREFIX}${prefix}/share/perl/gLite/LB; \
	done
	for p in ${T} ; do \
		install -m 644 ${top_srcdir}/$$p ${DESTDIR}${PREFIX}${prefix}/share/glite-lb/at3; \
	done
	install -m 755 ${top_srcdir}/at3.in ${DESTDIR}${PREFIX}${prefix}/sbin/glite-lb-at3
	install -m 755 ${top_srcdir}/check_version.pl ${DESTDIR}${PREFIX}${prefix}/sbin/glite-lb-check_version.pl
	(cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix}) 
	
clean:

distclean:
	rm -rvf Makefile.inc *.spec debian/

check:

.PHONY: default compile stage install check clean distclean
