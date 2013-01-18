top_srcdir=.
stagedir=../stage
package=glite-lb-client-java

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

# broken 
# SimpleLLTest.class ProducerTestLL.class ProducerTestIL.class

FULL_EXAMPLES := $(addprefix ${top_srcdir}/examples/,SSLClient.java SSLServer.java QueryDemo.java NotificationExample.java CreamTest.java)
SSL_EXAMPLES := $(addprefix ${top_srcdir}/examples/simple-ssl/,MyX509KeyManager.java MyX509TrustManager.java ExampleSSLSocketFactory.java LBClientSSL.java)
TM_EXAMPLES := $(addprefix ${top_srcdir}/examples/simple-trustmanager/,LBClientTM.java)

SOURCES=\
	examples/*.java examples/*/*.java \
	project/list-jars.sh project/genEventTypes.pl \
	src_c/*.c src_c/Makefile \
	src/org/glite/lb/*.java \
	Makefile
SOURCES_EXEC=project/list-jars.sh project/genEventTypes.pl

VPATH := ${top_srcdir}/src/org/glite/lb ${top_srcdir}/examples ${top_srcdir}/examples/simple-ssl ${top_srcdir}/examples/simple-trustmanager

JAVAC:=${jdk_prefix}/bin/javac
JAVAH:=${jdk_prefix}/bin/javah
JAVA:=${jdk_prefix}/bin/java

GEN:=${stagedir}${prefix}/bin/glite-lb-at3 ${top_srcdir}/project/genEventTypes.pl
AT3DIR:=${stagedir}${prefix}/share/glite-lb/at3
axis_classpath:=$(shell ${top_srcdir}/project/list-jars.sh ${axis_prefix}/lib ${axis_prefix}/share/java ${axis_prefix}/share/java/axis /usr/share/java /usr/lib/java /usr/lib/java/axis)
trustmanager_classpath:=$(shell ls -1 ${trustmanager_prefix}/share/java/glite-security-trustmanager.jar ${trustmanager_prefix}/share/java/trustmanager.jar ${trustmanager_prefix}/share/java/trustmanager-axis.jar ${stagedir}${prefix}/share/java/trustmanager.jar ${stagedir}${prefix}/share/java/trustmanager-axis.jar 2>/dev/null | tr '\012' :)

all compile: compile-java-axis compile-java-lb compile-java-gen compile-java-lb2 build-jar examples build-c

wtf:
	echo "axis_classpath: ${axis_classpath}"

compile-java-axis: genws
	${JAVAC} \
		-classpath ${jakarta-commons-lang_jar}:${stagedir}${prefix}/share/java/jobid-api-java.jar:${trustmanager_classpath}:${utiljava_prefix}/share/java/glite-security-util-java.jar:${axis_classpath} \
		-d build \
		build/axis/org/glite/wsdl/services/lb/*.java \
		build/axis/org/glite/wsdl/services/lb4agu/*.java \
		`find build/axis/org/ogf/schemas/glue -name *.java -print` \
		build/axis/org/glite/wsdl/types/lb/*.java \
		build/axis/org/glite/wsdl/types/lb/holders/*.java \
		build/axis/org/glite/wsdl/elements/lb/*.java \
		build/axis/holders/StringArrayHolder.java

compile-java-lb: Event.java Timeval.java Sources.java Level.java SeqCode.java Escape.java
	-mkdir -p build/classes
	${JAVAC} \
		-classpath ${jakarta-commons-lang_jar}:${stagedir}${prefix}/share/java/jobid-api-java.jar:${trustmanager_classpath}:${utiljava_prefix}/share/java/glite-security-util-java.jar:${axis_classpath}:build/classes \
		-d build $+

compile-java-gen: genevents
	${JAVAC} \
		-classpath ${jakarta-commons-lang_jar}:${stagedir}${prefix}/share/java/jobid-api-java.jar:${trustmanager_classpath}:${utiljava_prefix}/share/java/glite-security-util-java.jar:${axis_classpath}:build:build/classes \
		-d build \
		build/gen/*.java

compile-java-lb2: Context.java ContextDirect.java ContextIL.java ContextLL.java EventConvertor.java ILFileWriter.java ILProto.java Job.java LBCredentials.java LBException.java NotifParser.java Notification.java SSL.java SSLSend.java ServerConnection.java
	${JAVAC} \
		-classpath ${jakarta-commons-lang_jar}:${stagedir}${prefix}/share/java/jobid-api-java.jar:${trustmanager_classpath}:${utiljava_prefix}/share/java/glite-security-util-java.jar:${axis_classpath}:build:build/classes \
		-d build/classes $+

build-jar: lb-client-java.jar

lb-client-java.jar:
	cd build && ${jdk_prefix}/bin/jar cf lb-client-java.jar holders org
	cd build/classes && ${jdk_prefix}/bin/jar uf ../lb-client-java.jar org
	${jdk_prefix}/bin/jar i build/lb-client-java.jar

build-c:
	${JAVAH} -classpath build:build/classes -jni -d build org.glite.lb.ContextIL
	-mkdir -p build/c
	-ln -s ${top_srcdir}/src_c .
	-ln -s ../../src_c/Makefile build/c
	cd build/c && $(MAKE) PREFIX=${PREFIX} JAVA_HOME=${jdk_prefix} topdir=../..

examples: build/classes/examples

build/classes/examples: ${FULL_EXAMPLES} ${SSL_EXAMPLES} ${TM_EXAMPLES}
	-mkdir -p build/classes/examples/src/simple-ssl build/classes/examples/src/simple-trustmanager
	${jdk_prefix}/bin/javac -d build/classes/examples -cp build:build/classes:build/classes/examples:${stagedir}${prefix}/share/java/jobid-api-java.jar:${axis_classpath} ${FULL_EXAMPLES}
	${jdk_prefix}/bin/javac -d build/classes/examples -cp build:build/classes:build/classes/examples:${axis_classpath} ${SSL_EXAMPLES}
	${jdk_prefix}/bin/javac -d build/classes/examples -cp build:build/classes:${trustmanager_classpath}:${axis_classpath} ${TM_EXAMPLES}
	cp ${FULL_EXAMPLES} build/classes/examples/src
	cp ${SSL_EXAMPLES} build/classes/examples/src/simple-ssl
	cp ${TM_EXAMPLES} build/classes/examples/src/simple-trustmanager
	cd build/classes/examples && ${jdk_prefix}/bin/jar cfi lb-client-java-examples.jar src org $(addsuffix .class,$(basename $(notdir ${FULL_EXAMPLES}))) && ${jdk_prefix}/bin/jar i lb-client-java-examples.jar

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
	cp build/classes/examples/lb-client-java-examples.jar ${DESTDIR}${PREFIX}${prefix}/share/java
	cd build/c && $(MAKE) install PREFIX=${PREFIX}

clean:
	rm -rf build
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

.PHONY: default all compile compile-java-axis compile-java-lb compile-java-gen compile-java-lb2 build-jar build-c examples genevents genws check stage install clean distclean wtf dist distcheck
