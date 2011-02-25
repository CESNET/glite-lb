-include Makefile.inc

all compile:
	${ant_prefix}/bin/ant -Dfile.reference.commons-codec.jar=${jakarta-commons-codec_jar}

install:
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/java
	cp dist/jobid-api-java.jar ${DESTDIR}${PREFIX}${prefix}/share/java

check:

clean:
	${ant_prefix}/bin/ant clean
	rm -rvf log.xml project/glite-jobid-api-java.spec rpmbuild/ RPMS/ tgz/ debian/
