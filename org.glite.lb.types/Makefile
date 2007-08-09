stagedir=../stage

-include Makefile.inc

PM_DEST=${stagedir}/share/perl/gLite/LB
T_DEST=${stagedir}/share/lb/at3
SBIN=${stagedir}/sbin

PM=StructField.pm MultiStruct.pm
T=events.T status.T types.T
SCRIPTS=at3 check_version.pl

default: compile

compile: at3

at3: at3.in
	sed "s?%PREFIX%?${stagedir}?" at3.in >$@

stage: install

install: compile
	mkdir -p ${PM_DEST} ${T_DEST} ${SBIN}
	install -m 644 ${PM} ${PM_DEST}
	install -m 644 ${T} ${T_DEST}
	for p in ${SCRIPTS} ; do \
		install -m 755 "$$p" "${SBIN}/glite-lb-$$p"; \
	done
	
clean:
	rm -vf at3
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

check:
