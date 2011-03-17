top_srcdir=.
stagedir=../stage

-include Makefile.inc

ANT_ARGS=-f ${top_srcdir}/build.xml -Dfile.reference.commons-codec.jar=${jakarta-commons-codec_jar} -Dsrc.dir=${top_srcdir}/src -Dbuild.dir=$(shell pwd)/build -Ddist.dir=$(shell pwd)/dist

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
	rm -rvf log.xml rpmbuild/ RPMS/ tgz/ debian/

.PHONY: all compile stage install check clean
