stagedir=../stage

-include Makefile.inc


PM=StructField.pm MultiStruct.pm
T=events.T status.T types.T
SCRIPTS=at3 check_version.pl

default: compile

compile: at3

at3: at3.in
	sed "s?%PREFIX%?${stagedir}?" at3.in >$@

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install: compile
	mkdir -p ${PREFIX}/share/perl/gLite/LB ${PREFIX}/share/lb/at3 ${PREFIX}/sbin
	install -m 644 ${PM} ${PREFIX}/share/perl/gLite/LB
	install -m 644 ${T} ${PREFIX}/share/lb/at3
	for p in ${SCRIPTS} ; do \
		install -m 755 "$$p" "${PREFIX}/sbin/glite-lb-$$p"; \
	done
	
clean:
	rm -vf at3
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

check:
