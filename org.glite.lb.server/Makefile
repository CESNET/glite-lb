# defaults
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-server
version=0.2.0
PREFIX=/opt/glite

glite_location=/opt/glite
globus_prefix=/opt/globus
nothrflavour=gcc32
thrflavour=gcc32pthr
expat_prefix=/opt/expat
cares_prefix=/opt/c-ares
gsoap_prefix=/opt/gsoap
classads_prefix=/opt/classads

-include Makefile.inc

default all: compile

GLITE_LB_SERVER_WITH_WS=yes

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
	VPATH=${top_srcdir}/interface:${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/examples:${top_srcdir}/project:${gsoap_prefix}
else
	VPATH=${top_srcdir}/interface:${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/examples:${top_srcdir}/project
endif
AT3=perl -I${top_srcdir}/project ${top_srcdir}/project/at3

TEST_LIBS:=-L${cppunit}/lib -lcppunit
TEST_INC:=-I${cppunit}/include

SUFFIXES = .T 

DEBUG:=-g -O0 -Wall

GRIDSITE_CFLAGS = `xml2-config --cflags`
GRIDSITE_LIBS = -lgridsite_globus `xml2-config --libs`

CFLAGS:= \
	${WS_CFLAGS} ${DEBUG} \
	-DVERSION=\"${version}\" \
	-I${stagedir}/include -I${top_srcdir}/src -I. \
	-I${top_srcdir}/interface \
	-I${expat_prefix}/include \
	-I${cares_prefix}/include \
	-I${gsoap_prefix}/include -I${gsoap_prefix}/ \
	-I${classads_prefix}/include \
	${COVERAGE_FLAGS} \
	-I${mysql_prefix}/include -I${mysql_prefix}/include/mysql \
	-I${globus_prefix}/include/${nothrflavour} \
	$(GRIDSITE_CFLAGS) \
	-D_GNU_SOURCE

TEST_LIBS:=-L${cppunit}/lib -lcppunit
TEST_INC:=-I${cppunit}/include

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/lib  ${LDFLAGS} 
SOLINK:=libtool --mode=link ${CC} -module ${LDFLAGS} -rpath ${stagedir}/lib
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install
LINKXX:=libtool --mode=link  ${CXX} -rpath ${stagedir}/lib ${LDFLAGS}
XSLTPROC:=xsltproc

GLOBUS_LIBS:= -L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lglobus_gssapi_gsi_${nothrflavour} \

#ifeq ($(shell ls ${gsoap_prefix}/bin/soapcpp2),${gsoap_prefix}/bin/soapcpp2)
#	gsoap_bin_prefix := ${gsoap_prefix}/bin
#else
#	gsoap_bin_prefix := ${gsoap_prefix}
#endif

gsoap_bin_prefix:=${shell if [ -x  ${gsoap_prefix}/bin/soapcpp2 ]; then echo ${gsoap_prefix}/bin; else echo ${gsoap_prefix}; fi }

ifneq (${mysql_prefix},/usr)
	ifeq ($(shell echo ${mysql_version} | cut -d. -f1,2),4.1)
		mysqlib := -L${mysql_prefix}/lib/mysql
	else
		mysqlib := -L${mysql_prefix}/lib
	endif
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

#EXT_LIBS:= \
#	${mysqlib} -lmysqlclient -lz\
#	${expatlib} -lexpat \
#	${GRIDSITE_LIBS} \
#	-lvomsc${vomsflavour} \
#	${GLOBUS_LIBS}

ifneq (${classads_prefix},/usr)
	classadslib := -L${classads_prefix}/lib -lclassad
endif

EXT_LIBS:=  \
	${mysqlib} -lmysqlclient -lz\
	${GRIDSITE_LIBS} \
	${classadslib} \
	-lvomsc${vomsflavour}

SRVBONES_LIB:= -L${stagedir}/lib -lglite_lb_server_bones
COMMON_LIBS:= -L${stagedir}/lib -lglite_lb_common_${nothrflavour} -lglite_security_gss_${nothrflavour}
PLUGIN_LIBS:= -L${stagedir}/lib -lglite_lb_common_${nothrflavour}\
	${expatlib} -lexpat\

PLUGIN_LOBJS:= lb_plugin.lo jobstat_supp.lo process_event.lo lbs_db_supp.lo

BKSERVER_BASE_OBJS:= \
	bkserverd.o il_lbproxy.o get_events.o index.o jobstat.o jobstat_supp.o process_event.o \
	seqcode.o write2rgma.o lbs_db.o lbs_db_supp.o lb_html.o lb_http.o lb_proto.o lb_xml_parse.o \
	lb_xml_parse_V21.o \
	lock.o openserver.o query.o userjobs.o db_store.o request.o store.o \
	stored_master.o srv_purge.o server_state.o dump.o lb_authz.o load.o \
	notification.o il_notification.o notif_match.o stats.o 

dotless_gsoap_ver:=${shell echo ${gsoap_version} | tr -d . }
GSOAP_LIB:=-L${stagedir}/lib -lglite_security_gsoap_plugin_${dotless_gsoap_ver}_${nothrflavour}

ifeq ($(GLITE_LB_SERVER_WITH_WS),yes)
	BKSERVER_OBJS:= \
		${BKSERVER_BASE_OBJS} \
		${GSOAP_FILES_PREFIX}C.o ${GSOAP_FILES_PREFIX}Server.o \
		ws_query.o ws_fault.o ws_typeref.o

	BKSERVER_LIBS= \
		${SRVBONES_LIB} \
		-lglite_lb_common_${nothrflavour} \
		${GSOAP_LIB} \
		${EXT_LIBS}
else
	BKSERVER_OBJS:= ${BKSERVER_BASE_OBJS}
		
	BKSERVER_LIBS= \
		${SRVBONES_LIB} \
		-lglite_lb_common_${nothrflavour} \
		-lglite_security_gss_${nothrflavour} \
		${EXT_LIBS}
endif

INDEX_OBJS:= index.o index_parse.o jobstat_supp.o lbs_db.o lbs_db_supp.o openserver.o \
	jobstat.o process_event.o query.o lock.o get_events.o write2rgma.o index_lex.o \
	lb_authz.o store.o bkindex.o stats.o

INDEX_LIBS:= ${SRVBONES_LIB} ${COMMON_LIBS} ${EXT_LIBS} 

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

HDRS=index.h lb_authz.h lbs_db.h store.h

LIB_OBJS_BK:= \
	il_lbproxy.o get_events.o index.o jobstat.o jobstat_supp.o process_event.o \
	seqcode.o write2rgma.o lbs_db.o lbs_db_supp.o lb_html.o lb_http.o lb_proto.o lb_xml_parse.o \
	lb_xml_parse_V21.o \
	lock.o openserver.o query.o userjobs.o db_store.o request.o store.o \
	stored_master.o srv_purge.o server_state.o dump.o lb_authz.o load.o \
	notification.o il_notification.o notif_match.o stats.o
STATIC_LIB_BK:=libglite_lb_bkserver.a

glite_lb_bkserverd: ${NSMAP} ${BKSERVER_OBJS}
	${LINKXX} -o $@ ${BKSERVER_OBJS} ${BKSERVER_LIBS}

glite_lb_bkindex: ${INDEX_OBJS}
	${LINK} -o $@ ${INDEX_OBJS} ${INDEX_LIBS}

glite_lb_plugin.la: ${PLUGIN_LOBJS}
	${SOLINK} -o $@ ${PLUGIN_LOBJS} ${PLUGIN_LIBS}

default all: compile

compile: glite_lb_bkserverd glite_lb_bkindex ${STATIC_LIB_BK} glite_lb_plugin.la

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
	${LINKXX} -o $@ test_xml.o lb_xml_parse.o ${COMMON_LIBS} ${TEST_LIBS} ${EXT_LIBS}

test.query: test_query_events
	./test_query_events

query_events_objs:=`echo ${BKSERVER_BASE_OBJS} |sed 's/lbs_db\.o//;s/bkserverd\.o//' |sed 's/lbs_db_supp\.o//;s/bkserverd\.o//'`

test_query_events: test_query_events.o
	${LINKXX} -o $@ test_query_events.o ${query_events_objs} \
	${TEST_LIBS} ${COMMON_LIBS} ${EXT_LIBS}

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

ws_query_ext: ws_query_ext.o ${WS_CLIENT_OBJS}
	${LINK} -o $@ ws_query_ext.o ${WS_CLIENT_OBJS} ${WS_CLIENT_LIBS}

ws_joblog: ws_joblog.o ${WS_CLIENT_OBJS}
	${LINK} -o $@ ws_joblog.o ${WS_CLIENT_OBJS} ${WS_CLIENT_LIBS}


${STATIC_LIB_BK}: ${LIB_OBJS_BK}
	ar crv $@ ${LIB_OBJS_BK}
	ranlib $@

jp_job_attrs.h: job-attrs.xsd jp_job_attrs.xsl 
	${XSLTPROC} ../src/jp_job_attrs.xsl $< >$@

doc:

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

dist: distsrc distbin

distsrc:
	mkdir -p ${top_srcdir}/${package}-${version}
	cd ${top_srcdir} && GLOBIGNORE="${package}-${version}" && cp -Rf * ${package}-${version}
	cd ${top_srcdir} && tar -czf ${distdir}/${package}-${version}_src.tar.gz --exclude-from=project/tar_exclude ${package}-${version}
	rm -rf ${top_srcdir}/${package}-${version}

distbin:
	$(MAKE) install PREFIX=`pwd`/tmpbuilddir${stagedir}
	save_dir=`pwd`; cd tmpbuilddir${stagedir} && tar -czf $$save_dir/${top_srcdir}/${distdir}/${package}-${version}_bin.tar.gz *; cd $$save_dir
	rm -rf tmpbuilddir
        
install:
	-mkdir -p ${PREFIX}/bin ${PREFIX}/etc ${PREFIX}/etc/init.d ${PREFIX}/interface
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}
	-mkdir -p ${PREFIX}/lib
	-mkdir -p ${PREFIX}/include/${globalprefix}/${lbprefix}
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	for p in bkserverd bkindex; do \
		${INSTALL} -m 755 "glite_lb_$$p" "${PREFIX}/bin/glite-lb-$$p"; \
	done

	for f in dbsetup.sql index.conf.template; do \
		${INSTALL} -m 644 ${top_srcdir}/config/"glite-lb-$$f" ${PREFIX}/etc; \
	done

	${INSTALL} -m 755 ${top_srcdir}/config/startup ${PREFIX}/etc/init.d/glite-lb-bkserverd
	${INSTALL} -m 644 ${top_srcdir}/interface/job-attrs.xsd ${PREFIX}/interface
	${INSTALL} -m 644 ${top_srcdir}/interface/job-record.xsd ${PREFIX}/interface
	${INSTALL} -m 644 jp_job_attrs.h ${PREFIX}/include/${globalprefix}/${lbprefix}

	if [ x${DOSTAGE} != xyes ]; then \
		${INSTALL} -m 755 ${stagedir}/bin/glite-lb-notif-interlogd ${PREFIX}/bin; \
	fi
	if [ x${DOSTAGE} = xyes ]; then \
		mkdir -p ${PREFIX}/include/${globalprefix}/${lbprefix} ; \
		(cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${lbprefix}) ; \
		install -m 644 ${STATIC_LIB_BK} ${PREFIX}/lib; \
		${INSTALL} -m 755 glite_lb_plugin.la ${PREFIX}/lib; \
	fi

clean:

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
	${CXX} -c ${CFLAGS} ${GLOBUSINC} ${TEST_INC} $<


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
