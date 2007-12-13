# defaults
top_srcdir=.
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-server
version=0.2.0
PREFIX=/opt/glite

# gridsite requires openssl libraries, we want to include the ones from globus
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

default all: compile

# disable lb plugin in order to build also with 3.1 JP
BUILD_PLUGIN=yes

GLITE_LB_SERVER_WITH_WS=yes

ifdef LB_STANDALONE
	LB_STANDALONE_FLAGS:=-DLB_STANDALONE
#	GLITE_LB_SERVER_WITH_WS=no
endif

ifdef LB_PERF
	STATIC_LIB_BK:=libglite_lb_bkserver_perf.a
	LB_PERF_FLAGS:=-DLB_PERF
else
	STATIC_LIB_BK:=libglite_lb_bkserver.a
endif

# Use embrionic DAG registration implicitely
LB_DAG_FLAGS:=-DLB_DAG_EMBRIONIC

ifdef LB_BUF
	LB_BUF_FLAGS:=-DLB_BUF
endif

ifeq ($(GLITE_LB_SERVER_WITH_WS),yes)
	WS_CFLAGS=-DGLITE_LB_SERVER_WITH_WS 
	NSMAP=LoggingAndBookkeeping.nsmap
else
	WS_CFLAGS=
endif

GSOAP_FILES_PREFIX:= bk_ws_

YACC=bison -y
CC=gcc

