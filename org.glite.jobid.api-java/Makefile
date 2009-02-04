-include Makefile.inc

all compile:
	${ant_prefix}/bin/ant -Dfile.reference.commons-codec.jar=${commons-codec_jar}

install:
	mkdir -p ${PREFIX}/share/java
	cp dist/jobid-api-java.jar ${PREFIX}/share/java

check:

clean:
	echo TODO
