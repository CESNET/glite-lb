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
KPATH = TEXINPUTS=".:$(VPATH)//:"
LATEX = $(KPATH) latex
PDFLATEX = $(KPATH) pdflatex
BIBTEX = $(KPATH) bibtex
DVIPS = $(KPATH) dvips

default: LBUG.pdf

%.dvi: %.tex
	$(LATEX) $<
#	$(BIBTEX) `basename $< .tex`
#	$(LATEX) $<
	$(LATEX) $<

%.ps: %.dvi
	$(DVIPS) -ta4 -o $@ $<

%.pdf: %.tex
	$(PDFLATEX) $<
#	$(BIBTEX) `basename $< .tex`
#	$(PDFLATEX) $<
	$(PDFLATEX) $<

clean:
	rm -rvf LBUG*

.PHONY: all clean purge ps pdf
