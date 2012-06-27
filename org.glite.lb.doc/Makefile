# Default values
top_srcdir=..
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-doc
version=0.0.0
PREFIX=/opt/glite
prefix=

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

VPATH = ${top_srcdir}/src
SOURCES=\
	examples/*.c examples/*.cpp examples/Makefile examples/README* examples/*.C \
	src/lbjp.bib src/*.cls src/*.sty src/*.tex src/*.tex.T src/README* src/images/*.pdf \
	GGUS19469-reply.txt LICENSE Makefile
# example source code is part of the org.glite.lb.client
EXAMPLE_SOURCE_DIR=${top_srcdir}/examples
KPATH = TEXINPUTS=".:$(EXAMPLE_SOURCE_DIR):$(VPATH)//:"
KPATHBIB = BIBINPUTS=".:$(VPATH)//:"

LATEX = $(KPATH) latex
PDFLATEX = $(KPATH) pdflatex
BIBTEX = $(KPATHBIB) bibtex
DVIPS = $(KPATH) dvips
AT3=${stagedir}${prefix}/sbin/glite-lb-at3
INSTALL=install

all: generate doc

generate: events.tex status.tex ver.tex

doc: LBUG.pdf LBAG.pdf LBDG.pdf LBTP.pdf LBTG.pdf

# %.dvi: %.tex
# 	$(LATEX) $<
# 	$(BIBTEX) `basename $< .tex`
# 	$(LATEX) $<
# 	$(LATEX) $<
#
# %.ps: %.dvi
# 	$(DVIPS) -ta4 -o $@ $<

%.pdf: %.tex
	$(PDFLATEX) $<
	$(BIBTEX) `basename $< .tex`
	$(PDFLATEX) $<
	$(PDFLATEX) $<

%.tex: %.tex.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

ver.tex:
	printf "\134def\134version{${version}}\n" > ver.tex

stage: doc
	$(MAKE) install PREFIX=${stagedir}

check:
	@echo "No test"

install:
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}/examples
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	( cd ${top_srcdir}/project && ${INSTALL} -m 644 ChangeLog package.description package.summary ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version} )
	${INSTALL} -m 644 ${top_srcdir}/src/README ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	${INSTALL} -m 644 LBUG.pdf ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	${INSTALL} -m 644 LBAG.pdf ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	${INSTALL} -m 644 LBDG.pdf ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	${INSTALL} -m 644 LBTG.pdf ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	${INSTALL} -m 644 LBTP.pdf ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
# install DG example sources
	for p in ${top_srcdir}/examples/*; do \
		if [ "$$p" = "${top_srcdir}/examples/CVS" ]; then continue; fi; \
		${INSTALL} -m 644 "$$p" "${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}/examples"; \
	done

clean:
	rm -rvf LBUG* LBAG* LBDG* LBTP* events.tex status.tex ver.tex comment*
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf Makefile.inc *.spec debian/


# dependencies:

COMMON:=copyright.tex \
	definitions.tex \
	emi.cls \
	frontmatter.tex \
	lbjp.bib \
	versions.tex \
	faq.tex \
	ver.tex

LBUG.pdf: ${COMMON} \
	LBUG.tex \
	LBUG-Abstract.tex \
	LBUG-Introduction.tex components.tex \
	LBUG-Tools.tex logevent.tex https_configuration.tex notify.tex log_usertag.tex change_acl.tex \
	LBUG-Troubleshooting.tex \
	LBUG-Appendix.tex events.tex status.tex faq.tex

LBAG.pdf: ${COMMON} \
	LBAG.tex \
	LBAG-Abstract.tex \
	LBAG-Introduction.tex components.tex \
	https_configuration.tex \
	LBAG-Installation.tex \
	LBAG-Running.tex \
	LBAG-Troubleshooting.tex faq.tex

LBDG.pdf: ${COMMON} \
	LBDG.tex \
	LBDG-Abstract.tex \
	LBDG-Introduction.tex \
	producer_api.tex \
	consumer_api.tex \
	notification_api.tex \
	web_services.tex

LBTP.pdf: ${COMMON} \
	LBTP.tex \
	LBTP-Abstract.tex \
	LBTP-Introduction.tex \
	LBTP-IntegrationTests.tex \
	LBTP-Tests.tex \
	LBTP-PerfTests.tex \
	LBTP-InterTests.tex \
	LBTP-Nagios.tex

LBTG.pdf: ${COMMON} \
	LBTG.tex \
	LBTG-Abstract.tex \
	LBAG-Troubleshooting.tex

.PHONY: default all generate doc stage check install clean distclean doc dist distcheck
