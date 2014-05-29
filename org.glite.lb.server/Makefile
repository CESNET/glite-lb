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

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}
docdir?=${prefix}/share/doc/${package}-${version}

SOURCES=\
	config/glite-lb* config/my.cnf config/*.conf config/startup config/startup.* config/setup config/systemd.* \
	doc/*.1 doc/*.8 \
	interface/*.h \
	src/*.c src/*.cpp src/*.h src/*.c.T src/*.l src/*.y src/*.sh src/*.sh.in src/ws_typemap.dat \
	project/.post* project/.pre* \
	test/*.cpp test/*.cpp.T test/*.sh test/oneJob.qry test/*.c \
	Makefile
SOURCES_EXEC=config/glite-lb-migrate* config/startup config/startup.* config/setup src/*.sh test/*.sh

all: compile

GLITE_LB_SERVER_WITH_WS=yes
GLITE_LB_SERVER_WITH_LCAS=no

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

sysdefaultdir?=${sysconfdir}/default

ifeq ($(os_type),systemd)
unitdir=`pkg-config systemd --variable=systemdsystemunitdir`
endif
mysqlconfdir?=${sysconfdir}/mysql/conf.d

GSOAP_FILES_PREFIX:= bk_ws_

YACC=bison -y
CC=gcc
CXX=g++

ifeq ($(gsoap_version),2.7.0)
	VPATH=${top_srcdir}/interface:${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/doc:${top_srcdir}/examples:${top_srcdir}/project:${gsoap_prefix}
else
	VPATH=${top_srcdir}/interface:${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/doc:${top_srcdir}/examples:${top_srcdir}/project
endif
AT3=${stagedir}${prefix}/bin/glite-lb-at3

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
GRIDSITE_LIBS?=-L${gridsite_prefix}/${libdir} -L${gridsite_prefix}/lib -lgridsite `xml2-config --libs`

ifeq (${GLITE_LB_SERVER_WITH_LCAS},yes)
LCAS_CFLAGS?=-I${lcas_prefix}/include -I${lcas_prefix}/include/glite/security -DHAVE_LCAS
LCAS_LIBS?=-L${lcas_prefix}/${libdir} -llcas
endif

ifneq (${classads_prefix},/usr)
	classads_libs := -L${classads_prefix}/${libdir} -L${classads_prefix}/lib
endif
CLASSADS_CFLAGS?=-I${classads_prefix}/include -I${classads_prefix}/include/classad
CLASSADS_LIBS?=${classads_libs} -lclassad
classad_namespace:=${shell if ${classads_prefix}/bin/classad_version | grep -i 'classad namespace.*no' >/dev/null; then echo no; else echo yes; fi 2>/dev/null}
ifeq (${classad_namespace},yes)
CLASSADS_CFLAGS+=-DCLASSAD_NAMESPACE=classad::
else
CLASSADS_CFLAGS+=-DCLASSAD_NAMESPACE=
endif

ifneq (${cares_prefix},/usr)
	cares_libs := -L${cares_prefix}/${libdir} -L${cares_prefix}/lib
endif
LIBCARES_CFLAGS?=-I${cares_prefix}/include
LIBCARES_LIBS?=${cares_libs} -lcares

GSOAP_CFLAGS?=-I${gsoap_prefix}/include -I${gsoap_prefix}/
ifeq (${expat_prefix},/usr)
EXPAT_CFLAGS?=
EXPAT_LIBS?=-lexpat
else
EXPAT_CFLAGS?=-I${expat_prefix}/include
EXPAT_LIBS?=-L${expat_prefix}/${libdir} -lexpat
endif
GLOBUS_GSSAPI_GSI_CFLAGS?=-I${globus_prefix}/include/${nothrflavour}
KERBEROS_GSSAPI_GSI_CFLAGS=-DNO_GLOBUS_GSSAPI

CPPUNIT_LIBS?=-L${cppunit_prefix}/${libdir} -lcppunit
CPPUNIT_CFLAGS?=-I${cppunit_prefix}/include

VOMS_CFLAGS?=-I${voms_prefix}/include -I${voms_prefix}/include/glite/security
VOMS_LIBS?=-L${voms_prefix}/${libdir} -lvomsapi${vomsflavour}

GLITE_LBJP_COMMON_CFLAGS?=`PKG_CONFIG_PATH=$$PKG_CONFIG_PATH:${PREFIX}${prefix}/${libdir}/pkgconfig pkg-config glite-security-gss glite-security-gsoap-plugin --cflags`

ifeq ($(gssapi_provider), kerberos)
GSSAPI_GSI_CFLAGS=${KERBEROS_GSSAPI_GSI_CFLAGS}
VOMS_LIBS=
VOMS_CFLAGS=
else
GSSAPI_GSI_CFLAGS=${GLOBUS_GSSAPI_GSI_CFLAGS}
endif

CXXFLAGS?=${CFLAGS}

CPPFLAGS:= \
	${GSSAPI_GSI_CFLAGS} \
	${WS_CFLAGS} \
	-DVERSION=\"${version}\" \
	 ${GSOAP_CFLAGS} \
	-I${stagedir}${prefix}/include -I${top_srcdir}/src -I. \
	-I${top_srcdir}/interface \
	${EXPAT_CFLAGS} \
	${LIBCARES_CFLAGS} \
	${CLASSADS_CFLAGS} \
	${VOMS_CFLAGS} \
	$(GRIDSITE_CFLAGS) ${LCAS_CFLAGS} ${GSOAP_CFLAGS} \
	-D_GNU_SOURCE ${LB_STANDALONE_FLAGS} ${LB_PERF_FLAGS} -DLB_EVENTS_BLOB \
	${GLITE_LBJP_COMMON_CFLAGS}

CFLAGS:=${CPPFLAGS} ${DEBUG} ${COVERAGE_FLAGS} ${CFLAGS}
CXXFLAGS:=${CPPFLAGS} ${DEBUG} ${COVERAGE_FLAGS} ${CXXFLAGS}

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
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
SOLINK:=libtool --mode=link ${CC} -module ${LDFLAGS} -rpath ${stagedir}${prefix}/${libdir}
INSTALL:=libtool --mode=install install
LINKXX:=libtool --mode=link  ${CXX} ${LDFLAGS}
XSLTPROC:=xsltproc --novalid

gsoap_bin_prefix:=${shell if [ -x  ${gsoap_prefix}/bin/soapcpp2 ]; then echo ${gsoap_prefix}/bin; else echo ${gsoap_prefix}; fi }

EXT_LIBS:=  \
	${GRIDSITE_LIBS} \
	${CLASSADS_LIBS} \
	${LIBCARES_LIBS} 

ifeq ($(gssapi_provider), kerberos)
else
EXT_LIBS+= \
	${LCAS_LIBS} \
	${VOMS_LIBS}
endif

COMMON_LIBS:= -L${stagedir}${prefix}/${libdir} -lglite_lb_common${default_flavour} -lglite_security_gss${default_flavour} -lglite_lbu_trio -lglite_lbu_maildir -lglite_lbu_log

BKSERVER_BASE_OBJS:= \
	bkserverd.o il_lbproxy.o get_events.o index.o jobstat.o jobstat_supp.o \
	seqcode.o write2rgma.o lb_html.o lb_http.o lb_proto.o cond_dump.o lb_xml_parse.o \
	lb_xml_parse_V21.o lb_text.o \
	openserver.o query.o userjobs.o db_store.o request.o store.o \
	stored_master.o srv_purge.o server_state.o dump.o lb_authz.o load.o \
	notification.o il_notification.o notif_match.o stats.o server_stats.o db_calls.o db_supp.o lb_rss.o pretty_print_wrapper.o \
	policy_gram.o policy_lex.o authz_policy.o crypto.o

gsoap_version ?= ${gsoap_default_version}

dotless_gsoap_ver:=${shell echo ${gsoap_version} | tr -d . }
ifeq ($(shell test -f ${stagedir}${prefix}/${libdir}/libglite_security_gsoap_plugin_${dotless_gsoap_ver}${default_flavour}_c.so && echo ok),ok)
	langflavour:=_c
endif
GSOAP_LIB:=-L${stagedir}${prefix}/${libdir} -lglite_security_gsoap_plugin_${dotless_gsoap_ver}${default_flavour}${langflavour} -lglite_security_gss${default_flavour} ${GSOAP_LIBS}

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
		${EXPAT_LIBS} \
		-lglite_lbu_maildir \
		-lglite_lbu_log \
		-lglite_lbu_trio \
		-lglite_jobid
else
	BKSERVER_OBJS:= ${BKSERVER_BASE_OBJS}
		
	BKSERVER_LIBS= \
		${SRVBONES_LIB} \
		${LB_COMMON_LIB} \
		${LB_MACHINE_LIB} \
		${LB_UTILS_DB_LIB} \
		-lglite_security_gss${default_flavour} \
		${EXT_LIBS} \
		${EXPAT_LIBS} \
		-lglite_lbu_maildir \
		-lglite_lbu_log \
		-lglite_lbu_trio \
		-lglite_jobid
endif

INDEX_OBJS:= index.o index_parse.o jobstat_supp.o openserver.o \
	jobstat.o query.o get_events.o write2rgma.o index_lex.o \
	lb_authz.o store.o bkindex.o stats.o server_stats.o\
	request.o db_store.o srv_purge.o notif_match.o il_lbproxy.o dump.o lb_xml_parse.o il_notification.o lb_proto.o lb_text.o server_state.o lb_xml_parse_V21.o lb_html.o cond_dump.o notification.o seqcode.o userjobs.o load.o db_calls.o db_supp.o lb_rss.o pretty_print_wrapper.o authz_policy.o crypto.o

INDEX_LIBS:= ${SRVBONES_LIB} ${COMMON_LIBS} ${LB_MACHINE_LIB} ${EXT_LIBS} \
	${EXPAT_LIBS} ${LB_UTILS_DB_LIB} -lglite_lbu_trio -lglite_jobid

WS_CLIENT_OBJS:= $(GSOAP_FILES_PREFIX)C.o $(GSOAP_FILES_PREFIX)Client.o ws_fault.o ws_typeref.o
WS_CLIENT_LIBS:= ${GSOAP_LIB} -lglite_lb_common${default_flavour} \
	-lglite_lb_common${default_flavour}

HDRS=index.h lb_authz.h store.h

MAN1_GZ:=glite-lb-mon-db.1.gz
MAN8_GZ:=glite-lb-bkindex.8.gz glite-lb-bkserverd.8.gz glite-lb-setup.8.gz
MAN_GZ:=$(MAN1_GZ) $(MAN8_GZ)
MAN = $(MAN_GZ:.gz=)

LIB_OBJS_BK:= \
	il_lbproxy.o get_events.o index.o jobstat.o jobstat_supp.o \
	seqcode.o lb_html.o lb_http.o lb_proto.o cond_dump.o lb_xml_parse.o \
	lb_xml_parse_V21.o lb_text.o \
	openserver.o query.o userjobs.o db_store.o request.o store.o \
	stored_master.o srv_purge.o server_state.o dump.o lb_authz.o load.o \
	notification.o il_notification.o notif_match.o stats.o server_stats.o write2rgma.o \
	db_calls.o db_supp.o lb_rss.o pretty_print_wrapper.o authz_policy.o \
	crypto.o

MONDB_OBJS:=mon-db.o ${LIB_OBJS_BK}
MONDB_LIBS:=${COMMON_LIBS} ${LB_MACHINE_LIB} ${LB_UTILS_DB_LIB} ${EXT_LIBS} \
	${EXPAT_LIBS} -lglite_lbu_trio -lglite_jobid

ifeq (${GLITE_LB_SERVER_WITH_LCAS},yes)
LCAS_PLUGIN_OBJS:=lcas_lb.o
LCAS_PLUGIN_LOBJS:=${LCAS_PLUGIN_OBJS:.o=.lo}
LCAS_PLUGIN_LIB:=liblcas_lb.la
endif

EXAMPLES=

TESTS=test_coverage test_xml test_query_events test_soap_conv test_query_parse

SCRIPTS=startup setup glite-lb-server-purge.cron glite-lb-proxy-purge.cron glite-lb-notif-keeper.cron glite-lb
SCRIPTS2=export.sh notif-keeper.sh
SCRIPTS_SRC=$(addprefix ${top_srcdir}/config/,${SCRIPTS})

glite-lb-bkserverd: ${NSMAP} ${BKSERVER_OBJS}
	${LINKXX} -o $@ ${BKSERVER_OBJS} ${BKSERVER_LIBS} -Wl,--export-dynamic

glite-lb-bkindex: ${INDEX_OBJS}
	${LINKXX} -o $@ ${INDEX_OBJS} ${INDEX_LIBS}

glite-lb-mon-db: ${MONDB_OBJS}
	${LINKXX} -o $@ ${MONDB_OBJS} ${MONDB_LIBS}

compile: generate ${SCRIPTS} ${SCRIPTS2} glite-lb-bkserverd glite-lb-bkindex glite-lb-mon-db ${STATIC_LIB_BK} ${LCAS_PLUGIN_LIB} ${MAN_GZ}

generate: store.c index.c 

${SCRIPTS}: ${SCRIPTS_SRC}
	glite_var="${localstatedir}/glite"; \
	if echo "${localstatedir}" | grep 'glite'>/dev/null; then \
		glite_var="${localstatedir}"; \
	fi; \
	sed -e 's:@glite_prefix@:${sysroot}${prefix}:g' -e 's:@glite_etc@:${sysconfdir}:g' -e "s:@glite_var@:$$glite_var:g" -e 's:@defaultdir@:${sysdefaultdir}:g' -e 's:@mysqlconfdir@:${mysqlconfdir}:' ${top_srcdir}/config/$@ > $@
ifneq ($(os_type),debian)
	sed -i 's/# \(Default-Start\|Default-Stop\):.*/# \1:/' $@
