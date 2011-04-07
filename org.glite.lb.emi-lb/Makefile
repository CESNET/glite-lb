top_srcdir=..
stagedir=.
package=glite-lb-yaim
version=0.0.0
PREFIX=/opt/glite
prefix=

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

default all:

install:

stage:
	$(MAKE) install PREFIX=${stagedir}

check:

clean:
	rm -rvf log.xml rpmbuild/ RPMS/ tgz/ debian/

.PHONY: default all check install stage clean