ifeq ($(gsoap_version),2.7.0)
	VPATH=${top_srcdir}/interface:${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/doc:${top_srcdir}/examples:${top_srcdir}/project:${gsoap_prefix}
else
	VPATH=${top_srcdir}/interface:${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/doc:${top_srcdir}/examples:${top_srcdir}/project
endif
AT3=${stagedir}/sbin/glite-lb-at3

SUFFIXES = .T 

DEBUG:=-g -O0 -Wall

GRIDSITE_CFLAGS = `xml2-config --cflags`
GRIDSITE_LIBS = -lgridsite_globus `xml2-config --libs`

CFLAGS:= \
	${WS_CFLAGS} ${DEBUG} \
	-DVERSION=\"${version}\" \
	-I${gsoap_prefix}/include -I${gsoap_prefix}/ \
	-I${stagedir}/include -I${top_srcdir}/src -I. \
	-I${top_srcdir}/interface \
	-I${expat_prefix}/include \
	-I${cares_prefix}/include \
	-I${classads_prefix}/include \
	-I${voms_prefix}/include \
	${COVERAGE_FLAGS} \
	$(GRIDSITE_CFLAGS) \
	-D_GNU_SOURCE ${LB_STANDALONE_FLAGS} ${LB_PERF_FLAGS} ${LB_DAG_FLAGS} \
	${LB_BUF_FLAGS} \
	-I${globus_prefix}/include/${nothrflavour} #gridsite needs this

ifdef LB_PROF
	SRVBONES_LIB:= ${stagedir}/lib/libglite_lbu_server_bones.la
	LB_COMMON_LIB:=${stagedir}/lib/libglite_lb_common_${nothrflavour}.la
	LB_UTILS_DB_LIB:=${stagedir}/lib/libglite_lbu_db.la
	CFLAGS:=${CFLAGS} -pg -g
	LDFLAGS:=${LDFLAGS} -pg
else
	SRVBONES_LIB:= -L${stagedir}/lib -lglite_lbu_server_bones
	LB_COMMON_LIB:=-L${stagedir}/lib -lglite_lb_common_${nothrflavour}
	LB_UTILS_DB_LIB:=-L${stagedir}/lib -lglite_lbu_db
	LDFLAGS:= -L${stagedir}/lib
endif

ifdef LBS_DB_PROFILE
	CFLAGS:=${CFLAGS} -DLBS_DB_PROFILE
endif

TEST_LIBS:=-L${cppunit_prefix}/lib -lcppunit
TEST_INC:=-I${cppunit_prefix}/include

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/lib  ${LDFLAGS} 
SOLINK:=libtool --mode=link ${CC} -module ${LDFLAGS} -rpath ${stagedir}/lib
# XXX: not used? LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install
LINKXX:=libtool --mode=link  ${CXX} -rpath ${stagedir}/lib ${LDFLAGS}
XSLTPROC:=xsltproc --novalid

#ifeq ($(shell ls ${gsoap_prefix}/bin/soapcpp2),${gsoap_prefix}/bin/soapcpp2)
#	gsoap_bin_prefix := ${gsoap_prefix}/bin
#else
#	gsoap_bin_prefix := ${gsoap_prefix}
#endif

gsoap_bin_prefix:=${shell if [ -x  ${gsoap_prefix}/bin/soapcpp2 ]; then echo ${gsoap_prefix}/bin; else echo ${gsoap_prefix}; fi }

archlib:=lib
host_cpu:=${shell uname -m}
ifeq (${host_cpu},x86_64) 
    archlib:=lib64
endif

ifneq (${expat_prefix},/usr)
	expatlib := -L${expat_prefix}/lib
endif

vomsflavour := _${nothrflavour}
ifeq (${nothrflavour},gcc32) 
	vomsflavour :=
endif 
ifeq (${nothrflavour},gcc32dbg)
	vomsflavour :=
endif
VOMS_LIBS:=-L${voms_prefix}/lib -lvomsapi${vomsflavour}

#EXT_LIBS:= \
#	${expatlib} -lexpat \
#	${GRIDSITE_LIBS} \
#	-lvomsc${vomsflavour} \

ifneq (${classads_prefix},/usr)
	classadslib := -L${classads_prefix}/${archlib} -lclassad
endif

EXT_LIBS:=  \
	-lz -lcares\
	${GRIDSITE_LIBS} \
	${classadslib} \
	${VOMS_LIBS}

COMMON_LIBS:= -L${stagedir}/lib  -lglite_lb_common_${nothrflavour} -lglite_security_gss_${nothrflavour} -lglite_lbu_trio
PLUGIN_LIBS:= -L${stagedir}/lib -lglite_lb_common_${nothrflavour} -lglite_lbu_trio \
	${classadslib} -lstdc++ ${expatlib} -lexpat

PLUGIN_LOBJS:= lb_plugin.lo jobstat_supp.lo process_event.lo process_event_pbs.lo process_event_condor.lo

BKSERVER_BASE_OBJS:= \
	bkserverd.o il_lbproxy.o get_events.o index.o jobstat.o jobstat_supp.o process_event.o process_event_pbs.o process_event_condor.o \
	seqcode.o write2rgma.o lb_html.o lb_http.o lb_proto.o lb_xml_parse.o \
	lb_xml_parse_V21.o \
	lock.o openserver.o query.o userjobs.o db_store.o request.o store.o \
	stored_master.o srv_purge.o server_state.o dump.o lb_authz.o load.o \
	notification.o il_notification.o notif_match.o stats.o db_calls.o db_supp.o

dotless_gsoap_ver:=${shell echo ${gsoap_version} | tr -d . }
GSOAP_LIB:=-L${stagedir}/lib -lglite_security_gsoap_plugin_${dotless_gsoap_ver}_${nothrflavour}

ifeq ($(GLITE_LB_SERVER_WITH_WS),yes)
	BKSERVER_OBJS:= \
		${BKSERVER_BASE_OBJS} \
		${GSOAP_FILES_PREFIX}C.o ${GSOAP_FILES_PREFIX}Server.o \
		ws_query.o ws_fault.o ws_typeref.o

	BKSERVER_LIBS= \
		${SRVBONES_LIB} \
		${LB_COMMON_LIB} \
		${LB_UTILS_DB_LIB} \
		${GSOAP_LIB} \
		${EXT_LIBS}
else
	BKSERVER_OBJS:= ${BKSERVER_BASE_OBJS}
		
	BKSERVER_LIBS= \
		${SRVBONES_LIB} \
		${LB_COMMON_LIB} \
		${LB_UTILS_DB_LIB} \
		-lglite_security_gss_${nothrflavour} \
		${EXT_LIBS}
endif

INDEX_OBJS:= index.o index_parse.o jobstat_supp.o openserver.o \
	jobstat.o process_event.o process_event_pbs.o process_event_condor.o query.o lock.o get_events.o write2rgma.o index_lex.o \
	lb_authz.o store.o bkindex.o stats.o\
	request.o db_store.o srv_purge.o notif_match.o il_lbproxy.o dump.o lb_xml_parse.o il_notification.o lb_proto.o server_state.o lb_xml_parse_V21.o lb_html.o notification.o seqcode.o userjobs.o load.o db_calls.o db_supp.lo

INDEX_LIBS:= ${SRVBONES_LIB} ${COMMON_LIBS} ${EXT_LIBS} ${LB_UTILS_DB_LIB}

WS_CLIENT_OBJS:= $(GSOAP_FILES_PREFIX)C.o $(GSOAP_FILES_PREFIX)Client.o ws_fault.o ws_typeref.o
WS_CLIENT_LIBS:= ${GSOAP_LIB} -lglite_lb_common_${nothrflavour} \
	-lglite_lb_common_${nothrflavour}

# WS_CLIENT_LIBS:= -L${stagedir}/lib \
#                 -lglite_lb_client_${nothrflavour} \
#                 -lglite_lb_common_${nothrflavour} \
#                 -L${gsoap_prefix}/lib -lgsoap \
#                 -lglite_security_gsoap_plugin_${nothrflavour} \
#                 ${EXT_LIBS}
# 

HDRS=index.h lb_authz.h store.h

MAN_GZ:=glite-lb-mon-db.1.gz
MAN = $(MAN_GZ:.gz=)

LIB_OBJS_BK:= \
	il_lbproxy.o get_events.o index.o jobstat.o jobstat_supp.o process_event.o process_event_pbs.o process_event_condor.o \
	seqcode.o lb_html.o lb_http.o lb_proto.o lb_xml_parse.o \
	lb_xml_parse_V21.o \
	lock.o openserver.o query.o userjobs.o db_store.o request.o store.o \
	stored_master.o srv_purge.o server_state.o dump.o lb_authz.o load.o \
	notification.o il_notification.o notif_match.o stats.o write2rgma.o \
	db_calls.o db_supp.o

MONDB_OBJS:=mon-db.o ${LIB_OBJS_BK}
MONDB_LIBS:=${COMMON_LIBS} ${LB_UTILS_DB_LIB} ${EXT_LIBS} 

glite-lb-bkserverd: ${NSMAP} ${BKSERVER_OBJS}
	${LINKXX} -o $@ ${BKSERVER_OBJS} ${BKSERVER_LIBS}

glite-lb-bkindex: ${INDEX_OBJS}
	${LINKXX} -o $@ ${INDEX_OBJS} ${INDEX_LIBS}

glite_lb_plugin.la: ${PLUGIN_LOBJS}
	${SOLINK} -o $@ ${PLUGIN_LOBJS} ${PLUGIN_LIBS}

glite-lb-mon-db: ${MONDB_OBJS}
	${LINKXX} -o $@ ${MONDB_OBJS} ${MONDB_LIBS}

ifdef LB_STANDALONE
    PLUGIN_LIB=
else
ifndef BUILD_PLUGIN
    PLUGIN_LIB=
else
    PLUGIN_LIB=glite_lb_plugin.la
endif
endif

compile: generate glite-lb-bkserverd glite-lb-bkindex glite-lb-mon-db ${STATIC_LIB_BK} ${PLUGIN_LIB} ${MAN_GZ}

generate: store.c index.c jp_job_attrs.h

check: compile test.xml test.query
	-echo check.query not complete yet

test_coverage:
	-mkdir coverage
	cd coverage && $(MAKE) -f ../Makefile top_srcdir=../../ COVERAGE_FLAGS="-fprofile-arcs -ftest-coverage" check
	cd coverage && for i in `echo ${INDEX_OBJS} ${BKSERVER_OBJS} | tr ' ' '\012' | sort -u`; do gcov $$i ; done

lb_xml_parse.c: lb_xml_parse.c.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

${GSOAP_FILES_PREFIX}H.h ${GSOAP_FILES_PREFIX}C.c ${GSOAP_FILES_PREFIX}Server.c ${GSOAP_FILES_PREFIX}Client.c ${GSOAP_FILES_PREFIX}ServerLib.c ${GSOAP_FILES_PREFIX}ClientLib.c LoggingAndBookkeeping.nsmap: LB.xh
	${gsoap_bin_prefix}/soapcpp2 -w -c -p ${GSOAP_FILES_PREFIX} LB.xh

# try several times -- LB.wsdl downloads BaseFault.xsd from www.ibm.com which may be failing
# not used right now but may be useful one day
# LB.xh: ws_typemap.dat
#	${gsoap_prefix}/bin/wsdl2h -c -t ${top_srcdir}/src/ws_typemap.dat -o $@ ${stagedir}/interface/LB.wsdl || \
#	{ sleep 1 ; ${gsoap_prefix}/bin/wsdl2h -c -t ${top_srcdir}/src/ws_typemap.dat -o $@ ${stagedir}/interface/LB.wsdl; } || \
#	{ sleep 4 ; ${gsoap_prefix}/bin/wsdl2h -c -t ${top_srcdir}/src/ws_typemap.dat -o $@ ${stagedir}/interface/LB.wsdl; } || \
#	{ sleep 16 ; ${gsoap_prefix}/bin/wsdl2h -c -t ${top_srcdir}/src/ws_typemap.dat -o $@ ${stagedir}/interface/LB.wsdl; }

LB.xh: ws_typemap.dat ${stagedir}/interface/LB.wsdl
	cp ${stagedir}/interface/LBTypes.wsdl .
	${gsoap_bin_prefix}/wsdl2h -c -t ${top_srcdir}/src/ws_typemap.dat -o $@ ${stagedir}/interface/LB.wsdl
	rm -f LBTypes.wsdl

test.xml: test_xml
	./test_xml

test_xml: test_xml.cpp
	${CXX} -c ${CFLAGS} ${TEST_INC} $<
	${LINKXX} -o $@ test_xml.o lb_xml_parse.o ${LB_COMMON_LIB} ${TEST_LIBS}

test.query: test_query_events
	./test_query_events

#query_events_objs:=`echo ${BKSERVER_BASE_OBJS} |sed 's/bkserverd\.o//;s/db_store\.o//;s/stored_master\.o//;s/store\.o//;s/srv_purge\.o//;s/load\.o//;s/dump\.o//;s/lb_proto\.o//;s/lb_html\.o//;s/lb_http\.o//;'`

query_events_objs := test_query_events.o query.o get_events.o jobstat_supp.o index.o db_supp.o

test_query_events: ${query_events_objs}
	${LINKXX} -o $@ ${query_events_objs} \
	${TEST_LIBS} ${LB_COMMON_LIB}
#	${TEST_LIBS} ${COMMON_LIBS} ${EXT_LIBS}

test.soapconv: test_soap_conv
	./test_soap_conv

test_soap_conv: test_soap_conv.cpp ${WS_CLIENT_OBJS}
	${CXX} -c ${CFLAGS} ${TEST_INC} $<
	${LINKXX} -o $@ test_soap_conv.o ${WS_CLIENT_OBJS} ${WS_CLIENT_LIBS} ${TEST_LIBS}

examples: ws_getversion ws_jobstat ws_query_ex ws_joblog

ws_getversion: ws_getversion.o ${WS_CLIENT_OBJS}
	${LINK} -o $@ ws_getversion.o ${WS_CLIENT_OBJS} ${WS_CLIENT_LIBS}

ws_jobstat: ws_jobstat.o ${WS_CLIENT_OBJS}
	${LINK} -o $@ ws_jobstat.o ${WS_CLIENT_OBJS} ${WS_CLIENT_LIBS}

ws_query_ex: ws_query_ex.o ${WS_CLIENT_OBJS}
	${LINK} -o $@ ws_query_ex.o ${WS_CLIENT_OBJS} ${WS_CLIENT_LIBS}

ws_joblog: ws_joblog.o ${WS_CLIENT_OBJS}
	${LINK} -o $@ ws_joblog.o ${WS_CLIENT_OBJS} ${WS_CLIENT_LIBS}

${STATIC_LIB_BK}: ${LIB_OBJS_BK}
	ar crv $@ ${LIB_OBJS_BK}
	ranlib $@

jp_job_attrs.h: job-attrs.xsd jp_job_attrs.xsl 
	${XSLTPROC} ../src/jp_job_attrs.xsl $< >$@

doc: ${MAN_GZ}

${MAN_GZ}: ${MAN}
	cp $? .
	gzip -f $(notdir $?)

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

install:
	-mkdir -p ${PREFIX}/bin ${PREFIX}/etc ${PREFIX}/etc/init.d ${PREFIX}/interface
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}
	-mkdir -p ${PREFIX}/share/man/man1
	-mkdir -p ${PREFIX}/lib
	-mkdir -p ${PREFIX}/include/${globalprefix}/${lbprefix}
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${MAN_GZ} ${PREFIX}/share/man/man1
	for p in bkserverd bkindex mon-db; do \
		${INSTALL} -m 755 "glite-lb-$$p" "${PREFIX}/bin/glite-lb-$$p"; \
	done

	for f in dbsetup.sql dbsetup-migrate2transactions.sql index.conf.template; do \
		${INSTALL} -m 644 ${top_srcdir}/config/"glite-lb-$$f" ${PREFIX}/etc; \
	done

	${INSTALL} -m 755 ${top_srcdir}/config/startup ${PREFIX}/etc/init.d/glite-lb-bkserverd
	${INSTALL} -m 644 ${top_srcdir}/interface/job-attrs.xsd ${PREFIX}/interface
	${INSTALL} -m 644 ${top_srcdir}/interface/job-record.xsd ${PREFIX}/interface

	if [ ${stagedir} != ${PREFIX} ]; then ${INSTALL} -m 755 ${stagedir}/bin/glite-lb-notif-interlogd ${PREFIX}/bin; fi
	mkdir -p ${PREFIX}/include/${globalprefix}/${lbprefix}
	(cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${lbprefix})
	install -m 644 ${STATIC_LIB_BK} ${PREFIX}/lib
	if [ x${LB_STANDALONE} = x -a x${PLUGIN_LIB} != x ]; then \
		${INSTALL} -m 755 ${PLUGIN_LIB} ${PREFIX}/lib; \
		${INSTALL} -m 644 jp_job_attrs.h ${PREFIX}/include/${globalprefix}/${lbprefix} ; \
	fi
	${INSTALL} -m 644 ${top_srcdir}/interface/srv_perf.h ${PREFIX}/include/${globalprefix}/${lbprefix}

clean:
	rm -rvf *.c *.h *.ch *.xh *.xml *.nsmap *.o *.lo .libs glite-lb-* ${STATIC_LIB_BK} ${PLUGIN_LIB} test*
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

%.o: %.y
	${YACC} -d ${YFLAGS} $<
	mv y.tab.c $*.c
	mv y.tab.h $*.h
	${CC} -c ${CFLAGS} $*.c
	rm $*.c

%.cpp: %.cpp.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

%.o %.lo: %.c
	${COMPILE} -c $<

test_query_events.o: %.o: %.cpp
	${CXX} -c ${CFLAGS} ${TEST_INC} $<


%.lo: %.c
	${COMPILE} -o $@ -c $<

lb_plugin.lo: lb_plugin.c jp_job_attrs.h
	${COMPILE} -DPLUGIN_DEBUG -o $@ -c $<

soap_version.h:
	${gsoap_bin_prefix}/soapcpp2 /dev/null
	perl -ne '$$. == 2 && /.*([0-9])\.([0-9])\.([0-9]).*/ && printf "#define GSOAP_VERSION %d%02d%02d\n",$$1,$$2,$$3' soapH.h >$@
	-rm soapC.cpp soapH.h soapStub.h soapClient.cpp soapServer.cpp soapClientLib.cpp soapServerLib.cpp

ifeq ($(GLITE_LB_SERVER_WITH_WS),yes)
bkserverd.o ws_fault.o: soap_version.h
endif
