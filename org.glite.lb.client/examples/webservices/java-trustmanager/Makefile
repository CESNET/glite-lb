CLASSPATH:=${shell echo `find lib -type f | tr '\n' ':'`build}
CLASSES=LBClientTM.class
VPATH=src:build

all: wsdl build

wsdl: org/glite/wsdl/services/lb/stubs

build: $(CLASSES)

run:
	if test -z "${FILE}" ; then echo 'Specify the FILE variable.'; exit 1; fi
	java -classpath $(CLASSPATH) org.glite.wsdl.services.lb.example.LBClientTM ${FILE} ${ENDPOINT}

clean:
	rm -rvf build/

org/glite/wsdl/services/lb/stubs:
	java -classpath $(CLASSPATH) org.apache.axis.wsdl.WSDL2Java -v \
	  --buildFile \
	  --package org.glite.wsdl.services.lb.stubs \
	  --output build \
	  http://egee.cesnet.cz/cms/export/sites/egee/en/WSDL/HEAD/LB.wsdl

LBClientTM.class: build/org/glite/wsdl/services/lb/stubs LBClientTM.java
	-mkdir build
	javac -classpath $(CLASSPATH) -d build src/*.java build/org/glite/wsdl/services/lb/{stubs,stubs/holders}/*.java

.PHONY: all wsdl run clean
