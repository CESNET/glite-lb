top_srcdir=.
stagedir=../stage
package=glite-jobid-api-java

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

ANT_ARGS=-f ${top_srcdir}/build.xml -Dfile.reference.commons-codec.jar=${jakarta-commons-codec_jar} -Dsrc.dir=${top_srcdir}/src -Dbuild.dir=$(shell pwd)/build -Ddist.dir=$(shell pwd)/dist

SOURCES=\
	build.xml Makefile \
	nbproject/*.properties nbproject/*.xml \
	nbproject/private/*.properties nbproject/private/*.xml \
	src/org/glite/jobid/*.java

all compile:
	${ant_prefix}/bin/ant ${ANT_ARGS}

stage:
	$(MAKE) install PREFIX=${stagedir}

install:
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/java
	cp dist/jobid-api-java.jar ${DESTDIR}${PREFIX}${prefix}/share/java

check:

clean:
	${ant_prefix}/bin/ant ${ANT_ARGS} clean
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

.PHONY: default all compile stage install check clean distclean dist distcheck
