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

default all: compile

CC=gcc
CXX=g++
AT3=${stagedir}/sbin/glite-lb-at3

SUFFIXES = .T 

DEBUG:=-g -O0 -Wall

CFLAGS:= \
	${DEBUG} \
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

PLUGIN_LOBJS:=
MACHINE_BASE_OBJS:=

default all: compile

compile: 

doc:

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

install:

clean:

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

lb_plugin.lo: lb_plugin.c jp_job_attrs.h
	${COMPILE} -DPLUGIN_DEBUG -o $@ -c $<

