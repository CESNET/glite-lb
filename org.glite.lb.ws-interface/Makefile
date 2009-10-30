# Default values
top_srcdir=..
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
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
AT3=${stagedir}/sbin/glite-lb-at3
XSLTPROC:=xsltproc --nonet
XMLLINT:=xmllint --nonet
TIDY=tidy -i -q --show-warnings no --tidy-mark no --wrap 0
docbookxls:=http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl

STAGETO=interface

WSDL=LB.wsdl LBTypes.wsdl

all compile: ${WSDL} ${doc} glue2.xsd

check: 
	@echo No unit test required for interface-only module.

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

glue2.xsd:
	wget -O glue2.xsd http://schemas.ogf.org/glue/2008/05/spec_2.0_d42_r01 || cp ${top_srcdir}/src/glue2.xsd .

install:
	-mkdir -p ${PREFIX}/${STAGETO}
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}/examples
	install -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	( cd ${top_srcdir}/project && install -m 644 ChangeLog package.description package.summary ${PREFIX}/share/doc/${package}-${version} )
	-test -f LB.html && install -m 644 LB.html ${PREFIX}/share/doc/${package}-${version}
# install the generated stuff instead
#	cd ${top_srcdir}/interface && install -m 644 ${WSDL} ${PREFIX}/${STAGETO}
	install -m 644 ${WSDL} glue2.xsd ${PREFIX}/${STAGETO}
	install -m 644 ${top_srcdir}/src/lb4agu.wsdl ${PREFIX}/${STAGETO}

# JP has its own version anyway
#	if [ x${DOSTAGE} = xyes ]; then \
#		mkdir -p ${PREFIX}/share/lb; \
#		install -m 644 ${top_srcdir}/src/puke-wsdl.xsl ${top_srcdir}/src/puke-ug.xsl ${PREFIX}/share/lb; \
#	fi

clean:
	rm -rvf *.h *.html *.xml *.wsdl
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

%.xml: %.xml.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

${WSDL}: %.wsdl: %.xml puke-wsdl.xsl
	${XSLTPROC} ../src/puke-wsdl.xsl $< >$@
	-${TIDY} -xml -m $@
# XXX: obsolete:
#	-perl -i -n -e 'if (/^\s*$$/) { $$empty .= "\n"; } elsif (/^\s*<(xsd:)?(enumeration|element|input|output|fault)/) { print $$_; $$empty = "";} else { print "$$empty$$_"; $$empty=""; }; ' $@

LB.html: doc.xml LBTypes.xml LB.xml puke-ug.xsl
	${XSLTPROC} --novalid ../src/puke-ug.xsl $< >doc-html.xml
	-${TIDY} -xml -m doc-html.xml
	${XMLLINT} --valid --noout doc-html.xml
	${XSLTPROC} --stringparam  chapter.autolabel 0 ${docbookxls} doc-html.xml >$@
	-${TIDY} -asxhtml -m $@

