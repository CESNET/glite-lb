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

-include Makefile.inc

YACC=bison -y
CC=gcc

VPATH=${top_srcdir}/src:${top_srcdir}/test
AT3=perl -I${top_srcdir}/project ${top_srcdir}/project/at3

TEST_LIBS:=-L${cppunit}/lib -lcppunit
TEST_INC:=-I${cppunit}/include

SUFFIXES = .T 

DEBUG:=-g -O0 
# not yet
#	-I${voms}/include \
#	-I${gacl}/include \
#	-I/usr/include/libxml2 \

# -DNO_VOMS -DNO_GACL to be removed when voms/gridsite are available
CFLAGS:= -DNO_VOMS -DNO_GACL \
	${DEBUG} -I${stagedir}/include -I${top_srcdir}/src -I. \
	-I${expat_prefix}/include \
	-I${ares_prefix}/include \
	${COVERAGE_FLAGS} \
	-I${mysql_prefix}/include \
	-I${globus_prefix}/include/${nothrflavour} \
	-I${gridsite_prefix}/include -I${globus_prefix}/include/${nothrflavour}/openssl


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
	-lglobus_gssapi_gsi_${nothrflavour} \


# XXX: our vomsc.la depends on badly installed expat

# not yet
#	-L${repository}/${gacl}/lib -lgacl \
#	${repository}/${voms}/lib/libvomsc.a \

EXT_LIBS:= -L${ares_prefix}/lib -lares \
	-L${mysql_prefix}/lib -lmysqlclient -lz\
	-L${expat_prefix}/lib -lexpat \
	${GLOBUS_LIBS}

# -L${gridsite_prefix}/lib -lgridsite -lxml2

COMMON_LIB:= -L${stagedir}/lib -lglite_lb_common_${nothrflavour}

#	-lvomsc

SERVER_OBJS:= bkserverd.o get_events.o index.o jobstat.o jobstat_supp.o \
	write2rgma.o lbs_db.o lb_html.o lb_http.o lb_proto.o lb_xml_parse.o \
	lb_xml_parse_V21.o \
	lock.o openserver.o query.o userjobs.o db_store.o request.o store.o \
	stored_master.o srv_purge.o server_state.o dump.o lb_authz.o load.o \
	notification.o il_notification.o notif_match.o

INDEX_OBJS:= index.o index_parse.o jobstat_supp.o lbs_db.o openserver.o \
	jobstat.o query.o lock.o get_events.o write2rgma.o index_lex.o \
	lb_authz.o store.o bkindex.o

glite_lb_bkserverd: ${SERVER_OBJS}
	${LINK} -o $@ ${SERVER_OBJS} ${COMMON_LIB} ${EXT_LIBS}

glite_lb_bkindex: ${INDEX_OBJS}
	${LINK} -o $@ ${INDEX_OBJS} ${COMMON_LIB} ${EXT_LIBS}

default all: compile

compile: glite_lb_bkserverd glite_lb_bkindex

check: compile test.xml test.query
	-echo check.query not complete yet

test_xml: test_xml.cpp
	${CXX} -c ${CFLAGS} ${TEST_INC} $<
	${LINKXX} -o $@ test_xml.o lb_xml_parse.o ${COMMON_LIB} ${TEST_LIBS} ${EXT_LIBS}

test.query: test_query_events
	./test_query_events ${top_srcdir}/test

test.xml: test_xml
	./test_xml

lb_xml_parse.c: lb_xml_parse.c.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null


check.query: test_query_events
	./test_query_events

query_events_objs:=`echo ${SERVER_OBJS} | sed 's/lbs_db\.o//;s/bkserverd\.o//'`

test_query_events: test_query_events.o
	${LINKXX} -o $@ test_query_events.o ${query_events_objs} \
	${TEST_LIBS} ${COMMON_LIB} ${EXT_LIBS}

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
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
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