endif
	sed "s/^@$(os_type)@//" $@ > $@.tmp
	grep -v '^@' $@.tmp > $@
	rm $@.tmp

%.sh: %.sh.in
	glite_var="${localstatedir}/glite"; \
	if echo "${localstatedir}" | grep 'glite'>/dev/null; then \
		glite_var="${localstatedir}"; \
	fi; \
	sed -e 's:@glite_prefix@:${sysroot}${prefix}:g' -e 's:@glite_etc@:${sysconfdir}:g' -e "s:@glite_var@:$$glite_var:g" -e 's:@defaultdir@:${sysdefaultdir}:g' $< > $@

# XXX test.query disabled (coredumps, doesn't build anymore_
check: compile test.xml test.query_parse

test_coverage:
	-mkdir coverage
	cd coverage && $(MAKE) -f ../Makefile top_srcdir=../../ COVERAGE_FLAGS="-fprofile-arcs -ftest-coverage" check
	cd coverage && for i in `echo ${INDEX_OBJS} ${BKSERVER_OBJS} | tr ' ' '\012' | sort -u`; do gcov $$i ; done

lb_xml_parse.c: lb_xml_parse.c.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

${GSOAP_FILES_PREFIX}H.h ${GSOAP_FILES_PREFIX}C.c ${GSOAP_FILES_PREFIX}Server.c ${GSOAP_FILES_PREFIX}Client.c ${GSOAP_FILES_PREFIX}ServerLib.c ${GSOAP_FILES_PREFIX}ClientLib.c LoggingAndBookkeeping.nsmap: gsoap.stamp

