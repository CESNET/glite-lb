# defaults
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-server
version=0.0.0
PREFIX=/opt/glite

glite_location=/opt/glite
globus_prefix=/opt/globus
nothrflavour=gcc32
thrflavour=gcc32pthr
expat_prefix=/opt/expat
ares_prefix=/opt/ares
gsoap_prefix=/opt/gsoap

-include Makefile.inc
-include ../Makefile.inc

GSOAP_FILES_PREFIX:= bk_ws_

YACC=bison -y

VPATH=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/project
AT3=perl -I${top_srcdir}/project ${top_srcdir}/project/at3

TEST_LIBS:=-L${cppunit}/lib -lcppunit
TEST_INC:=-I${cppunit}/include

SUFFIXES = .T 

DEBUG:=-g -O0 -Wall
# not yet
#	-I${voms}/include \
#	-I${gacl}/include \
#	-I/usr/include/libxml2 \

# -DNO_VOMS -DNO_GACL to be removed when voms/gridsite are available
CFLAGS:= -DNO_VOMS -DNO_GACL \
	${DEBUG} -I${stagedir}/include -I${top_srcdir}/src -I. \
	-I${expat_prefix}/include \
	-I${ares_prefix}/include \
	-I${gsoap_prefix}/include \
	${COVERAGE_FLAGS} \
	-I${mysql_prefix}/include \
	-I${globus_prefix}/include/${nothrflavour}


TEST_LIBS:=-L${cppunit}/lib -lcppunit
TEST_INC:=-I${cppunit}/include

LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install
LINKXX:=libtool --mode=link  ${CXX} -rpath ${stagedir}/lib ${LDFLAGS}

# assisst & gss due to VOMS only
#	-lglobus_gss_assist_${nothrflavour} \
#	-lglobus_gssapi_gsi_${nothrflavour} \

GLOBUS_LIBS:= -L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lglobus_gssapi_gsi_${nothrflavour}


# XXX: our vomsc.la depends on badly installed expat

# not yet
#	-L${repository}/${gacl}/lib -lgacl \
#	${repository}/${voms}/lib/libvomsc.a \

EXT_LIBS:= -L${ares_prefix}/lib -lares \
	-L${mysql_prefix}/lib -lmysqlclient -lz\
	-L${expat_prefix}/lib -lexpat \
	${GLOBUS_LIBS}

GSOAP_LIBS:= -L${gsoap_prefix}/lib -lgsoap

SRVBONES_LIB:= -L${stagedir}/lib -lglite_lb_server_bones

COMMON_LIB:= -L${stagedir}/lib -lglite_lb_common_${nothrflavour}

SERVER_OBJS:= bkserverd.o get_events.o index.o jobstat.o jobstat_supp.o \
	write2rgma.o lbs_db.o lb_html.o lb_http.o lb_proto.o lb_xml_parse.o \
	lb_xml_parse_V21.o \
	lock.o openserver.o query.o userjobs.o db_store.o request.o store.o \
	stored_master.o srv_purge.o server_state.o dump.o lb_authz.o load.o \
	notification.o il_notification.o notif_match.o

SERVER_SOAP_OBJS:=${GSOAP_FILES_PREFIX}C.o ${GSOAP_FILES_PREFIX}Server.o \
	ws_plugin.o ws_query.o ws_fault.o ws_typeref.o


INDEX_OBJS:= index.o index_parse.o jobstat_supp.o lbs_db.o openserver.o \
	jobstat.o query.o lock.o get_events.o write2rgma.o index_lex.o \
	lb_authz.o store.o bkindex.o

glite_lb_bkserverd: ${SERVER_OBJS} ${SERVER_SOAP_OBJS}
	${LINK} -o $@ ${SERVER_OBJS} ${SERVER_SOAP_OBJS} ${COMMON_LIB} ${SRVBONES_LIB} ${GSOAP_LIBS} ${EXT_LIBS}

glite_lb_bkindex: ${INDEX_OBJS}
	${LINK} -o $@ ${INDEX_OBJS} ${COMMON_LIB} ${EXT_LIBS}

