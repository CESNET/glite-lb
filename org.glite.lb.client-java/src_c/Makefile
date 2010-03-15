-include ../Makefile.inc
-include ../../Makefile.inc

CC=gcc
PATH_TO_JAVA=${JAVA_HOME}

LIB=libglite_lb_sendviasocket.la
OBJ=send_via_socket.lo

VPATH=${topdir}/src_c

compile: ${LIB}

${LIB}: ${OBJ}
	libtool --mode=link ${CC} -rpath ${PREFIX}/${libdir} -o $@ ${OBJ}

%.lo: %.c
	libtool --mode=compile ${CC} -I.. -I${PATH_TO_JAVA}/include -I${PATH_TO_JAVA}/include/linux -c $<

install: compile
	-mkdir -p ${PREFIX}/${libdir}
	libtool --mode=install install -m 755 ${LIB} ${PREFIX}/${libdir}

clean:
	rm -rf ${OBJ} .libs ${LIB}