gsoap.stamp: LB.xh
	${gsoap_bin_prefix}/soapcpp2 -w -c -p ${GSOAP_FILES_PREFIX} LB.xh
	touch gsoap.stamp

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
	${LINKXX} -o $@ test_xml.o lb_xml_parse.o ${LB_COMMON_LIB} ${CPPUNIT_LIBS} ${EXPAT_LIBS} -lglite_lbu_trio -lglite_jobid

test.query: test_query_events
	# XXX coredumps ./test_query_events

#query_events_objs:=`echo ${BKSERVER_BASE_OBJS} |sed 's/bkserverd\.o//;s/db_store\.o//;s/stored_master\.o//;s/store\.o//;s/srv_purge\.o//;s/load\.o//;s/dump\.o//;s/lb_proto\.o//;s/lb_html\.o//;s/lb_http\.o//;'`

query_events_objs := test_query_events.o query.o get_events.o jobstat_supp.o index.o db_supp.o

test_query_events: ${query_events_objs}
	${LINKXX} -o $@ ${query_events_objs} \
	${CPPUNIT_LIBS} ${LB_COMMON_LIB} ${LB_MACHINE_LIB} ${CLASSADS_LIBS} -lglite_lbu_log -lglite_lbu_trio -lglite_security_gss${default_flavour} -lglite_jobid

