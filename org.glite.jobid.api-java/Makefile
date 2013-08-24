top_srcdir=.
stagedir=../stage
package=glite-jobid-api-java

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}
docdir?=${prefix}/share/doc/${package}-${version}

ANT_ARGS=-f ${top_srcdir}/build.xml -Dfile.reference.commons-codec.jar=${jakarta-commons-codec_jar} -Dsrc.dir=${top_srcdir}/src -Dbuild.dir=$(shell pwd)/build -Ddist.dir=$(shell pwd)/dist

SOURCES=\
	build.xml Makefile \
	nbproject/*.properties nbproject/*.xml \
	nbproject/private/*.properties nbproject/private/*.xml \
	project/pom.xml \
	src/org/glite/jobid/*.java

POM=JPP-${package}.pom

all compile doc: ${POM}
	${ant_prefix}/bin/ant ${ANT_ARGS}

stage:
	$(MAKE) install PREFIX=${stagedir}

install:
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/java ${DESTDIR}${PREFIX}${docdir}/api
	cp dist/glite-jobid-api-java.jar ${DESTDIR}${PREFIX}${prefix}/share/java
	cp -rp dist/javadoc/* ${DESTDIR}${PREFIX}${docdir}/api

check:

clean:
	${ant_prefix}/bin/ant ${ANT_ARGS} clean
	rm -fv ${POM}
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

${POM}: ${top_srcdir}/project/pom.xml
	sed -e "s:@DESCRIPTION@:$(shell cat ${top_srcdir}/project/package.description):" \
	    -e "s:@VERSION@:${version}:" \
	    $< > $@

.PHONY: default all compile stage install check clean distclean dist distcheck
