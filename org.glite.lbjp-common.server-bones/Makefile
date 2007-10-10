# defaults
top_srcdir=..
stagedir=.
globalprefix=glite
lbutilsprefix=lbu
package=glite-lb-server-bones
version=0.0.1
PREFIX=/opt/glite

CC=gcc

-include Makefile.inc
-include ../project/version.properties

VPATH=${top_srcdir}/src:${top_srcdir}/examples

DEBUG:=-g -O0 -Wall
CFLAGS:= ${DEBUG} -I${top_srcdir}/interface -D_GNU_SOURCE
LDFLAGS:=

ifdef LB_PROF
	CFLAGS:= ${CFLAGS} -pg -g
	LDFLAGS:= ${LDFLAGS} -pg
endif

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/lib ${LDFLAGS} 
INSTALL:=libtool --mode=install install

STATICLIB:=libglite_lbu_server_bones.a
LTLIB:=libglite_lbu_server_bones.la

OBJS:=srvbones.o
LOBJS:=${OBJS:.o=.lo}

HDRS:=srvbones.h

default all: compile

compile: ${STATICLIB} ${LTLIB} example

${STATICLIB}: ${OBJS}
	ar crv $@ ${OBJS}
	ranlib $@

${LTLIB}: ${LOBJS}
	${LINK} -o $@ ${LOBJS}

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

check:
	-echo "No unit tests so far."

example: srv_example cnt_example

srv_example: srv_example.o
	${LINK} -o $@ ${LTLIB} srv_example.o

cnt_example: cnt_example.o
	${LINK} -o $@ cnt_example.o

doc:

install:
	mkdir -p ${PREFIX}/include/${globalprefix}/${lbutilsprefix}
	mkdir -p ${PREFIX}/lib
	${INSTALL} -m 644 ${LTLIB} ${PREFIX}/lib
	${INSTALL} -m 644 ${STATICLIB} ${PREFIX}/lib ; \
	cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${lbutilsprefix}

clean:
	rm -rvf *.o *.lo .libs lib* srv_example cnt_example
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

%.o: %.c
	${COMPILE} -c $<
