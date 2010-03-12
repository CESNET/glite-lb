top_srcdir=..
stagedir=.
version=0.0.0
PREFIX=/opt/glite

glite_location=/opt/glite
globus_prefix=/opt/globus
nothrflavour=gcc32
thrflavour=gcc32pthr
expat_prefix=/opt/expat
cares_prefix=/opt/c-ares
gsoap_prefix=/opt/gsoap
classads_prefix=/opt/classads
voms_prefix=/opt/voms

-include Makefile.inc
-include ../project/version.properties

version=${module.version}

CEXAMPLES:=ws_getversion ws_jobstat ws_joblog
#TODO ws_query_ex 

default all: ${CEXAMPLES}

VPATH=${top_srcdir}/examples
CC=gcc
CFLAGS=-I${gsoap_prefix}/include -I${gsoap_prefix}/ \
	-I${stagedir}/include -I${top_srcdir}/src -I. \
	-I${globus_prefix}/include/${nothrflavour} \
	${DEBUG}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/${libdir} ${LDFLAGS}
INSTALL:=libtool --mode=install install

GSOAP_FILES_PREFIX:= bk_ws_
dotless_gsoap_ver:=${shell echo ${gsoap_default_version} | tr -d . }
ifeq ($(shell test -f ${stagedir}/lib/libglite_security_gsoap_plugin_${dotless_gsoap_ver}_${nothrflavour}_c.so && echo ok),ok)
	langflavour:=_c
endif
ifeq ($(shell test -f ${stagedir}/${libdir}/libglite_security_gsoap_plugin_${dotless_gsoap_ver}_${nothrflavour}_c.so && echo ok),ok)
	langflavour:=_c
endif
GSOAP_LIB:=-L${stagedir}/${libdir} -L${stagedir}/lib -lglite_security_gsoap_plugin_${dotless_gsoap_ver}_${nothrflavour}${langflavour}

WS_CLIENT_OBJS:= $(GSOAP_FILES_PREFIX)C.o $(GSOAP_FILES_PREFIX)Client.o ws_fault.o
# ws_typeref.o
WS_CLIENT_LIBS:= ${GSOAP_LIB} 

SOAP_FILES:=${GSOAP_FILES_PREFIX}H.h ${GSOAP_FILES_PREFIX}C.c ${GSOAP_FILES_PREFIX}Server.c ${GSOAP_FILES_PREFIX}Client.c ${GSOAP_FILES_PREFIX}ServerLib.c ${GSOAP_FILES_PREFIX}ClientLib.c LoggingAndBookkeeping.nsmap

gsoap_bin_prefix:=${shell if [ -x  ${gsoap_prefix}/bin/soapcpp2 ]; then echo ${gsoap_prefix}/bin; else echo ${gsoap_prefix}; fi }

${SOAP_FILES}: %: LB.xh
	${gsoap_bin_prefix}/soapcpp2 -w -c -p ${GSOAP_FILES_PREFIX} LB.xh

${CEXAMPLES}: %: %.o ${WS_CLIENT_OBJS}
	${LINK} -o $@ $@.o ${WS_CLIENT_OBJS} ${WS_CLIENT_LIBS}

%.o %.lo: %.c soap_version.h ${SOAP_FILES}
	${COMPILE} -c $<

check:
	@echo "No test"

install:
	mkdir -p ${PREFIX}/examples/
	for f in ${CEXAMPLES}; do \
		${INSTALL} -m 755 $$f ${PREFIX}/examples/glite-lb-$$f; \
	done

clean:
	rm -rf .libs LB.xh *.xml ${GSOAP_FILES_PREFIX}* *.o *.lo *.nsmap soap_version.h ${CEXAMPLES}
	


LB.xh: ws_typemap.dat ${stagedir}/interface/LB.wsdl
	cp ${stagedir}/interface/LBTypes.wsdl .
	${gsoap_bin_prefix}/wsdl2h -c -t ${top_srcdir}/examples/ws_typemap.dat -o $@ ${stagedir}/interface/LB.wsdl
	rm -f LBTypes.wsdl

soap_version.h:
	${gsoap_bin_prefix}/soapcpp2 /dev/null 
	perl -ne '$$. == 2 && /.*([0-9]+)\.([0-9]+)\.([0-9]+)([a-z]?).*/ && printf "#define GSOAP_VERSION %d%02d%02d\n#define GSOAP_VERSION_LETTER '\''".($$4?$$4:"\\0")."'\''\n",$$1,$$2,$$3' soapH.h >$@
	-rm soapC.cpp soapH.h soapStub.h soapClient.cpp soapServer.cpp soapClientLib.cpp soapServerLib.cpp

