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

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

SUFFIXES = .T

VPATH=${top_srcdir}/src
AT3=${stagedir}${prefix}/bin/glite-lb-at3
XSLTPROC:=xsltproc --nonet
XMLLINT:=xmllint --nonet
TIDY=tidy -i -q --show-warnings no --tidy-mark no --wrap 0
docbookxls:=http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl

SOURCES=\
	examples/*.pl \
	src/*.xsd src/*.xml src/*.xml.T src/*.wsdl src/*.xsl src/README* \
	Makefile
SOURCES_EXEC=examples/*.pl

STAGETO=share/wsdl/glite-lb

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
	-test -f LB.html && mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix}
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
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf Makefile.inc *.spec debian/

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

.PHONY: default all compile check stage install clean distclean dist distcheck
