# defaults
top_srcdir=.
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-server-bones
version=0.0.1
PREFIX=/opt/glite

glite_location=/opt/glite

CC=gcc

-include Makefile.inc
-include ../project/version.properties

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/examples

DEBUG:=-g -O0 -Wall
CFLAGS:= ${DEBUG} -I${top_srcdir}/interface
LDFLAGS:=

ifdef LB_PROF
	CFLAGS:= ${CFLAGS} -pg -g
	LDFLAGS:= ${LDFLAGS} -pg
endif

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/lib ${LDFLAGS} 
INSTALL:=libtool --mode=install install

STATICLIB:=libglite_lb_server_bones.a
LTLIB:=libglite_lb_server_bones.la

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
	mkdir -p ${PREFIX}/include/${globalprefix}/${lbprefix}
	mkdir -p ${PREFIX}/lib
	${INSTALL} -m 644 ${LTLIB} ${PREFIX}/lib
	if [ x${DOSTAGE} = xyes ]; then \
		${INSTALL} -m 644 ${STATICLIB} ${PREFIX}/lib ; \
		cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${lbprefix} ; \
	fi

clean:

%.o: %.c
	${COMPILE} -c $<