test.soapconv: test_soap_conv
	LD_LIBRARY_PATH=${cares_prefix}/${libdir}:${classads_prefix}/${libdir}:${LD_LIBRARY_PATH} ./test_soap_conv

test_soap_conv: test_soap_conv.cpp ${WS_CLIENT_OBJS}
	${CXX} -c ${CFLAGS} ${CPPUNIT_CFLAGS} $<
	${LINKXX} -o $@ test_soap_conv.o ${WS_CLIENT_OBJS} ${WS_CLIENT_LIBS} ${CPPUNIT_LIBS}

test.query_parse: test_query_parse
	./test_query_parse

test_query_parse: test_query_parse.o
	${LINKXX} -o $@ ${COMMON_LIBS} ${LB_UTILS_DB_LIB} ${LB_MACHINE_LIB} lb_proto.o db_supp.o lb_authz.o lb_html.o authz_policy.o query.o lb_xml_parse_V21.o lb_xml_parse.o server_stats.o lb_rss.o lb_text.o srv_purge.o notification.o userjobs.o jobstat.o dump.o crypto.o load.o server_state.o get_events.o write2rgma.o stats.o db_store.o notif_match.o il_notification.o index.o store.o db_calls.o il_lbproxy.o cond_dump.o jobstat_supp.o openserver.o seqcode.o pretty_print_wrapper.o ${EXT_LIBS} ${CPPUNIT_LIBS} ${EXPAT_LIBS} -lglite_jobid $+