default all: compile

compile: glite_lb_bkserverd glite_lb_bkindex

check: compile test.xml
	-echo check.query not complete yet

test_coverage:
	-mkdir coverage
	cd coverage && $(MAKE) -f ../Makefile top_srcdir=../../ COVERAGE_FLAGS="-fprofile-arcs -ftest-coverage" check
	cd coverage && for i in `echo ${INDEX_OBJS} ${SERVER_OBJS} | tr ' ' '\012' | sort -u`; do gcov $$i ; done

test.xml: test_xml
	./test_xml

test_xml: test_xml.cpp
	${CXX} -c ${CFLAGS} ${TEST_INC} $<
	${LINKXX} -o $@ test_xml.o lb_xml_parse.o ${COMMON_LIB} ${TEST_LIBS}

lb_xml_parse.c: lb_xml_parse.c.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

${GSOAP_FILES_PREFIX}H.h ${GSOAP_FILES_PREFIX}C.c ${GSOAP_FILES_PREFIX}Server.c: LB.xh
	${gsoap_prefix}/bin/soapcpp2 -w -c -p ${GSOAP_FILES_PREFIX} LB.xh

LB.xh: LB.wsdl ws_typemap.dat
	cp ${top_srcdir}/project/LB.wsdl .
	${gsoap_prefix}/bin/wsdl2h -c -t ${top_srcdir}/src/ws_typemap.dat -o $@ LB.wsdl
	rm LB.wsdl


check.query: test_query_events
	./test_query_events

query_events_objs:=`echo ${SERVER_OBJS} | sed 's/lbs_db\.o//;s/bkserverd\.o//'`

test_query_events: test_query_events.o
	${LINKXX} -o $@ test_query_events.o ${query_events_objs} \
	${TEST_LIBS} ${COMMON_LIB} ${EXT_LIBS}

WS_CLIENT_OBJS:= test_ws_plugin.o $(GSOAP_FILES_PREFIX)C.o $(GSOAP_FILES_PREFIX)Client.o ws_fault.o ws_typeref.o
WS_CLIENT_LIBS:= ${GSOAP_LIBS} -L${stagedir}/lib -lglite_lb_client_${nothrflavour}


ws_jobstat: ws_jobstat.o ${WS_CLIENT_OBJS}
	${LINK} -o $@ ws_jobstat.o ${WS_CLIENT_OBJS} ${WS_CLIENT_LIBS} ${EXT_LIBS}

ws_query_ex: ws_query_ex.o ${WS_CLIENT_OBJS}
	${LINK} -o $@ ws_query_ex.o ${WS_CLIENT_OBJS} ${WS_CLIENT_LIBS} ${EXT_LIBS}

test_ws_plugin.o: ${top_srcdir}/src/ws_plugin.c
	${CC} ${CFLAGS} -DPLUGIN_TEST -c -o $@ ${top_srcdir}/src/ws_plugin.c

doc:

stage: compile
#	$(MAKE) install PREFIX=${top_srcdir}/${stagedir}
	$(MAKE) install PREFIX=${stagedir}
#	cd ${top_srcdir}/interface && install -m 644 ${STATIC_H} ${PREFIX}/${STAGETO}
	${INSTALL} -m 755 glite_lb_bkserverd glite_lb_bkindex ${stagedir}/bin

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
	-mkdir -p ${PREFIX}/bin ${PREFIX}/etc ${PREFIX}/etc/init.d
	for p in bkserverd bkindex; do \
		${INSTALL} -m 755 "glite_lb_$$p" "${PREFIX}/bin/glite-lb-$$p"; \
	done

	for f in dbsetup.sql index.conf.template; do \
		${INSTALL} -m 644 ${top_srcdir}/config/"glite-lb-$$f" ${PREFIX}/etc; \
	done

	${INSTALL} -m 755 ${top_srcdir}/config/startup ${PREFIX}/etc/init.d/glite-lb-bkserverd

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


test_query_events.o: %.o: %.cpp
	${CXX} -c ${CFLAGS} ${GLOBUSINC} ${TEST_INC} $<

