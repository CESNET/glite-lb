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

VPATH=${top_srcdir}/src
AT3=perl -I${top_srcdir}/project ${top_srcdir}/project/at3

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
	-I${mysql_prefix}/include \
	-I${globus_prefix}/include/${nothrflavour} \
	-I${globus_prefix}/include/${nothrflavour}/openssl

LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

# assisst & gss due to VOMS only
#	-lglobus_gss_assist_${nothrflavour} \
#	-lglobus_gssapi_gsi_${nothrflavour} \

GLOBUS_LIBS:= -L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lssl_${nothrflavour}


# XXX: our vomsc.la depends on badly installed expat

# not yet
#	-L${repository}/${gacl}/lib -lgacl \
#	${repository}/${voms}/lib/libvomsc.a \

EXT_LIBS:= -L${ares_prefix}/lib -lares \
	-L${mysql_prefix}/lib -lmysqlclient -lz\
	-L${expat_prefix}/lib -lexpat \
	${GLOBUS_LIBS}

COMMON_LIB:= -L${stagedir}/lib -lglite_lb_common_${nothrflavour}

HELPERS:= -L${stagedir}/lib -lglite_wmsutils_tls_ssl_helpers


SERVER_OBJS:= bkserverd.o get_events.o index.o jobstat.o jobstat_supp.o \
	write2rgma.o lbs_db.o lb_html.o lb_http.o lb_proto.o lb_xml_parse.o \
	lb_xml_parse_V21.o \
	lock.o openserver.o query.o userjobs.o db_store.o request.o store.o \
	stored_master.o srv_purge.o server_state.o dump.o lb_authz.o load.o \
	notification.o il_notification.o notif_match.o

INDEX_OBJS:= index.o index_parse.o jobstat_supp.o lbs_db.o openserver.o \
	jobstat.o query.o lock.o get_events.o write2rgma.o index_lex.o \
	lb_authz.o store.o bkindex.o

default: stage

compile: glite_lb_bkserverd glite_lb_bkindex

glite_lb_bkserverd: ${SERVER_OBJS}
	${LINK} -o $@ ${SERVER_OBJS} ${COMMON_LIB} ${HELPERS} ${EXT_LIBS}

glite_lb_bkindex: ${INDEX_OBJS}
	${LINK} -o $@ ${INDEX_OBJS} ${COMMON_LIB} ${HELPERS} ${EXT_LIBS}

all stage export: compile
	${INSTALL} -m 755 glite_lb_bkserverd glite_lb_bkindex ${stagedir}/bin


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
