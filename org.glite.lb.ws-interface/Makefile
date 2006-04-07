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
-include ../project/version.properties

version=${module.version}

SUFFIXES = .T

VPATH=${top_srcdir}/src
AT3=perl -I${top_srcdir}/project ${top_srcdir}/project/at3
XSLTPROC:=xsltproc --nonet
TIDY=tidy
XMLLINT:=xmllint --nonet
docbookxls:=http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl

STAGETO=interface

WSDL=LB.wsdl LBTypes.wsdl

all compile: ${WSDL} LB.html

check: 
	@echo No unit test required for interface-only module.

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

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
	install -m 644 LB.html ${PREFIX}/share/doc/${package}-${version}
# install the generated stuff instead
#	cd ${top_srcdir}/interface && install -m 644 ${WSDL} ${PREFIX}/${STAGETO}
	install -m 644 ${WSDL} ${PREFIX}/${STAGETO}

# JP has its own version anyway
#	if [ x${DOSTAGE} = xyes ]; then \
#		mkdir -p ${PREFIX}/share/lb; \
#		install -m 644 ${top_srcdir}/src/puke-wsdl.xsl ${top_srcdir}/src/puke-ug.xsl ${PREFIX}/share/lb; \
#	fi

clean:
	rm -f *.h
	

%.xml: %.xml.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

${WSDL}: %.wsdl: %.xml puke-wsdl.xsl
	${XSLTPROC} ../src/puke-wsdl.xsl $< >$@
	-${TIDY} -wrap 10000 -xml -m -i -q $@
	-perl -i -n -e 'if (/^\s*$$/) { $$empty .= "\n"; } elsif (/^\s*<(xsd:)?(enumeration|element|input|output|fault)/) { print $$_; $$empty = "";} else { print "$$empty$$_"; $$empty=""; }; ' $@

LB.html: doc.xml LBTypes.xml LB.xml puke-ug.xsl
	${XSLTPROC} --novalid ../src/puke-ug.xsl $< >doc-html.xml
	${XMLLINT} --valid --noout doc-html.xml
	${XSLTPROC} --stringparam  chapter.autolabel 0 ${docbookxls} doc-html.xml >$@
