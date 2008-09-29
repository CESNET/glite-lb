CLASSPATH:=${shell echo `find lib -type f | tr '\n' ':'`build}

all: wsdl build

wsdl: build/org/glite/wsdl/services/lb/stubs

build: build/org/glite/wsdl/services/lb/example

run:
	if test -z "${FILE}" -o -z "${PASS}"; then echo 'Specify FILE and PASS variables.'; exit 1; fi
	java -classpath $(CLASSPATH) org.glite.wsdl.services.lb.example.LBClientExample ${FILE} ${PASS} ${ENDPOINT}

clean:
	rm -rvf build/

build/org/glite/wsdl/services/lb/stubs:
	java -classpath $(CLASSPATH) org.apache.axis.wsdl.WSDL2Java -v \
	  --buildFile \
	  --package org.glite.wsdl.services.lb.stubs \
	  --output build \
	  http://egee.cesnet.cz/cms/export/sites/egee/en/WSDL/HEAD/LB.wsdl

build/org/glite/wsdl/services/lb/example:
	-mkdir build
	javac -classpath $(CLASSPATH) -d build src/*.java build/org/glite/wsdl/services/lb/{stubs,stubs/holders}/*.java

.PHONY: all wsdl run clean
