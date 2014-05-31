top_srcdir=.
stagedir=../stage
package=glite-lb-client-java

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}
docdir?=${prefix}/share/doc/${package}-${version}

# broken 
# SimpleLLTest.class ProducerTestLL.class ProducerTestIL.class

LOADER_SOURCES:=JNIDefaultLoader.java
FULL_EXAMPLES := $(addprefix ${top_srcdir}/examples/,SSLClient.java SSLServer.java QueryDemo.java NotificationExample.java CreamTest.java)
SSL_EXAMPLES := $(addprefix ${top_srcdir}/examples/simple-ssl/,MyX509KeyManager.java MyX509TrustManager.java ExampleSSLSocketFactory.java LBClientSSL.java)
TM_EXAMPLES := $(addprefix ${top_srcdir}/examples/simple-trustmanager/,LBClientTM.java)
CORE_SOURCES:=Context.java ContextIL.java Escape.java Event.java ILFileWriter.java ILProto.java JNIHelper.java LBException.java Level.java Timeval.java Sources.java SeqCode.java
AXIS_SOURCES:=ContextDirect.java ContextLL.java EventConvertor.java Job.java LBCredentials.java NotifParser.java Notification.java SSL.java SSLSend.java ServerConnection.java

CORE_CLASSES=$(CORE_SOURCES:.java=.class)
AXIS_CLASSES=$(AXIS_SOURCES:.java=.class) ServerConnection$$1.class ServerConnection$$QueryResultsFlag.class

