stagedir=../stage

-include Makefile.inc

PM_DEST=${stagedir}/share/perl/gLite/LB
T_DEST=${stagedir}/share/lb/at3
SBIN=${stagedir}/sbin

PM=StructField.pm MultiStruct.pm
T=events.T  status.T  types.T

default: compile

compile: at3

at3: at3.in
	sed "s?%PREFIX%?${stagedir}?" at3.in >$@

install: compile
	mkdir -p ${PM_DEST} ${T_DEST} ${SBIN}
	install -m 644 ${PM} ${PM_DEST}
	install -m 644 ${T} ${T_DEST}
	install -m 755 at3 ${SBIN}/glite-lb-at3
	
clean:
	rm -f at3

check:
