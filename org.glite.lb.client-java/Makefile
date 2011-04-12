top_srcdir=.
stagedir=../stage

-include Makefile.inc

# broken 
# SimpleLLTest.class ProducerTestLL.class ProducerTestIL.class

FULL_EXAMPLES := $(addprefix ${top_srcdir}/examples/,SSLClient.java SSLServer.java QueryDemo.java NotificationExample.java CreamTest.java)
SSL_EXAMPLES := $(addprefix ${top_srcdir}/examples/simple-ssl/,MyX509KeyManager.java MyX509TrustManager.java ExampleSSLSocketFactory.java LBClientSSL.java)
TM_EXAMPLES := $(addprefix ${top_srcdir}/examples/simple-trustmanager/,LBClientTM.java)

VPATH := ${top_srcdir}/examples ${top_srcdir}/examples/simple-ssl ${top_srcdir}/examples/simple-trustmanager

JAVAC:=${jdk_prefix}/bin/javac
JAVAH:=${jdk_prefix}/bin/javah
JAVA:=${jdk_prefix}/bin/java

at3prefix=$(shell at3dir=${prefix}/share/glite-lb/at3; if test -d ${stagedir}$$at3dir; then echo ${stagedir}${prefix}; else echo ${prefix}; fi)
GEN:=${at3prefix}/sbin/glite-lb-at3 ${top_srcdir}/project/genEventTypes.pl
AT3DIR:=${at3prefix}/share/glite-lb/at3
axis_classpath:=$(shell ls -1 ${axis_prefix}/lib/*.jar 2>/dev/null | tr '\012' :)
trustmanager_classpath:=$(shell ls -1 ${trustmanager_prefix}/share/java/glite-security-trustmanager.jar ${trustmanager_prefix}/share/java/trustmanager.jar ${trustmanager_prefix}/share/java/trustmanager-axis.jar ${stagedir}${prefix}/share/java/trustmanager.jar ${stagedir}${prefix}/share/java/trustmanager-axis.jar 2>/dev/null | tr '\012' :)

all compile: genevents genws compile-java build-jar examples build-c

compile-java:
	${JAVAC} \
		-classpath ${jakarta-commons-lang_jar}:${stagedir}${prefix}/share/java/jobid-api-java.jar:${trustmanager_classpath}:${utiljava_prefix}/share/java/glite-security-util-java.jar:${axis_classpath} \
		-d build \
		${top_srcdir}/src/org/glite/lb/*.java \
		build/gen/*.java \
		build/axis/org/glite/wsdl/services/lb/*.java \
		build/axis/org/glite/wsdl/services/lb4agu/*.java \
		`find build/axis/org/ogf/schemas/glue -name *.java -print` \
		build/axis/org/glite/wsdl/types/lb/*.java \
		build/axis/org/glite/wsdl/types/lb/holders/*.java \
		build/axis/org/glite/wsdl/elements/lb/*.java \
		build/axis/holders/StringArrayHolder.java

build-jar: lb-client-java.jar

lb-client-java.jar:
	cd build && ${jdk_prefix}/bin/jar cf lb-client-java.jar holders org && ${jdk_prefix}/bin/jar i lb-client-java.jar

build-c:
	${JAVAH} -classpath build -jni -d build org.glite.lb.ContextIL
	-mkdir -p build/c
	-ln -s ${top_srcdir}/src_c .
	-ln -s ../../src_c/Makefile build/c
	cd build/c && $(MAKE) PREFIX=${PREFIX} JAVA_HOME=${jdk_prefix} topdir=../..

examples: build/examples

build/examples: ${FULL_EXAMPLES} ${SSL_EXAMPLES} ${TM_EXAMPLES}
	-mkdir -p build/examples/src/simple-ssl build/examples/src/simple-trustmanager
	${jdk_prefix}/bin/javac -d build/examples -cp build:build/examples:${stagedir}${prefix}/share/java/jobid-api-java.jar:${axis_classpath} ${FULL_EXAMPLES}
	${jdk_prefix}/bin/javac -d build/examples -cp build:build/examples:${axis_classpath} ${SSL_EXAMPLES}
	${jdk_prefix}/bin/javac -d build/examples -cp build:${trustmanager_classpath}:${axis_classpath} ${TM_EXAMPLES}
	cp ${FULL_EXAMPLES} build/examples/src
	cp ${SSL_EXAMPLES} build/examples/src/simple-ssl
	cp ${TM_EXAMPLES} build/examples/src/simple-trustmanager
	cd build/examples && ${jdk_prefix}/bin/jar cfi lb-client-java-examples.jar src org $(addsuffix .class,$(basename $(notdir ${FULL_EXAMPLES}))) && ${jdk_prefix}/bin/jar i lb-client-java-examples.jar

genevents: build/gen

build/gen:
	-mkdir -p build/gen
	${GEN} build/gen

genws: build/axis

build/axis:
	${JAVA} -classpath ${axis_classpath} org.apache.axis.wsdl.WSDL2Java -o build/axis ${stagedir}${prefix}/share/wsdl/glite-lb/LB.wsdl

check:
	@echo "No check"

stage:
	$(MAKE) install PREFIX=${stagedir}

install:
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/java
	cp build/lb-client-java.jar ${DESTDIR}${PREFIX}${prefix}/share/java
	cp build/examples/lb-client-java-examples.jar ${DESTDIR}${PREFIX}${prefix}/share/java
	cd build/c && $(MAKE) install PREFIX=${PREFIX}

clean:
	rm -rf build
	rm -rvf log.xml rpmbuild/ RPMS/ tgz/ debian/

.PHONY: all compile compile-java build-jar build-c examples genevents genws check stage install clean
