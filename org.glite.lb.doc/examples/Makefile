glite_location:=${GLITE_LOCATION}
glite_prefix:=${glite_location}

nothrflavour=gcc32dbg
thrflavour=gcc32dbgpthr

CC:=gcc
CXX:=g++

COMMON_LIB:=-lglite_lb_common_${nothrflavour}
COMMON_LIB_THR:=-lglite_lb_common_${thrflavour}
CLIENT_LIB:=-lglite_lb_client_${nothrflavour}
CLIENT_LIB_THR:=-lglite_lb_client_${thrflavour}
JOBID_LIB:=-lglite_jobid

LIB:=${COMMON_LIB} ${CLIENT_LIB} ${JOBID_LIB}
LIB_THR:=${COMMON_LIB_THR} ${CLIENT_LIB_THR} ${JOBID_LIB}
PLUSLIB:=
PLUSLIB_THR:=

CFLAGS:=${DEBUG} \
	-I${glite_prefix}/include \
	-D_GNU_SOURCE 

CXXFLAGS:=${CFLAGS}

host_cpu:=${shell uname -m}
ifeq (${host_cpu},x86_64)
	LDFLAGS:=-L${glite_prefix}/lib64 
else
	LDFLAGS:=-L${glite_prefix}/lib
endif

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
CXXCOMPILE:=libtool --mode=compile ${CXX} ${CXXFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
INSTALL:=libtool --mode=install install

EXAMPLES_SRC:=example1.c
EXAMPLES:=${EXAMPLES_SRC:.c=}

EXTRA_SRC:=util.c
EXTRA_OBJ:=${EXTRA_SRC:.c=.o}

# TODO: migrate them here from branch_RC31_3
# EXAMPLES_PLUS_SRC:=
# EXAMPLES_PLUS:=${EXAMPLES_PLUS_SRC:.cpp=}
# EXAMPLES_PLUS_THR_SRC:=
# EXAMPLES_PLUS_THR:=${EXAMPLES_PLUS_THR_SRC:.cpp=}

EXAMPLES_THR_SRC:=
EXAMPLES_THR:=${EXAMPLES_CL_THR_SRC:.c=}

default all: examples

examples: ${EXAMPLES} ${EXAMPLES_THR} ${EXAMPLES_PLUS} ${EXAMPLES_PLUS_THR}

${EXAMPLES}: %: %.o ${EXTRA_OBJ}
	${LINK} -o $@ $< ${EXTRA_OBJ} ${LIB}

${EXAMPLES_THR}: %: %.o
	${LINK} -o $@ $< ${LIB_THR}

${EXAMPLES_PLUS}: %: %.o
	${LINKXX} -o $@ $< ${PLUSLIB}

${EXAMPLES_PLUS_THR}: %: %.o
	${LINKXX} -o $@ $< ${PLUSLIB_THR}

# catches $TOOLS and logevent compilation
%.o: %.c 
	${CC} ${CFLAGS} -c $<

%.o: %.cpp
	${CXX} ${CXXFLAGS} -c $<

clean:
	rm -rvf ${EXAMPLES} ${EXAMPLES_CL} ${EXAMPLES_CL_THR} 
	rm -rvf *.o .libs/

