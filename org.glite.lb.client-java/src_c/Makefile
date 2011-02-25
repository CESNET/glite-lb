-include ../Makefile.inc
-include ../../Makefile.inc

CC=gcc
PATH_TO_JAVA=${JAVA_HOME}

OS=$(shell uname | tr '[:upper:]' '[:lower:]')

LIB=libglite_lb_sendviasocket.la
OBJ=send_via_socket.lo

VPATH=${topdir}/src_c

compile: ${LIB}

${LIB}: ${OBJ}
	libtool --mode=link ${CC} -rpath ${PREFIX}${prefix}/${libdir} -o $@ ${OBJ}

%.lo: %.c
	libtool --mode=compile ${CC} -I.. -I${PATH_TO_JAVA}/include -I${PATH_TO_JAVA}/include/$(OS) -c $<

install: compile
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	libtool --mode=install install -m 755 ${LIB} ${DESTDIR}${PREFIX}${prefix}/${libdir}

clean:
	rm -rf ${OBJ} .libs ${LIB}
