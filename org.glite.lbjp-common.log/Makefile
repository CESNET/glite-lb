top_srcdir=..
stagedir=../stage
globalprefix=glite
lbutilsprefix=lbu
package=glite-lbu-log
version=0.0.0
PREFIX=/opt/glite

log4c_prefix=/usr

-include Makefile.inc
-include ./project/version.properties

HEADER=log.h

default: compile

compile: 

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install: compile
	mkdir -p ${PREFIX}/include/${globalprefix}/${lbutilsprefix} 
	install -m 644 ${HEADER} ${PREFIX}/include/${globalprefix}/${lbutilsprefix} 
	mkdir -p ${PREFIX}/share/doc/${package}-${version}
	install -m 644 LICENSE ${PREFIX}/share/doc/${package}-${version}
	mkdir -p ${PREFIX}/etc
	install -m 644 log4crc.example ${PREFIX}/etc/log4crc
clean:
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

check:
