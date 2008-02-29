# defaults
top_srcdir=.
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-state-machine
version=0.2.0
PREFIX=/opt/glite

-include Makefile.inc

VPATH = ../src:../interface

default all: compile

CC=gcc
CXX=g++
AT3=${stagedir}/sbin/glite-lb-at3

SUFFIXES = .T 

DEBUG:=-g -O0 -Wall

CFLAGS:= \
	${DEBUG} \
	-I${classads_prefix}/include \
	-I${stagedir}/include -I${top_srcdir}/src -I. \
	-I${top_srcdir}/interface 

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/lib  ${LDFLAGS} 
SOLINK:=libtool --mode=link ${CC} -module ${LDFLAGS} -rpath ${stagedir}/lib
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install
LINKXX:=libtool --mode=link  ${CXX} -rpath ${stagedir}/lib ${LDFLAGS}
XSLTPROC:=xsltproc --novalid

EXT_LIBS:=
COMMON_LIBS:= -L${stagedir}/lib  -lglite_lb_common_${nothrflavour} -lglite_security_gss_${nothrflavour}
PLUGIN_LIBS:= -L${stagedir}/lib -lglite_lb_common_${nothrflavour}\
	${classadslib} -lstdc++ ${expatlib} -lexpat\

PLUGIN_LOBJS:=seqcode_aux.lo process_event.lo process_event_pbs.lo process_event_condor.lo lb_plugin.lo
MACHINE_OBJS:=seqcode_aux.o process_event.o process_event_pbs.o process_event_condor.o

PLUGIN_LIB=glite_lb_plugin.la
MACHINE_LIB=libglite_lb_statemachine.a

default all: compile

compile: ${MACHINE_LIB} ${PLUGIN_LIB}

${PLUGIN_LIB}: ${PLUGIN_LOBJS}
	${SOLINK} -o $@ ${PLUGIN_LOBJS} ${PLUGIN_LIBS}

${MACHINE_LIB}:  ${MACHINE_OBJS}
	ar crv $@ ${MACHINE_OBJS}
	-ranlib $@


doc:

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install:
	-mkdir -p ${PREFIX}/lib ${PREFIX}/interface ${PREFIX}/include/glite/lb
	install -m 644 ${MACHINE_LIB} ${PREFIX}/lib 
#	${INSTALL} -m 755 ${PLUGIN_LIB} ${PREFIX}/lib
	install -m 644 ${top_srcdir}/interface/lb-job-attrs.xsd ${PREFIX}/interface
	install -m 644 ${top_srcdir}/interface/lb-job-record.xsd ${PREFIX}/interface
	install -m 644 ${top_srcdir}/interface/intjobstat.h ${PREFIX}/include/glite/lb


clean:
	rm -rvf .libs *.o *.lo ${PLUGIN_LIB} ${MACHINE_LIB} job_attrs.h
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

%.cpp: %.cpp.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

%.o %.lo: %.c
	${COMPILE} -c $<

%.lo: %.c
	${COMPILE} -o $@ -c $<

lb_plugin.lo: lb_plugin.c job_attrs.h
	${COMPILE} -DPLUGIN_DEBUG -o $@ -c $<

job_attrs.h: lb-job-attrs.xsd job_attrs.xsl
	${XSLTPROC} ../src/job_attrs.xsl $< >$@