examples: ${EXAMPLES}

${STATIC_LIB_BK}: ${LIB_OBJS_BK}
	ar crv $@ ${LIB_OBJS_BK}
	ranlib $@

${LCAS_PLUGIN_LIB}: ${LCAS_PLUGIN_LOBJS}
	${SOLINK} -o $@ $^

doc: man

man: ${MAN_GZ}

%.1.gz: %.1
	gzip -9 -f -c $< > $@

%.8.gz: %.8
	gzip -9 -f -c $< > $@

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

install:
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/bin ${DESTDIR}${PREFIX}${sysconfdir}/glite-lb
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/sbin
	-mkdir -p ${DESTDIR}${PREFIX}${docdir}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/glite
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/man/man1
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/man/man8
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	-mkdir -p ${DESTDIR}${PREFIX}${sysconfdir}/logrotate.d
	-mkdir -p ${DESTDIR}${PREFIX}${sysconfdir}/cron.d
	-mkdir -p ${DESTDIR}${PREFIX}${mysqlconfdir}
	-mkdir -p ${DESTDIR}${PREFIX}${sysdefaultdir}
	-mkdir -p -m 0775 ${DESTDIR}${PREFIX}/var/spool/glite/lb-locallogger
	-mkdir -p -m 0755 ${DESTDIR}${PREFIX}/var/spool/glite/lb-notif
	-mkdir -p -m 0755 ${DESTDIR}${PREFIX}/var/spool/glite/lb-proxy
	${INSTALL} -m 644 ${MAN1_GZ} ${DESTDIR}${PREFIX}${prefix}/share/man/man1
	${INSTALL} -m 644 ${MAN8_GZ} ${DESTDIR}${PREFIX}${prefix}/share/man/man8
	${INSTALL} -m 755 glite-lb-mon-db ${DESTDIR}${PREFIX}${prefix}/bin/glite-lb-mon-db
	if test -n "${LCAS_PLUGIN_LIB}"; then \
		-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}/modules; \
		${INSTALL} -m 755 ${LCAS_PLUGIN_LIB} ${DESTDIR}${PREFIX}${prefix}/${libdir}/modules; \
		ln -sf liblcas_lb.so ${DESTDIR}${PREFIX}${prefix}/${libdir}/modules/lcas_lb.mod; \
	fi
	${INSTALL} -m 755 glite-lb-bkindex ${DESTDIR}${PREFIX}${prefix}/sbin/glite-lb-bkindex
	${INSTALL} -m 755 glite-lb-bkserverd ${DESTDIR}${PREFIX}${prefix}/sbin/glite-lb-bkserverd
	${INSTALL} -m 755 export.sh ${DESTDIR}${PREFIX}${prefix}/sbin/glite-lb-export
	${INSTALL} -m 755 notif-keeper.sh ${DESTDIR}${PREFIX}${prefix}/sbin/glite-lb-notif-keeper

	${INSTALL} -m 644 ${top_srcdir}/config/glite-lb-dbsetup.sql ${DESTDIR}${PREFIX}${prefix}/share/glite
	${INSTALL} -m 644 ${top_srcdir}/config/glite-lb-index.conf.template ${DESTDIR}${PREFIX}${sysconfdir}/glite-lb
	${INSTALL} -m 755 ${top_srcdir}/config/glite-lb-migrate_db2version20 ${DESTDIR}${PREFIX}${prefix}/sbin
	${INSTALL} -m 755 ${top_srcdir}/config/glite-lb-migrate_db2version40 ${DESTDIR}${PREFIX}${prefix}/sbin
	${INSTALL} -m 644 ${top_srcdir}/config/site-notif.conf ${DESTDIR}${PREFIX}${sysconfdir}/glite-lb

	${INSTALL} -m 755 setup ${DESTDIR}${PREFIX}${prefix}/sbin/glite-lb-setup
	${INSTALL} -m 644 glite-lb-server-purge.cron ${DESTDIR}${PREFIX}${sysconfdir}/cron.d/${package}-purge
	${INSTALL} -m 644 glite-lb-proxy-purge.cron ${DESTDIR}${PREFIX}${sysconfdir}/cron.d/${package}-purge-proxy
	${INSTALL} -m 644 glite-lb-notif-keeper.cron ${DESTDIR}${PREFIX}${sysconfdir}/cron.d/${package}-notif-keeper
	${INSTALL} -m 644 ${top_srcdir}/config/glite-lb-server.logrotate ${DESTDIR}${PREFIX}${sysconfdir}/logrotate.d/glite-lb-server
	${INSTALL} -m 644 ${top_srcdir}/config/my.cnf ${DESTDIR}${PREFIX}${mysqlconfdir}/glite-lb-server.cnf
	${INSTALL} -m 644 glite-lb ${DESTDIR}${PREFIX}${docdir}/glite-lb
	${INSTALL} -m 644 glite-lb ${DESTDIR}${PREFIX}${sysdefaultdir}

	install -m 644 ${STATIC_LIB_BK} ${DESTDIR}${PREFIX}${prefix}/${libdir}
