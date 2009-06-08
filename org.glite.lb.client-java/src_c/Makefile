CC=gcc
PREFIX=${HOME}/tmp
PATH_TO_JAVA=${JAVA_HOME}

LIB=libglite_lb_sendviasocket.la
OBJ=send_via_socket.lo

compile: ${LIB}

${LIB}: ${OBJ}
	libtool --mode=link ${CC} -rpath ${PREFIX}/lib -o $@ ${OBJ}

%.lo: %.c
	libtool --mode=compile ${CC} -I${PATH_TO_JAVA}/include -I${PATH_TO_JAVA}/include/linux -c $<

install: compile
	-mkdir -p ${PREFIX}/lib
	libtool --mode=install install -m 755 ${LIB} ${PREFIX}/lib

clean:
	rm -rf ${OBJ} .libs ${LIB}
