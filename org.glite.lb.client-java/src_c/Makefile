default: all

-include ../../Makefile.inc

CC=gcc
PATH_TO_JAVA=${JAVA_HOME}

OS=$(shell uname | tr '[:upper:]' '[:lower:]')

# when there is a system prefix, we are probably working with java from GCC,
# with java headers already in default search paths
ifeq (${PATH_TO_JAVA},/usr)
JDK_CPPFLAGS=
else
JDK_CPPFLAGS=-I${PATH_TO_JAVA}/include/$(OS)
endif

LIB=libglite_lb_sendviasocket.la
OBJ=send_via_socket.lo

VPATH=${topdir}/src_c

all compile: ${LIB}

${LIB}: ${OBJ}
	libtool --mode=link ${CC} -rpath ${PREFIX}${prefix}/${libdir} -o $@ ${OBJ} -lm

%.lo: %.c
	libtool --mode=compile ${CC} -I.. -I${PATH_TO_JAVA}/include ${JDK_CPPFLAGS} -c $<

install: compile
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	libtool --mode=install install -m 755 ${LIB} ${DESTDIR}${PREFIX}${prefix}/${libdir}

clean:
	rm -rf ${OBJ} .libs ${LIB}

.PHONY: default all compile install clean
