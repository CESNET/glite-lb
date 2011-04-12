# defaults
top_srcdir=..
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-server
version=0.2.0
PREFIX=/opt/glite
prefix=
sysconfdir=/opt/glite/etc

# gridsite requires openssl libraries, we want to include the ones from globus
globus_prefix=/opt/globus
expat_prefix=/opt/expat
cares_prefix=/opt/c-ares
gsoap_prefix=/opt/gsoap
classads_prefix=/opt/classads
voms_prefix=/opt/glite
lcas_prefix=/opt/glite
gridsite_prefix=/opt/glite

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

default all: compile

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

ifeq ($(GLITE_LB_SERVER_WITH_WS),yes)
	WS_CFLAGS=-DGLITE_LB_SERVER_WITH_WS 
	NSMAP=LoggingAndBookkeeping.nsmap
else
	WS_CFLAGS=
endif

GSOAP_FILES_PREFIX:= bk_ws_

YACC=bison -y
CC=gcc
CXX=g++

ifeq ($(gsoap_version),2.7.0)
	VPATH=${top_srcdir}/interface:${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/doc:${top_srcdir}/examples:${top_srcdir}/project:${gsoap_prefix}
else
	VPATH=${top_srcdir}/interface:${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/doc:${top_srcdir}/examples:${top_srcdir}/project
endif
at3prefix=$(shell at3dir=${prefix}/share/glite-lb/at3; if test -d ${stagedir}$$at3dir; then echo ${stagedir}${prefix}; else echo ${prefix}; fi)
AT3=${at3prefix}/sbin/glite-lb-at3

SUFFIXES = .T 

DEBUG:=-g -O0 -Wall

ifeq (${thrflavour},)
default_flavour=
vomsflavour:=
else
default_flavour=_${nothrflavour}
vomsflavour:=_${nothrflavour}
ifeq (${nothrflavour},gcc32) 
	vomsflavour :=
endif 
ifeq (${nothrflavour},gcc32dbg)
	vomsflavour :=
endif
endif

GRIDSITE_CFLAGS?=-I${gridsite_prefix}/include `xml2-config --cflags`
GRIDSITE_LIBS?=-L${gridsite_prefix}/${libdir} -L${gridsite_prefix}/lib -lgridsite_globus `xml2-config --libs`

LCAS_CFLAGS?=-I${lcas_prefix}/include -I${lcas_prefix}/include/glite/security
LCAS_LIBS?=-L${lcas_prefix}/${libdir} -llcas

ifneq (${classads_prefix},/usr)
	classads_libs := -L${classads_prefix}/${libdir} -L${classads_prefix}/lib
endif
CLASSADS_CFLAGS?=-I${classads_prefix}/include -I${classads_prefix}/include/classad
CLASSADS_LIBS?=${classads_libs} -lclassad

ifneq (${cares_prefix},/usr)
	cares_libs := -L${cares_prefix}/${libdir} -L${cares_prefix}/lib
endif
LIBCARES_CFLAGS?=-I${cares_prefix}/include
LIBCARES_LIBS?=${cares_libs} -lcares

GSOAP_CFLAGS?=-I${gsoap_prefix}/include -I${gsoap_prefix}/
EXPAT_CFLAGS?=-I${expat_prefix}/include
GLOBUS_GSSAPI_GSI_CFLAGS?=-I${globus_prefix}/include/${nothrflavour}

CPPUNIT_LIBS?=-L${cppunit_prefix}/${libdir} -lcppunit
CPPUNIT_CFLAGS?=-I${cppunit_prefix}/include

VOMS_CFLAGS?=-I${voms_prefix}/include -I${voms_prefix}/include/glite/security
VOMS_LIBS?=-L${voms_prefix}/${libdir} -lvomsapi${vomsflavour}

CFLAGS:= ${CFLAGS} \
	${GLOBUS_GSSAPI_GSI_CFLAGS} \
	${WS_CFLAGS} ${DEBUG} \
	-DVERSION=\"${version}\" \
	 ${GSOAP_CFLAGS} \
	-I${stagedir}${prefix}/include -I${top_srcdir}/src -I. \
	-I${top_srcdir}/interface \
	${EXPAT_CFLAGS} \
	${LIBCARES_CFLAGS} \
	${CLASSADS_CFLAGS} \
	${VOMS_CFLAGS} \
	${COVERAGE_FLAGS} \
	$(GRIDSITE_CFLAGS) ${LCAS_CFLAGS} ${GSOAP_CFLAGS} \
	-D_GNU_SOURCE ${LB_STANDALONE_FLAGS} ${LB_PERF_FLAGS} -DLB_EVENTS_BLOB

CXXFLAGS:=${CFLAGS}

ifdef LB_PROF
	SRVBONES_LIB:= ${stagedir}${prefix}/${libdir}/libglite_lbu_server_bones.la
	LB_COMMON_LIB:=${stagedir}${prefix}/${libdir}/libglite_lb_common${default_flavour}.la
	LB_UTILS_DB_LIB:=${stagedir}${prefix}/${libdir}/libglite_lbu_db.la
	CFLAGS:=${CFLAGS} -pg -g
	LDFLAGS:=${LDFLAGS} -pg
else
	SRVBONES_LIB:= -L${stagedir}${prefix}/${libdir} -lglite_lbu_server_bones
	LB_COMMON_LIB:=-L${stagedir}${prefix}/${libdir} -lglite_lb_common${default_flavour}
	LB_UTILS_DB_LIB:=-L${stagedir}${prefix}/${libdir} -lglite_lbu_db
	LDFLAGS:=${LDFLAGS} -L${stagedir}${prefix}/${libdir}
endif

# LB_MACHINE_LIB:=${stagedir}${prefix}/${libdir}/libglite_lb_statemachine.a
LB_MACHINE_LIB:=-L${stagedir}${prefix}/${libdir} -lglite_lb_statemachine

ifdef LBS_DB_PROFILE
	CFLAGS:=${CFLAGS} -DLBS_DB_PROFILE
endif

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
CXXCOMPILE:=libtool --mode=compile ${CXX} ${CXXFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}${prefix}/${libdir}  ${LDFLAGS} 
SOLINK:=libtool --mode=link ${CC} -module ${LDFLAGS} -rpath ${stagedir}${prefix}/${libdir}
INSTALL:=libtool --mode=install install
LINKXX:=libtool --mode=link  ${CXX} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS}
XSLTPROC:=xsltproc --novalid

gsoap_bin_prefix:=${shell if [ -x  ${gsoap_prefix}/bin/soapcpp2 ]; then echo ${gsoap_prefix}/bin; else echo ${gsoap_prefix}; fi }

EXT_LIBS:=  \
	-lz \
	${GRIDSITE_LIBS} \
	${CLASSADS_LIBS} \
	${LIBCARES_LIBS} \
	${LCAS_LIBS} \
	${VOMS_LIBS}

COMMON_LIBS:= -L${stagedir}${prefix}/${libdir} -lglite_lb_common${default_flavour} -lglite_security_gss${default_flavour} -lglite_lbu_trio -lglite_lbu_maildir -lglite_lbu_log

BKSERVER_BASE_OBJS:= \
	bkserverd.o il_lbproxy.o get_events.o index.o jobstat.o jobstat_supp.o \
	seqcode.o write2rgma.o lb_html.o lb_http.o lb_proto.o cond_dump.o lb_xml_parse.o \
	lb_xml_parse_V21.o lb_text.o \
	openserver.o query.o userjobs.o db_store.o request.o store.o \
	stored_master.o srv_purge.o server_state.o dump.o lb_authz.o load.o \
	notification.o il_notification.o notif_match.o stats.o db_calls.o db_supp.o lb_rss.o pretty_print_wrapper.o \
	policy_gram.o policy_lex.o authz_policy.o

gsoap_version ?= ${gsoap_default_version}

dotless_gsoap_ver:=${shell echo ${gsoap_version} | tr -d . }
ifeq ($(shell test -f ${stagedir}${prefix}/${libdir}/libglite_security_gsoap_plugin_${dotless_gsoap_ver}${default_flavour}_c.so && echo ok),ok)
	langflavour:=_c
endif
GSOAP_LIB:=-L${stagedir}${prefix}/${libdir} -lglite_security_gsoap_plugin_${dotless_gsoap_ver}${default_flavour}${langflavour}

ifeq ($(GLITE_LB_SERVER_WITH_WS),yes)
	BKSERVER_OBJS:= \
		${BKSERVER_BASE_OBJS} \
		${GSOAP_FILES_PREFIX}C.o ${GSOAP_FILES_PREFIX}Server.o \
		ws_query.o ws_lb4agu.o ws_fault.o ws_typeref.o

	BKSERVER_LIBS= \
		${SRVBONES_LIB} \
		${LB_COMMON_LIB} \
		${LB_MACHINE_LIB} \
		${LB_UTILS_DB_LIB} \
		${GSOAP_LIB} \
		${EXT_LIBS} \
		${classadslib} \
		-lglite_lbu_maildir \
		-lglite_lbu_log
else
	BKSERVER_OBJS:= ${BKSERVER_BASE_OBJS}
		
	BKSERVER_LIBS= \
		${SRVBONES_LIB} \
		${LB_COMMON_LIB} \
		${LB_MACHINE_LIB} \
		${LB_UTILS_DB_LIB} \
		-lglite_security_gss${default_flavour} \
		${EXT_LIBS} \
		${classadslib} \
		-lglite_lbu_maildir \
		-lglite_lbu_log
endif

INDEX_OBJS:= index.o index_parse.o jobstat_supp.o openserver.o \
	jobstat.o query.o get_events.o write2rgma.o index_lex.o \
	lb_authz.o store.o bkindex.o stats.o\
	request.o db_store.o srv_purge.o notif_match.o il_lbproxy.o dump.o lb_xml_parse.o il_notification.o lb_proto.o lb_text.o server_state.o lb_xml_parse_V21.o lb_html.o cond_dump.o notification.o seqcode.o userjobs.o load.o db_calls.o db_supp.o lb_rss.o pretty_print_wrapper.o authz_policy.o

INDEX_LIBS:= ${SRVBONES_LIB} ${COMMON_LIBS} ${LB_MACHINE_LIB} ${EXT_LIBS} ${LB_UTILS_DB_LIB}

WS_CLIENT_OBJS:= $(GSOAP_FILES_PREFIX)C.o $(GSOAP_FILES_PREFIX)Client.o ws_fault.o ws_typeref.o
WS_CLIENT_LIBS:= ${GSOAP_LIB} -lglite_lb_common${default_flavour} \
	-lglite_lb_common${default_flavour}

HDRS=index.h lb_authz.h store.h

MAN_GZ:=glite-lb-bkindex.8.gz glite-lb-bkserverd.8.gz glite-lb-mon-db.1.gz
MAN = $(MAN_GZ:.gz=)

LIB_OBJS_BK:= \
	il_lbproxy.o get_events.o index.o jobstat.o jobstat_supp.o \
	seqcode.o lb_html.o lb_http.o lb_proto.o cond_dump.o lb_xml_parse.o \
	lb_xml_parse_V21.o lb_text.o \
	openserver.o query.o userjobs.o db_store.o request.o store.o \
	stored_master.o srv_purge.o server_state.o dump.o lb_authz.o load.o \
	notification.o il_notification.o notif_match.o stats.o write2rgma.o \
	db_calls.o db_supp.o lb_rss.o pretty_print_wrapper.o authz_policy.o

MONDB_OBJS:=mon-db.o ${LIB_OBJS_BK}
MONDB_LIBS:=${COMMON_LIBS} ${LB_MACHINE_LIB} ${LB_UTILS_DB_LIB} ${EXT_LIBS}

LCAS_PLUGIN_OBJS:=lcas_lb.o
LCAS_PLUGIN_LOBJS:=${LCAS_PLUGIN_OBJS:.o=.lo}
LCAS_PLUGIN_LIB:=liblcas_lb.la

EXAMPLES=

TESTS=test_coverage test_xml test_query_events test_soap_conv

glite-lb-bkserverd: ${NSMAP} ${BKSERVER_OBJS}
	${LINKXX} -o $@ ${BKSERVER_OBJS} ${BKSERVER_LIBS} -Wl,--export-dynamic

glite-lb-bkindex: ${INDEX_OBJS}
	${LINKXX} -o $@ ${INDEX_OBJS} ${INDEX_LIBS}

glite-lb-mon-db: ${MONDB_OBJS}
	${LINKXX} -o $@ ${MONDB_OBJS} ${MONDB_LIBS}

compile: generate startup glite-lb-bkserverd glite-lb-bkindex glite-lb-mon-db ${STATIC_LIB_BK} ${LCAS_PLUGIN_LIB} ${MAN_GZ}

generate: store.c index.c 

startup: ${top_srcdir}/config/startup
	glite_var="${localstatedir}/glite"; \
	if echo "${localstatedir}" | grep 'glite'>/dev/null; then \
		glite_var="${localstatedir}"; \
	fi; \
	sed -e 's:@glite_prefix@:${sysroot}${prefix}:' -e 's:@glite_etc@:${sysconfdir}:' -e "s:@glite_var@:$$glite_var:" $< > $@
	chmod +x $@

check: compile test.xml test.query
	-echo check.query not complete yet

test_coverage:
	-mkdir coverage
	cd coverage && $(MAKE) -f ../Makefile top_srcdir=../../ COVERAGE_FLAGS="-fprofile-arcs -ftest-coverage" check
	cd coverage && for i in `echo ${INDEX_OBJS} ${BKSERVER_OBJS} | tr ' ' '\012' | sort -u`; do gcov $$i ; done

lb_xml_parse.c: lb_xml_parse.c.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

${GSOAP_FILES_PREFIX}H.h ${GSOAP_FILES_PREFIX}C.c ${GSOAP_FILES_PREFIX}Server.c ${GSOAP_FILES_PREFIX}Client.c ${GSOAP_FILES_PREFIX}ServerLib.c ${GSOAP_FILES_PREFIX}ClientLib.c LoggingAndBookkeeping.nsmap: LB.xh
	${gsoap_bin_prefix}/soapcpp2 -w -c -p ${GSOAP_FILES_PREFIX} LB.xh

# try several times -- LB.wsdl downloads BaseFault.xsd from www.ibm.com which may be failing
# not used right now but may be useful one day
# LB.xh: ws_typemap.dat
#	${gsoap_prefix}/bin/wsdl2h -c -t ${top_srcdir}/src/ws_typemap.dat -o $@ ${stagedir}${prefix}/interface/LB.wsdl || \
#	{ sleep 1 ; ${gsoap_prefix}/bin/wsdl2h -c -t ${top_srcdir}/src/ws_typemap.dat -o $@ ${stagedir}${prefix}/interface/LB.wsdl; } || \
#	{ sleep 4 ; ${gsoap_prefix}/bin/wsdl2h -c -t ${top_srcdir}/src/ws_typemap.dat -o $@ ${stagedir}${prefix}/interface/LB.wsdl; } || \
#	{ sleep 16 ; ${gsoap_prefix}/bin/wsdl2h -c -t ${top_srcdir}/src/ws_typemap.dat -o $@ ${stagedir}${prefix}/interface/LB.wsdl; }

LB.xh: ws_typemap.dat ${stagedir}${prefix}/share/wsdl/glite-lb/LB.wsdl
	${gsoap_bin_prefix}/wsdl2h -c -t ${top_srcdir}/src/ws_typemap.dat -I${stagedir}${prefix}/share/wsdl/glite-lb -o $@ ${stagedir}${prefix}/share/wsdl/glite-lb/LB.wsdl

test.xml: test_xml
	LD_LIBRARY_PATH=${cares_prefix}/${libdir}:${classads_prefix}/${libdir}:${LD_LIBRARY_PATH} ./test_xml

test_xml: test_xml.cpp
	${CXX} -c ${CFLAGS} ${CPPUNIT_CFLAGS} $<
	${LINKXX} -o $@ test_xml.o lb_xml_parse.o ${LB_COMMON_LIB} ${CPPUNIT_LIBS}

test.query: test_query_events
	# XXX coredumps ./test_query_events

#query_events_objs:=`echo ${BKSERVER_BASE_OBJS} |sed 's/bkserverd\.o//;s/db_store\.o//;s/stored_master\.o//;s/store\.o//;s/srv_purge\.o//;s/load\.o//;s/dump\.o//;s/lb_proto\.o//;s/lb_html\.o//;s/lb_http\.o//;'`

query_events_objs := test_query_events.o query.o get_events.o jobstat_supp.o index.o db_supp.o

test_query_events: ${query_events_objs}
	${LINKXX} -o $@ ${query_events_objs} \
	${CPPUNIT_LIBS} ${LB_COMMON_LIB} ${LB_MACHINE_LIB} ${classadslib} -lglite_lbu_log

test.soapconv: test_soap_conv
	LD_LIBRARY_PATH=${cares_prefix}/${libdir}:${classads_prefix}/${libdir}:${LD_LIBRARY_PATH} ./test_soap_conv

test_soap_conv: test_soap_conv.cpp ${WS_CLIENT_OBJS}
	${CXX} -c ${CFLAGS} ${CPPUNIT_CFLAGS} $<
	${LINKXX} -o $@ test_soap_conv.o ${WS_CLIENT_OBJS} ${WS_CLIENT_LIBS} ${CPPUNIT_LIBS}

examples: ${EXAMPLES}

${STATIC_LIB_BK}: ${LIB_OBJS_BK}
	ar crv $@ ${LIB_OBJS_BK}
	ranlib $@

${LCAS_PLUGIN_LIB}: ${LCAS_PLUGIN_LOBJS}
	${LINK} -o $@ $^

doc: ${MAN_GZ}

man: ${MAN_GZ}

${MAN_GZ}: ${MAN}
	cp $? .
	gzip -f $(notdir $?)

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

install:
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/bin ${DESTDIR}${PREFIX}${sysconfdir}/glite-lb ${DESTDIR}${PREFIX}${sysconfdir}/init.d
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/man/man1
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix}
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/man/man8
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	( cd ${top_srcdir}/project && ${INSTALL} -m 644 ChangeLog package.description package.summary ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version} )
	${INSTALL} -m 644 ${MAN_GZ} ${DESTDIR}${PREFIX}${prefix}/share/man/man1
	for p in bkserverd bkindex mon-db; do \
		${INSTALL} -m 755 "glite-lb-$$p" "${DESTDIR}${PREFIX}${prefix}/bin/glite-lb-$$p"; \
	done
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}/modules
	${INSTALL} -m 755 ${LCAS_PLUGIN_LIB} ${DESTDIR}${PREFIX}${prefix}/${libdir}/modules
	ln -sf liblcas_lb.so ${DESTDIR}${PREFIX}${prefix}/${libdir}/modules/lcas_lb.mod

	for f in dbsetup.sql index.conf.template; do \
		${INSTALL} -m 644 ${top_srcdir}/config/"glite-lb-$$f" ${DESTDIR}${PREFIX}${sysconfdir}/glite-lb; \
		ln -sf "glite-lb/glite-lb-$$f" "${DESTDIR}${PREFIX}${sysconfdir}/glite-lb-$$f"; \
	done
	${INSTALL} -m 755 ${top_srcdir}/config/glite-lb-migrate_db2version20 ${DESTDIR}${PREFIX}${sysconfdir}/glite-lb

	${INSTALL} -m 755 startup ${DESTDIR}${PREFIX}${sysconfdir}/init.d/glite-lb-bkserverd

	mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix}
	(cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix})
	install -m 644 ${STATIC_LIB_BK} ${DESTDIR}${PREFIX}${prefix}/${libdir}
	${INSTALL} -m 644 ${top_srcdir}/interface/srv_perf.h ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix}
	${INSTALL} -m 644 ${MAN_GZ} ${DESTDIR}${PREFIX}${prefix}/share/man/man8
