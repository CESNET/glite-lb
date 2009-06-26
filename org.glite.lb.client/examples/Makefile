glite_location:=${GLITE_LOCATION}
glite_prefix:=${glite_location}

host_cpu:=${shell uname -m}
ifeq (${host_cpu},x86_64)
	LDFLAGS:=-L${glite_prefix}/lib64 -L${glite_prefix}/lib
	nothrflavour=gcc64dbg
	thrflavour=gcc64dbgpthr
else
	LDFLAGS:=-L${glite_prefix}/lib
	nothrflavour=gcc32dbg
	thrflavour=gcc32dbgpthr
endif

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

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
CXXCOMPILE:=libtool --mode=compile ${CXX} ${CXXFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
INSTALL:=libtool --mode=install install

EXAMPLES_SRC:=log_usertag_proxy.c job_log.c job_reg.c feed_shark.c query_ext.c query_seq_code.c stats.c abort_job.c change_acl.c stresslog.c flood_proxy.c dagids.c stress_context.c parse_eventsfile.c user_jobs.c job_status.c
EXAMPLES:=${EXAMPLES_SRC:.c=}

# TODO: migrate them here from branch_RC31_3
# EXAMPLES_PLUS_SRC:=indexed_attrs_plus.cpp job_status_plus.cpp query_events_plus.cpp listener.cpp user_jobs_plus.cpp job_log_plus.cpp notify_plus.cpp
# EXAMPLES_PLUS:=${EXAMPLES_PLUS_SRC:.cpp=}
# EXAMPLES_PLUS_THR_SRC:=
# EXAMPLES_PLUS_THR:=${EXAMPLES_PLUS_THR_SRC:.cpp=}

EXAMPLES_THR_SRC:=user_jobs_threaded.c
EXAMPLES_THR:=${EXAMPLES_CL_THR_SRC:.c=}

default all: examples

examples: ${EXAMPLES} ${EXAMPLES_THR} ${EXAMPLES_PLUS} ${EXAMPLES_PLUS_THR}

${EXAMPLES}: %: %.o
	${LINK} -o $@ $< ${LIB}

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

