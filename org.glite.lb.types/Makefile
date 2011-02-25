stagedir=../stage

-include Makefile.inc


PM=StructField.pm MultiStruct.pm
T=events.T status.T types.T
SCRIPTS=at3 check_version.pl

default: compile

compile: at3

at3: at3.in
	#XXX: sed "s?%PREFIX%?${stagedir}?" at3.in >$@
	cp at3.in $@

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install: compile
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/perl/gLite/LB ${DESTDIR}${PREFIX}${prefix}/share/glite-lb/at3 ${DESTDIR}${PREFIX}${prefix}/sbin
	install -m 644 ${PM} ${DESTDIR}${PREFIX}${prefix}/share/perl/gLite/LB
	install -m 644 ${T} ${DESTDIR}${PREFIX}${prefix}/share/glite-lb/at3
	for p in ${SCRIPTS} ; do \
		install -m 755 "$$p" "${DESTDIR}${PREFIX}${prefix}/sbin/glite-lb-$$p"; \
	done
	
clean:
	rm -vf at3
	rm -rvf log.xml project/glite-lb-types.spec rpmbuild/ RPMS/ tgz/ debian/

check:
