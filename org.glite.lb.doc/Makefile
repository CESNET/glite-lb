# Default values
top_srcdir=.
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-doc
version=0.0.0
PREFIX=/opt/glite

-include Makefile.inc

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

default all: generate LBUG.pdf LBAG.pdf LBDG.pdf

generate: events.tex status.tex

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

install:
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}
	$(INSTALL) -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	$(INSTALL) -m 644 ${top_srcdir}/src/README ${PREFIX}/share/doc/${package}-${version}
	$(INSTALL) -m 644 LBUG.pdf ${PREFIX}/share/doc/${package}-${version}
	$(INSTALL) -m 644 LBAG.pdf ${PREFIX}/share/doc/${package}-${version}
	$(INSTALL) -m 644 LBDG.pdf ${PREFIX}/share/doc/${package}-${version}
	-mkdir -p ${PREFIX}/share/examples/dg
# michal: install DG example sources
	for p in ${top_srcdir}/examples/*; do \
		${INSTALL} -m 644 "$$p" "${PREFIX}/share/examples/dg"; \
	done

clean:
	rm -rvf LBUG* LBAG* LBDG* events.tex status.tex
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/


# dependencies:

LBUG.pdf: LBUG.tex \
	LBUG-Introduction.tex components.tex \
	LBUG-Tools.tex logevent.tex notify.tex \
	LBUG-UseCases.tex log_usertag.tex change_acl.tex \
	LBUG-Appendix.tex events.tex status.tex 

LBAG.pdf: LBAG.tex \
	LBAG-Introduction.tex components.tex \
	LBAG-Installation.tex \
	LBAG-Running.tex

LBDG.pdf: LBDG.tex \
	LBDG-Introduction.tex \
	producer_api.tex \
	consumer_api.tex \
	notification_api.tex \
	web_services.tex

.PHONY: all clean