ifdef LB_PERF
	${INSTALL} -m 755 ${top_srcdir}/src/perftest_proxy.sh ${DESTDIR}${PREFIX}${prefix}/sbin
endif
ifeq ($(os_type),redhat)
	mkdir -p ${DESTDIR}${PREFIX}${sysconfdir}/rc.d/init.d
	${INSTALL} -m 755 ${top_srcdir}/config/startup.redhat.bkserverd ${DESTDIR}${PREFIX}${sysconfdir}/rc.d/init.d/glite-lb-bkserverd
endif
ifeq ($(os_type),systemd)
	mkdir -p ${DESTDIR}${PREFIX}${unitdir}
	install -m 644 ${top_srcdir}/config/systemd.bkserverd ${DESTDIR}${PREFIX}${unitdir}/glite-lb-bkserverd.service
endif
ifeq ($(os_type),debian)
	mkdir -p ${DESTDIR}${PREFIX}${sysconfdir}/init.d
	${INSTALL} -m 755 ${top_srcdir}/config/startup.debian.bkserverd ${DESTDIR}${PREFIX}${sysconfdir}/init.d/glite-lb-bkserverd
endif

clean:
	rm -rvf *.c *.cpp *.h *.ch *.xh *.xml *.nsmap *.o *.lo .libs glite-lb-bkindex glite-lb-bkserverd glite-lb-mon-db ${STATIC_LIB_BK} ${LCAS_PLUGIN_LIB} ${TESTS} ${MAN_GZ} ${EXAMPLES} ${SCRIPTS} ${SCRIPTS2} ${MAN_GZ}
	rm -vf gsoap.stamp
	rm -rvf test_xml.cpp
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

%.o: %.y
	-mkdir $*
	(cd $*; \
	 ${YACC} -d ${YFLAGS} ../$<; \
	 mv y.tab.c ../$*.c; \
	 mv y.tab.h ../$*.h)
	rm -r $*
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
bkserverd.o ws_fault.o: soap_version.h gsoap.stamp
ws_fault.o ws_query.o ws_lb4agu.o ws_typeref.o bk_ws_C.o bk_ws_Client.o bk_ws_Server.o: gsoap.stamp
endif

.PHONY: default all compile check test_coverage test.xml test.query examples doc man stage install clean distclean dist distcheck
