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
prefix=

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

SUFFIXES = .T

VPATH=${top_srcdir}/src
at3prefix=$(shell at3dir=${prefix}/share/glite-lb/at3; if test -d ${stagedir}$$at3dir; then echo ${stagedir}${prefix}; else echo ${prefix}; fi)
AT3=${at3prefix}/sbin/glite-lb-at3
XSLTPROC:=xsltproc --nonet
XMLLINT:=xmllint --nonet
TIDY=tidy -i -q --show-warnings no --tidy-mark no --wrap 0
docbookxls:=http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl

STAGETO=interface

WSDL=LB.wsdl LBTypes.wsdl

all compile: ${WSDL} ${doc} glue2-copy.xsd ws_interface_version.h

check: 
	@echo No unit test required for interface-only module.

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

glue2-copy.xsd:
	wget -O $@ http://schemas.ogf.org/glue/2008/05/spec_2.0_d42_r01 || cp ${top_srcdir}/src/glue2.xsd $@

install:
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${STAGETO}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}/examples
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix}
	install -m 644 ${top_srcdir}/LICENSE ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	( cd ${top_srcdir}/project && install -m 644 ChangeLog package.description package.summary ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version} )
	-test -f LB.html && install -m 644 LB.html ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
# install the generated stuff instead
#	cd ${top_srcdir}/interface && install -m 644 ${WSDL} ${DESTDIR}${PREFIX}${prefix}/${STAGETO}
	install -m 644 ${WSDL} ${DESTDIR}${PREFIX}${prefix}/${STAGETO}
	install -m 644 glue2-copy.xsd ${DESTDIR}${PREFIX}${prefix}/${STAGETO}/glue2.xsd
	install -m 644 ${top_srcdir}/src/lb4agu.wsdl ${DESTDIR}${PREFIX}${prefix}/${STAGETO}
	install -m 644 ws_interface_version.h ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix}

# JP has its own version anyway
#	if [ x${DOSTAGE} = xyes ]; then \
#		mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/glite-lb; \
#		install -m 644 ${top_srcdir}/src/puke-wsdl.xsl ${top_srcdir}/src/puke-ug.xsl ${DESTDIR}${PREFIX}${prefix}/share/glite-lb; \
#	fi

clean:
	rm -vf *.h *.html *.xml *.wsdl glue2-copy.xsd
	rm -rvf log.xml rpmbuild/ RPMS/ tgz/ debian/

%.xml: %.xml.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

${WSDL}: %.wsdl: %.xml puke-wsdl.xsl
	${XSLTPROC} ${top_srcdir}/src/puke-wsdl.xsl $< >$@
	-${TIDY} -xml -m $@
# XXX: obsolete:
#	-perl -i -n -e 'if (/^\s*$$/) { $$empty .= "\n"; } elsif (/^\s*<(xsd:)?(enumeration|element|input|output|fault)/) { print $$_; $$empty = "";} else { print "$$empty$$_"; $$empty=""; }; ' $@

LB.html: doc.xml LBTypes.xml LB.xml puke-ug.xsl
	${XSLTPROC} --novalid ${top_srcdir}/src/puke-ug.xsl $< >doc-html.xml
	-${TIDY} -xml -m doc-html.xml
#	${XMLLINT} --valid --noout doc-html.xml
	${XSLTPROC} --stringparam  chapter.autolabel 0 ${docbookxls} doc-html.xml >$@
	-${TIDY} -asxhtml -m $@

ws_interface_version.h: ${top_srcdi${top_srcdir}/project/version.properties
	echo "#define GLITE_LB_WS_INTERFACE_VERSION \"${version}\"" >$@
