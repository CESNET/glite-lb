# Default values
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-ws-interface
version=0.0.0
PREFIX=/opt/glite

-include Makefile.inc

VPATH=${top_srcdir}/interface
STAGETO=interface

WSDL=LB.wsdl

all compile:

check: 
	@echo No unit test required for interface-only module.

stage:
	$(MAKE) install PREFIX=${stagedir}

dist: distsrc distbin

distsrc:
	mkdir -p ${top_srcdir}/${package}-${version}
	cd ${top_srcdir} && GLOBIGNORE="${package}-${version}" && cp -Rf * ${package}-${version}
	cd ${top_srcdir} && tar -czf ${distdir}/${package}-${version}_src.tar.gz --exclude-from=project/tar_exclude ${package}-${version}
	rm -rf ${top_srcdir}/${package}-${version}

distbin:
	$(MAKE) install PREFIX=${top_srcdir}/tmpbuilddir
	cd ${top_srcdir}/tmpbuilddir && tar -czf ${top_srcdir}/${distdir}/${package}-${version}_bin.tar.gz *
	rm -rf ${top_srcdir}/tmpbuilddir
	
install:
	-mkdir -p ${PREFIX}/${STAGETO}
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}
	install -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	cd ${top_srcdir}/interface && install -m 644 ${WSDL} ${PREFIX}/${STAGETO}

clean:
	rm -f *.h
	
