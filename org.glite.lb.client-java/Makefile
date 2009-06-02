-include Makefile.inc

EXAMPLES := SimpleLLTest.class SSLClient.class SSLServer.class
# broken: ProducerTestIL.class ProducerTestLL.class

VPATH := examples

certmanagement_jar := $(shell ls ${jglobus_prefix}/lib/cog-certmanagement-*.jar | sort | tail -1)
jglobus_jar := $(shell ls ${jglobus_prefix}/lib/cog-jglobus-*.jar | sort | tail -1)

all compile:
	JAVA_HOME=${jdk_prefix} \
	${ant_prefix}/bin/ant -Dno.deps=yes -DstageDir=${stagedir} \
		-Dreference.jobid-api-java.jar=${stagedir}/share/java/jobid-api-java.jar \
		-Dreference.trustmanager.jar=${trustmanager_prefix}/share/java/glite-security-trustmanager.jar \
		-Daxis.classpath=`ls ${axis_prefix}/lib/*.jar | tr '\012' :` 
	cd src_c && make PREFIX=${PREFIX}

examples: ${EXAMPLES}


${EXAMPLES}: %.class: %.java
	javac -cp build/classes/:${stagedir}/share/java/jobid-api-java.jar $<


check:
	@echo "No check"

install:
	mkdir -p ${PREFIX}/share/java
	cp dist/lb-client-java.jar ${PREFIX}/share/java
	cd src_c && make install PREFIX=${PREFIX}

clean:
	echo TODO