SOURCES=\
	examples/*.java examples/*/*.java \
	project/list-jars.sh project/genEventTypes.pl project/client-java*.pom \
	src_c/*.c src_c/Makefile \
	src/org/glite/lb/*.java \
	Makefile
SOURCES_EXEC=project/list-jars.sh project/genEventTypes.pl

VPATH := ${top_srcdir}/src/org/glite/lb ${top_srcdir}/examples ${top_srcdir}/examples/simple-ssl ${top_srcdir}/examples/simple-trustmanager ${top_srcdir}/project

JAVAC:=${jdk_prefix}/bin/javac
JAVAH:=${jdk_prefix}/bin/javah
JAVA:=${jdk_prefix}/bin/java
JAVADOC:=${jdk_prefix}/bin/javadoc

GEN:=${stagedir}${prefix}/bin/glite-lb-at3 ${top_srcdir}/project/genEventTypes.pl
AT3DIR:=${stagedir}${prefix}/share/glite-lb/at3
jobid_classpath:=${stagedir}${prefix}/share/java/glite-jobid-api-java.jar
axis_classpath:=$(shell ${top_srcdir}/project/list-jars.sh ${axis_prefix}/lib ${axis_prefix}/share/java ${axis_prefix}/share/java/axis /usr/share/java /usr/lib/java /usr/lib/java/axis)
trustmanager_classpath:=$(shell ls -1 ${trustmanager_prefix}/share/java/glite-security-trustmanager.jar ${trustmanager_prefix}/share/java/trustmanager.jar ${trustmanager_prefix}/share/java/trustmanager-axis.jar ${stagedir}${prefix}/share/java/trustmanager.jar ${stagedir}${prefix}/share/java/trustmanager-axis.jar 2>/dev/null | tr '\012' :)

JARS:=glite-lb-client-java.jar
POM_SOURCES:=client-java.pom

DOC_SOURCES=$(CORE_SOURCES)
AXIS_TARGETS=build/classes/axis
ifneq (${axis_prefix},no)
# with axis generate the axis jar
JARS+=glite-lb-client-java-axis.jar
POM_SOURCES+=client-java-axis.pom
ifneq (${trustmanager_prefix},no)
# with trustmanager include the security code
DOC_SOURCES+=$(AXIS_SOURCES)
AXIS_TARGETS+=build/classes/security
JARS+=glite-lb-client-java-examples.jar
POM_SOURCES+=client-java-examples.pom
POM_AXIS_SOURCE=client-java-axis-security.pom
else
POM_AXIS_SOURCE=client-java-axis-nosecurity.pom
endif
endif

POMS=$(addprefix JPP-glite-lb-,$(POM_SOURCES))

all compile: $(JARS) doc build/c poms

poms: $(POMS)

wtf:
	echo "axis_classpath: ${axis_classpath}"

build/classes/axis: build/axis
	-mkdir -p build/classes/axis
	${JAVAC} \
		-classpath ${jakarta-commons-lang_jar}:${jobid_classpath}:${axis_classpath} \
		-d build/classes/axis \
		build/axis/org/glite/wsdl/services/lb/*.java \
		build/axis/org/glite/wsdl/services/lb4agu/*.java \
		`find build/axis/org/ogf/schemas/glue -name \*.java -print` \
		build/axis/org/glite/wsdl/types/lb/*.java \
		build/axis/org/glite/wsdl/types/lb/holders/*.java \
		build/axis/org/glite/wsdl/elements/lb/*.java \
		build/axis/holders/StringArrayHolder.java

build/classes/core: compile-java-lb compile-java-gen

compile-java-lb: $(CORE_SOURCES) build/JNILoader.java
	-mkdir -p build/classes/core
	${JAVAC} \
		-classpath ${jobid_classpath} \
		-d build/classes/core $+

compile-java-gen: build/gen
	-mkdir -p build/classes/core
	${JAVAC} \
		-classpath ${jobid_classpath}:build/classes/core \
		-d build/classes/core \
		build/gen/*.java

build/classes/security: build/classes/axis build/classes/core compile-java-security

compile-java-security: $(AXIS_SOURCES)
	-mkdir -p build/classes/security
	${JAVAC} \
		-classpath ${jakarta-commons-lang_jar}:${jobid_classpath}:${axis_classpath}:${trustmanager_classpath}:build/classes/core:build/classes/axis \
		-d build/classes/security $+

glite-lb-client-java.jar: build/classes/core
	cd build/classes/core && ${jdk_prefix}/bin/jar cf ../../$@ org/glite/lb
	cd build && ${jdk_prefix}/bin/jar i $@

glite-lb-client-java-axis.jar: $(AXIS_TARGETS)
	cd build/classes/axis && ${jdk_prefix}/bin/jar cf ../../$@ .
	if test -d build/classes/security; then \
		cd build/classes/security && ${jdk_prefix}/bin/jar uf ../../$@ $(addprefix org/glite/lb/,$(AXIS_CLASSES)); \
	fi
	cd build && ${jdk_prefix}/bin/jar i $@

build/c:
	${JAVAH} -classpath build/classes/core -jni -d build org.glite.lb.ContextIL
	-mkdir -p build/c
	-ln -s ${top_srcdir}/src_c .
	-ln -s ../../src_c/Makefile build/c
	cd build/c && $(MAKE) PREFIX=${PREFIX} JAVA_HOME=${jdk_prefix} topdir=../..

build/classes/examples: ${FULL_EXAMPLES} ${SSL_EXAMPLES} ${TM_EXAMPLES} build/classes/axis build/classes/core build/classes/security
	-mkdir -p build/classes/examples/src/simple-ssl build/classes/examples/src/simple-trustmanager
	${jdk_prefix}/bin/javac -d build/classes/examples -cp build/classes/axis:build/classes/core:build/classes/security:${jobid_classpath}:${axis_classpath} ${FULL_EXAMPLES}
	${jdk_prefix}/bin/javac -d build/classes/examples -cp build/classes/axis:build/classes/core:${axis_classpath} ${SSL_EXAMPLES}
	${jdk_prefix}/bin/javac -d build/classes/examples -cp build/classes/axis:build/classes/core:build/classes/security:${axis_classpath}:${trustmanager_classpath} ${TM_EXAMPLES}
	cp ${FULL_EXAMPLES} build/classes/examples/src
	cp ${SSL_EXAMPLES} build/classes/examples/src/simple-ssl
	cp ${TM_EXAMPLES} build/classes/examples/src/simple-trustmanager

glite-lb-client-java-examples.jar: build/classes/examples
	cd build/classes/examples && ${jdk_prefix}/bin/jar cf ../../$@ src org $(addsuffix .class,$(basename $(notdir ${FULL_EXAMPLES})))
	cd build && ${jdk_prefix}/bin/jar i $@

build/JNILoader.java: $(LOADER_SOURCES)
	-mkdir -p build
	cp -p $+ $@

build/gen:
	-mkdir -p build/gen
	${GEN} build/gen

build/axis:
	${JAVA} -classpath ${axis_classpath} org.apache.axis.wsdl.WSDL2Java --output build/axis --timeout 600 ${stagedir}${prefix}/share/wsdl/glite-lb/LB.wsdl

build/javadoc: build/gen
	${JAVADOC} -d build/javadoc -classpath build/classes/axis:build/classes/core:build/classes/security:${jobid_classpath}:${axis_classpath}:${trustmanager_classpath} $(addprefix ${top_srcdir}/src/org/glite/lb/,$(DOC_SOURCES)) build/gen/*.java

check:
	@echo "No check"

stage:
	$(MAKE) install PREFIX=${stagedir}

doc: build/javadoc

install:
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/java ${DESTDIR}${PREFIX}${docdir}/api
	for j in $(JARS); do \
		cp build/$$j ${DESTDIR}${PREFIX}${prefix}/share/java; \
	done
	cd build/c && $(MAKE) install PREFIX=${PREFIX}
	cp -rp build/javadoc/* ${DESTDIR}${PREFIX}${docdir}/api

clean:
	rm -rf build
	rm -vf *.pom
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

client-java-axis.pom: ${POM_AXIS_SOURCE}
	cp -p $< $@

JPP-glite-lb-client-java.pom: client-java.pom
	sed -e "s:@DESCRIPTION@:$(shell cat ${top_srcdir}/project/package.description | sed 's/&/\\\&amp;/g'):" \
	    -e "s:@VERSION@:${version}:" \
	    $< > $@; \

JPP-glite-lb-client-java-axis.pom: client-java-axis.pom
	sed -e "s:@DESCRIPTION@:$(shell cat ${top_srcdir}/project/package.description | sed 's/&/\\\&amp;/g'):" \
	    -e "s:@VERSION@:${version}:" \
	    $< > $@; \

JPP-glite-lb-client-java-examples.pom: client-java-examples.pom
	sed -e "s:@DESCRIPTION@:$(shell cat ${top_srcdir}/project/package.description | sed 's/&/\\\&amp;/g'):" \
	    -e "s:@VERSION@:${version}:" \
	    $< > $@; \

.PHONY: default all compile compile-java-lb compile-java-gen compile-java-security doc check stage install clean distclean wtf dist distcheck poms
