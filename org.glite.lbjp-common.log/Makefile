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
EXAMPLE=log4crc.example

default: compile

compile: 

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install: compile
	mkdir -p ${PREFIX}/include/${globalprefix}/${lbutilsprefix} 
	install -m 644 ${HEADER} ${PREFIX}/include/${globalprefix}/${lbutilsprefix} 
	mkdir -p ${PREFIX}/share/doc/${package}-${version}
	install -m 644 ${EXAMPLE} LICENSE ${PREFIX}/share/doc/${package}-${version}
clean:
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

check:
