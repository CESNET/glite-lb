-include Makefile.inc

EXAMPLES := SimpleLLTest.class SSLClient.class SSLServer.class QueryDemo.class ProducerTestIL.class ProducerTestLL.class NotificationExample.class CreamTest.class

VPATH := examples

all compile:
	JAVA_HOME=${jdk_prefix} \
	${ant_prefix}/bin/ant -Dno.deps=yes -DstageDir=${stagedir} \
		-Dfile.reference.commons-lang.jar=${commons-lang_jar} \
		-Dreference.jobid-api-java.jar=${stagedir}/share/java/jobid-api-java.jar \
		-Dreference.trustmanager.jar=${trustmanager_prefix}/share/java/glite-security-trustmanager.jar \
		-Daxis.classpath=`ls ${axis_prefix}/lib/*.jar | tr '\012' :` 
	javah -classpath build/classes -jni org.glite.lb.ContextIL
	cd src_c && make PREFIX=${PREFIX} JAVA_HOME=${jdk_prefix} topdir=..

examples: ${EXAMPLES}


${EXAMPLES}: %.class: %.java
	${jdk_prefix}/bin/javac -cp build/classes/:${stagedir}/share/java/jobid-api-java.jar:`ls ${axis_prefix}/lib/*.jar | tr '\012' :` $<


check:
	@echo "No check"

install:
	mkdir -p ${PREFIX}/share/java
	cp dist/lb-client-java.jar ${PREFIX}/share/java
	cd src_c && make install PREFIX=${PREFIX}

clean:
	rm -rf dist
	find . -name \*.class -exec rm {} \;
	cd src_c && make clean


