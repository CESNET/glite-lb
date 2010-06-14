-include Makefile.inc

# broken 
# SimpleLLTest.class ProducerTestLL.class ProducerTestIL.class

EXAMPLES := SSLClient.class SSLServer.class QueryDemo.class NotificationExample.class CreamTest.class

VPATH := examples

JAVAC:=${jdk_prefix}/bin/javac
JAVAH:=${jdk_prefix}/bin/javah
JAVA:=${jdk_prefix}/bin/java

all compile: genevents genws compile-java build-jar build-c

compile-java:
	${JAVAC} \
		-classpath ${commons-lang_jar}:${stagedir}/share/java/jobid-api-java.jar:${trustmanager_prefix}/share/java/glite-security-trustmanager.jar:${utiljava_prefix}/share/java/glite-security-util-java.jar:`ls ${axis_prefix}/lib/*.jar | tr '\012' :` \
		-d build \
		src/org/glite/lb/*.java \
		build/gen/*.java \
		build/axis/org/glite/wsdl/services/lb/*.java \
		build/axis/org/glite/wsdl/services/lb4agu/*.java \
		`find build/axis/org/ogf/schemas/glue -name *.java -print` \
		build/axis/org/glite/wsdl/types/lb/*.java \
		build/axis/org/glite/wsdl/types/lb/holders/*.java \
		build/axis/org/glite/wsdl/elements/lb/*.java \
		build/axis/holders/StringArrayHolder.java

build-jar:
	cd build && ${jdk_prefix}/bin/jar cf lb-client-java.jar holders org


build-c:
	${JAVAH} -classpath build -jni -d build org.glite.lb.ContextIL
	-mkdir -p build/c
	-ln -s ../../src_c/Makefile build/c
	cd build/c && $(MAKE) PREFIX=${PREFIX} JAVA_HOME=${jdk_prefix} topdir=../..

examples: ${EXAMPLES}

GEN:=${stagedir}/sbin/glite-lb-at3 project/genEventTypes.pl
AT3DIR:=${stagedir}/share/lb/at3
axis_classpath:=`ls ${axis_prefix}/lib/*.jar | tr '\012' :`

genevents:
	-mkdir -p build/gen
	${GEN} build/gen

genws: 
	${JAVA} -classpath ${axis_classpath} org.apache.axis.wsdl.WSDL2Java -o build/axis ${stagedir}/interface/LB.wsdl

${EXAMPLES}: %.class: %.java
	${jdk_prefix}/bin/javac -d build -cp build:build/classes/:${stagedir}/share/java/jobid-api-java.jar:`ls ${axis_prefix}/lib/*.jar | tr '\012' :` $<


check:
	@echo "No check"

install:
	mkdir -p ${PREFIX}/share/java
	cp build/lb-client-java.jar ${PREFIX}/share/java
	cd build/c && $(MAKE) install PREFIX=${PREFIX}

clean:
	rm -rf build
	cd src_c && $(MAKE) clean


