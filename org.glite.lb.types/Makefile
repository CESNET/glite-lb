stagedir=../stage
top_srcdir=.
package=glite-lb-types

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

globalprefix=glite
lbprefix=lb
perlinstalldir=$(shell eval `perl -V:installvendorlib`; echo $$installvendorlib)

VPATH=${top_srcdir}

SOURCES=interface/*.h at3.in *.pm *.T check_version.pl Makefile
SOURCES_EXEC=at3.in check_version.pl

PM=StructField.pm MultiStruct.pm
T=events.T status.T types.T
SCRIPTS=at3 check_version.pl
HDRS=lb_types.h

all: compile

compile: at3

at3: at3.in
	sed -e 's:@prefix@:${PREFIX}${prefix}:' -e 's:@perlinstalldir@:${PREFIX}${perlinstalldir}:' $< > $@ || ( rm -f $@; false )
	chmod +x $@

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install: compile
	mkdir -p ${DESTDIR}${PREFIX}${perlinstalldir}/gLite/LB ${DESTDIR}${PREFIX}${prefix}/share/glite-lb/at3 ${DESTDIR}${PREFIX}${prefix}/bin ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix}
	for p in ${PM} ; do \
		install -m 644 ${top_srcdir}/$$p ${DESTDIR}${PREFIX}${perlinstalldir}/gLite/LB; \
	done
	for p in ${T} ; do \
		install -m 644 ${top_srcdir}/$$p ${DESTDIR}${PREFIX}${prefix}/share/glite-lb/at3; \
	done
	install -m 755 ${top_srcdir}/at3 ${DESTDIR}${PREFIX}${prefix}/bin/glite-lb-at3
	install -m 755 ${top_srcdir}/check_version.pl ${DESTDIR}${PREFIX}${prefix}/bin/glite-lb-check_version.pl
	(cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix}) 
	
clean:
	rm -fv at3
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

check:

.PHONY: default all compile stage install check clean distclean dist distcheck
