include Makefile.inc

YACC=bison -y

VPATH:=${src} 
AT3:=perl -I${lbconfig} ${lbproject}/at3
SUFFIXES = .T 

DEBUG:=-g -O0 
CFLAGS:=${DEBUG} -I${stageinc} -I${src} -I. \
	-I${repository}/${voms}/include \
	-I${repository}/${gacl}/include \
	-I${repository}/${expat}/include \
	-I${repository}/${mysql}/include \
	-I/usr/include/libxml2 \
	-I${repository}/${globus}/include/${globusflavour} \
	-I${repository}/${globus}/include/${globusflavour}/openssl

LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

# assisst & gss due to VOMS only

GLOBUS_LIBS:= -L${repository}/${globus}/lib \
	-lglobus_gss_assist_${globusflavour} \
	-lglobus_gssapi_gsi_${globusflavour} \
	-lglobus_common_${globusflavour} \
	-lssl_${globusflavour}


# XXX: our vomsc.la depends on badly installed expat

EXT_LIBS:= -L${repository}/${ares}/lib -lares \
	-L${repository}/${mysql}/lib -lmysqlclient \
	-L${repository}/${gacl}/lib -lgacl \
	${repository}/${voms}/lib/libvomsc.a \
	-L${repository}/${expat}/lib -lexpat \
	${GLOBUS_LIBS}

COMMON_LIB:= -L${stagelib} -lglite_lb_common

HELPERS:= -L${stagelib} -lglite_wms_tls_ssl_helpers


SERVER_OBJS:= bkserverd.o get_events.o index.o jobstat.o jobstat_supp.o \
	write2rgma.o lbs_db.o lb_html.o lb_http.o lb_proto.o lb_xml_parse.o \
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
	${INSTALL} -m 755 glite_lb_bkserverd glite_lb_bkindex ${stagebin}


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
