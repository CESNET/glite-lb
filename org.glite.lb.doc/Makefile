# Default values
top_srcdir=.
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-doc
version=0.0.0
PREFIX=/opt/glite

-include Makefile.inc
-include ../project/version.properties

version=${module.version}

VPATH = ${top_srcdir}/src
# example source code is part of the org.glite.lb.client
EXAMPLE_SOURCE_DIR=${top_srcdir}/examples
KPATH = TEXINPUTS=".:$(EXAMPLE_SOURCE_DIR):$(VPATH)//:"
KPATHBIB = BIBINPUTS=".:$(VPATH)//:"

LATEX = $(KPATH) latex
PDFLATEX = $(KPATH) pdflatex
BIBTEX = $(KPATHBIB) bibtex
DVIPS = $(KPATH) dvips
AT3=${stagedir}/sbin/glite-lb-at3
INSTALL=install

default all: generate doc

generate: events.tex status.tex ver.tex

doc: LBUG.pdf LBAG.pdf LBDG.pdf LBTP.pdf

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
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

ver.tex:
	@echo "\def\version{${version}}" > ver.tex

stage: doc
	$(MAKE) install PREFIX=${stagedir}

install:
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}/examples
	$(INSTALL) -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	( cd ${top_srcdir}/project && ${INSTALL} -m 644 ChangeLog package.description package.summary ${PREFIX}/share/doc/${package}-${version} )
	$(INSTALL) -m 644 ${top_srcdir}/src/README ${PREFIX}/share/doc/${package}-${version}
	$(INSTALL) -m 644 LBUG.pdf ${PREFIX}/share/doc/${package}-${version}
	$(INSTALL) -m 644 LBAG.pdf ${PREFIX}/share/doc/${package}-${version}
	$(INSTALL) -m 644 LBDG.pdf ${PREFIX}/share/doc/${package}-${version}
# install DG example sources
	for p in ${top_srcdir}/examples/*; do \
		if [ "$$p" == "${top_srcdir}/examples/CVS" ]; then continue; fi; \
		${INSTALL} -m 644 "$$p" "${PREFIX}/share/doc/${package}-${version}/examples"; \
	done

clean:
	rm -rvf LBUG* LBAG* LBDG* LBTP* events.tex status.tex ver.tex comment*
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/


# dependencies:

COMMON:=copyright.tex \
	definitions.tex \
	egee.cls \
	frontmatter.tex \
	lbjp.bib \
	versions.tex \
	ver.tex

LBUG.pdf: ${COMMON} \
	LBUG.tex \
	LBUG-Abstract.tex \
	LBUG-Introduction.tex components.tex \
	LBUG-Tools.tex logevent.tex notify.tex \
	LBUG-UseCases.tex log_usertag.tex change_acl.tex \
	LBUG-Troubleshooting.tex \
	LBUG-Appendix.tex events.tex status.tex 

LBAG.pdf: ${COMMON} \
	LBAG.tex \
	LBAG-Abstract.tex \
	LBAG-Introduction.tex components.tex \
	LBAG-Installation.tex \
	LBAG-Running.tex \
	LBAG-Troubleshooting.tex

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
	LBTP-Tests.tex \
	LBTP-PerfTests.tex \
	LBTP-InterTests.tex

.PHONY: all clean doc