ifdef LB_PERF
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/sbin
	${INSTALL} -m 755 ${top_srcdir}/src/perftest_proxy.sh ${DESTDIR}${PREFIX}${prefix}/sbin
endif

clean:
	rm -rvf *.c *.h *.ch *.xh *.xml *.nsmap *.o *.lo .libs glite-lb-* ${STATIC_LIB_BK} ${LCAS_PLUGIN_LIB} ${TESTS} ${MAN_GZ} ${EXAMPLES} startup
	rm -rvf log.xml rpmbuild/ RPMS/ tgz/ debian/

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

%.o: %.y
	${YACC} -d ${YFLAGS} $<
	mv y.tab.c $*.c
	mv y.tab.h $*.h
	${CC} -c ${CFLAGS} $*.c
	rm $*.c

%.cpp: %.cpp.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

%.o %.lo: %.c
	${COMPILE} -c $<

%.o %.lo: %.cpp
	${CXXCOMPILE} -c $<

test_query_events.o: %.o: %.cpp
	${CXX} -c ${CFLAGS} ${CPPUNIT_CFLAGS} $<


%.lo: %.c
	${COMPILE} -o $@ -c $<

soap_version.h:
	${gsoap_bin_prefix}/soapcpp2 /dev/null
	perl -ne '$$. == 2 && /.*([0-9]+)\.([0-9]+)\.([0-9]+)([a-z]?).*/ && printf "#define GSOAP_VERSION %d%02d%02d\n#define GSOAP_VERSION_LETTER '\''".($$4?$$4:"\\0")."'\''\n",$$1,$$2,$$3' soapH.h >$@
	-rm soapC.cpp soapH.h soapStub.h soapClient.cpp soapServer.cpp soapClientLib.cpp soapServerLib.cpp

ifeq ($(GLITE_LB_SERVER_WITH_WS),yes)
bkserverd.o ws_fault.o: soap_version.h
endif

