# Default values
top_srcdir=.
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-client
version=0.0.0

nothrflavour=gcc32
thrflavour=gcc32pthr

CC:=gcc
CXX:=g++

-include Makefile.inc
-include ../project/version.properties

version=${module.version}

# We must follow major version changes of org.glite.lb.common due to
# binary compatibility. However, we may live a life of our own, changing our
# major version more frequently. This variable specifies how many steps ahead
# we are (the number can be even negative).

VERSION_AHEAD=-3

VPATH=${top_srcdir}/src:${top_srcdir}/interface:${top_srcdir}/test:${top_srcdir}/examples:${top_srcdir}/doc
AT3=${stagedir}/sbin/glite-lb-at3
GENSAM=${top_srcdir}/examples/gen_sample_job
CHECK_VERSION:=VERSION=${version} VERSION_AHEAD=${VERSION_AHEAD} perl ${stagedir}/sbin/glite-lb-check_version.pl

SUFFIXES = .T .l

l_SRC = \
	chkpt.l \
	cleared.l \
	done.l \
	done_dag.l \
	done_subjob.l \
	ready.l \
	ready_dag.l \
	ready_subjob.l \
	running.l \
	running_dag.l \
	running_subjob.l \
	scheduled.l \
	scheduled_dag.l \
	scheduled_subjob.l \
	submitted.l \
	submitted_dag.l \
	submitted_subjob.l \
	waiting.l \
	waiting_dag.l \
	waiting_subjob.l \
	failed_dag.l \
	failed_subjob.l \
	aborted.l \
	cancelled.l \

# TODO: missing resubmission_deep
#	shallow_resub_complex.l shallow_resub_simple.l shallow_resub_simple2.l \
#	resubmission.l resubmitted.l


sh_PROGS = $(l_SRC:.l=.sh)

ifdef DEBUG
	DEBUG:=-g -O0 -Wall -DEDG_WLL_LOG_STUB
else
	DEBUG:=-g -O0 -Wall
endif

ifdef LB_STANDALONE
        LB_STANDALONE_FLAGS:=-DLB_STANDALONE
endif

ifdef LB_PERF
	LB_PERF_FLAGS:=-DLB_PERF
	LB_PERF_TOOLS:=perftest_logjobs perftest_jobreg
endif
COMMON_LIB:=-lglite_lb_common_${nothrflavour}
COMMON_LIB_THR:=-lglite_lb_common_${thrflavour}
TRIO_LIB:=-lglite_lbu_trio

CFLAGS:=${DEBUG} \
	-I. \
	-I${top_srcdir}/src -I${top_srcdir}/interface \
	-I${stagedir}/include \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE ${LB_STANDALONE_FLAGS} ${LB_PERF_FLAGS}

CXXFLAGS:=${CFLAGS}

EXT_LIB:=

TEST_LIBS:=-L${cppunit_prefix}/lib -lcppunit
TEST_INC:=-I${cppunit_prefix}/include

# TODO: merge - check the 64bit stuff
#             - and what about the new JOBID lib?
archlib:=lib
host_cpu:=${shell uname -m}
ifeq (${host_cpu},x86_64)
	archlib:=lib64
	LDFLAGS:=-L${stagedir}/lib64 -L${stagedir}/lib ${COVERAGE_FLAGS}
else
	LDFLAGS:=-L${stagedir}/lib
endif

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
CXXCOMPILE:=libtool --mode=compile ${CXX} ${CXXFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
INSTALL:=libtool --mode=install install

LIBOBJS:=connection.o consumer.o notification.o prod_proto.o \
	producer.o uiwrap.o statistics.o
# TODO: restructuring - do we really need FAKE versions? maybe remove all FAKEs here
FAKELIBOBJS:=consumer_fake.o producer_fake.o

PLUSOBJS:=Event.o Job.o JobStatus.o Notification.o ServerConnection.o

HDRS:=consumer.h notification.h statistics.h prod_proto.h connection.h \
	Job.h Notification.h ServerConnection.h 
FAKE_HDRS:=consumer_fake.h producer_fake.h
GEN_HDRS:=JobStatus.h producer.h

LIBTHROBJS:=${LIBOBJS:.o=.thr.o}
LIBLOBJS:=${LIBOBJS:.o=.lo}
LIBTHRLOBJS:=${LIBOBJS:.o=.thr.lo}

PLUSTHROBJS:=${PLUSOBJS:.o=.thr.o}
PLUSLOBJS:=${PLUSOBJS:.o=.lo}
PLUSTHRLOBJS:=${PLUSOBJS:.o=.thr.lo}

FAKELIBTHROBJS:=${FAKELIBOBJS:.o=.thr.o}
FAKELIBLOBJS:=${FAKELIBOBJS:.o=.lo}
FAKELIBTHRLOBJS:=${FAKELIBOBJS:.o=.thr.lo}

LIB:=libglite_lb_client_${nothrflavour}.la
THRLIB:=libglite_lb_client_${thrflavour}.la
FAKELIB:=libglite_lb_client_fake_${nothrflavour}.la
FAKETHRLIB:=libglite_lb_client_fake_${thrflavour}.la

PLUSLIB:=libglite_lb_clientpp_${nothrflavour}.la
THRPLUSLIB:=libglite_lb_clientpp_${thrflavour}.la

TOOLS:=${LB_PERF_TOOLS}
EXAMPLES_SRC:=log_usertag_proxy.c job_log.c job_reg.c feed_shark.c notify.c query_ext.c query_seq_code.c stats.c abort_job.c change_acl.c stresslog.c flood_proxy.c dagids.c stress_context.c parse_eventsfile.c
EXAMPLES:=${EXAMPLES_SRC:.c=}

# TODO: migrate them here from branch_RC31_3
# EXAMPLES_PLUS_SRC:=indexed_attrs_plus.cpp job_status_plus.cpp query_events_plus.cpp listener.cpp user_jobs_plus.cpp job_log_plus.cpp notify_plus.cpp
# EXAMPLES_PLUS:=${EXAMPLES_PLUS_SRC:.cpp=}

EXAMPLES_CL_SRC:=user_jobs.c job_status.c 
EXAMPLES_CL:=${EXAMPLES_CL_SRC:.c=}

EXAMPLES_CL_THR_SRC:=user_jobs_threaded.c
EXAMPLES_CL_THR:=${EXAMPLES_CL_THR_SRC:.c=}

FAKE_EXAMPLES:=job_log_fake

MAN_GZ:=glite-lb-logevent.1.gz
MAN = $(MAN_GZ:.gz=)

PLUS_EXTRA_LIB:=-lglite_jobid


# version_info=-version-info `echo ${version} | cut -d. -f1,2 | tr . :`

# counted minor versions: 
offset=0

version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

ifdef LB_STANDALONE
compile all: generate ${LIB} ${THRLIB} ${TOOLS} logevent examples ${MAN_GZ}
else
compile all: check_version generate ${LIB} ${THRLIB} ${PLUSLIB} ${THRPLUSLIB} ${TOOLS} logevent examples ${MAN_GZ}
endif

generate: ${GEN_HDRS}
	rm -vf ${globalprefix} ${lbprefix} ${HDRS}
	ln -vs . ${globalprefix}
	ln -vs . ${lbprefix}
	for i in ${HDRS}; do ln -vs ../interface/$$i $$i ; done

default: all

${LIB}: ${LIBOBJS}
	${LINK} ${version_info} -o $@ ${LIBLOBJS} -rpath ${PREFIX}/lib \
			${COMMON_LIB} ${TRIO_LIB} \
			-lglite_security_gss_${nothrflavour}

${THRLIB}: ${LIBTHROBJS}
	${LINK} ${version_info} -o $@ ${LIBTHRLOBJS} -rpath ${PREFIX}/lib \
			${COMMON_LIB_THR} ${TRIO_LIB} \
			-lglite_security_gss_${thrflavour}

${FAKELIB}: ${FAKELIBOBJS}
	${LINK} ${version_info} -o $@ ${FAKELIBLOBJS} -rpath ${PREFIX}/lib \
			${COMMON_LIB} \
			-lglite_security_gss_${nothrflavour}

${FAKETHRLIB}: ${FAKELIBTHROBJS}
	${LINK} ${version_info} -o $@ ${FAKELIBTHRLOBJS} -rpath ${PREFIX}/lib \
			${COMMON_LIB_THR} \
			-lglite_security_gss_${thrflavour}

${PLUSLIB}: ${PLUSOBJS} ${LIB}
	${LINKXX} ${version_info} -o $@ ${PLUSLOBJS} -rpath ${PREFIX}/lib ${LIB} ${PLUS_EXTRA_LIB}

${THRPLUSLIB}: ${PLUSTHROBJS} ${THRLIB}
	${LINKXX} ${version_info} -o $@ ${PLUSTHRLOBJS} -rpath ${PREFIX}/lib ${THRLIB} ${PLUS_EXTRA_LIB}

logevent: logevent.o args.o
	${LINK} -o $@ logevent.o args.o ${LIB} ${EXT_LIB} 

${TOOLS} ${EXAMPLES}: %: %.o
	${LINK} -o $@ $< ${LIB} ${EXT_LIB} 

${EXAMPLES_CL}: %: %.o
	${LINK} -o $@ $< ${LIB} ${COMMON_LIB} ${EXT_LIB} 

${EXAMPLES_CL_THR}: %: %.o
	${LINK} -o $@ $< ${THRLIB} ${COMMON_LIB_THR} ${EXT_LIB} 

${FAKE_EXAMPLES}: %: %.o ${FAKELIB}
	${LINK} -o $@ $< ${FAKELIB} ${TEST_LIBS} ${EXT_LIB} 

${TOOLS}: ${LIB}

${FAKE_EXAMPLES:=.o}: %.o: %.cpp
	${COMPILE} ${TEST_INC} -c $< -o $@

${PLUSOBJS}: %.o: %.cpp
	${CXXCOMPILE} -c $<

${PLUSTHROBJS}: %.thr.o: %.cpp
	${CXXCOMPILE} -o $@ -c $<

${EXAMPLES_PLUS}: ${PLUSLIB}

${EXAMPLES_PLUS}: %: %.o
	${LINKXX} -o $@ $< ${PLUSLIB}

${LIBOBJS} ${FAKELIBOBJS}: %.o: %.c
	${COMPILE} -c $<

${LIBTHROBJS} ${FAKELIBTHROBJS}: %.thr.o: %.c
	${COMPILE} -o $@ -c $<

logevent_fake.o: logevent.c
	${COMPILE} -c $< -o $@

${MAN_GZ}: ${MAN}
	cp $? .
	gzip $(notdir $?)


perftest_logjobs.o: perftest_logjobs.c
	${CC} ${CFLAGS} -DLB_PERF_DROP -c $<

# catches $TOOLS and logevent compilation
%.o: %.c 
	${CC} ${CFLAGS} -c $<

%.h: %.h.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

%.o: %.cpp
	${CXX} ${CFLAGS} ${GLOBUSINC} -c $<

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

%.cpp: %.cpp.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

%.sh: %.l gen_begin gen_sample_job
	rm -f $@
	$(GENSAM) $< >$@ || rm -f $@
	chmod -w,+x $@ > /dev/null

examples: ${EXAMPLES} ${EXAMPLES_PLUS} ${EXAMPLES_CL} ${EXAMPLES_CL_THR} ${sh_PROGS}

fake: ${FAKE_EXAMPLES}

check: compile 
# shut up check.producer

check.producer: producer_test
	./producer_test

producer_test: producer_test.o prod_proto_test.o
	${LINKXX} -o $@ ${LIB} ${TEST_LIBS} $+ ${EXT_LIB}

producer_test.o: %.o: %.cpp
	${CXX} -c ${CXXFLAGS} ${TEST_INC} $<

man: ${MAN_GZ}

stage:  compile ${FAKELIB} ${FAKETHRLIB} 
	$(MAKE) install PREFIX=${stagedir}
	${INSTALL} -m 644 ${FAKELIB} ${FAKETHRLIB} ${stagedir}/lib

# TODO: restructuring - cleanup the documentation
doc: generate
	cp ${top_srcdir}/doc/*.dox .
	echo "PROJECT_NUMBER = ${version}" >> C.dox
	echo "PROJECT_NUMBER = ${version}" >> CPP.dox
	doxygen C.dox
	doxygen CPP.dox

install: doc
	mkdir -p ${PREFIX}/bin
	mkdir -p ${PREFIX}/sbin
	mkdir -p ${PREFIX}/include/${globalprefix}/${lbprefix}
	mkdir -p ${PREFIX}/lib
	mkdir -p ${PREFIX}/share/doc/${package}-${version}/api
	mkdir -p ${PREFIX}/share/doc/${package}-${version}/examples
	mkdir -p ${PREFIX}/share/man/man1
	mkdir -p ${PREFIX}/examples
ifdef LB_STANDALONE
	${INSTALL} -m 644 ${LIB} ${THRLIB} ${PREFIX}/lib
else
	${INSTALL} -m 644 ${LIB} ${THRLIB} ${PLUSLIB} ${THRPLUSLIB} ${PREFIX}/lib
	${INSTALL} -m 644 ${LIB} ${THRLIB} ${PREFIX}/lib
endif
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${top_srcdir}/doc/README-fake ${top_srcdir}/doc/README-notify ${PREFIX}/share/doc/${package}-${version}
	(cd ${top_srcdir}/interface && ${INSTALL} -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${lbprefix}) 
	${INSTALL} -m 644 ${GEN_HDRS} ${PREFIX}/include/${globalprefix}/${lbprefix} 
ifndef LB_STANDALONE
	cp -r C CPP ${PREFIX}/share/doc/${package}-${version}
#       cp -r ${top_srcdir}/doc/api/{Makefile,api.tex,fig} ${PREFIX}/share/doc/${package}-${version}/api
endif
	for p in logevent; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/bin/glite-lb-$$p"; \
	done
	for p in ${TOOLS} ; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/sbin/glite-lb-$$p"; \
	done
# TODO: restructuring - do we really need binaries of the examples to be installed?
	for p in ${EXAMPLES} ${EXAMPLES_PLUS} ${EXAMPLES_CL} ${EXAMPLES_CL_THR} ${sh_PROGS} ; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/examples/glite-lb-$$p"; \
	done
	for p in ${EXAMPLES_SRC} ${EXAMPLES_PLUS_SRC} ${EXAMPLES_CL_SRC} ${EXAMPLES_CL_THR_SRC} ; do \
		${INSTALL} -m 644 "${top_srcdir}/examples/$$p" "${PREFIX}/share/doc/${package}-${version}/examples/"; \
	done
	${INSTALL} -m 644 ${top_srcdir}/examples/Makefile "${PREFIX}/share/doc/${package}-${version}/examples/"
	${INSTALL} -m 644 ${top_srcdir}/examples/README.examples "${PREFIX}/share/doc/${package}-${version}/examples/"
	${INSTALL} -m 755 ${top_srcdir}/src/export.sh "${PREFIX}/sbin/glite-lb-export.sh"
	${INSTALL} -m 644 ${MAN_GZ} ${PREFIX}/share/man/man1

clean:
	rm -rvf *.o *.lo .libs lib* *.c *.cpp *.h *.dox producer_test C/ CPP/
	rm -rvf ${LIB} ${THRLIB} ${TOOLS} logevent ${PLUSLIB} ${THRPLUSLIB} ${MAN_GZ}
	rm -rvf ${EXAMPLES} ${EXAMPLES_CL} ${EXAMPLES_CL_THR} ${sh_PROGS}
	rm -vf ${globalprefix} ${lbprefix}
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

check_version:
	${CHECK_VERSION} ${stagedir}/include/glite/lb/common_version.h

JobStatus.h: StatusAttrNames.pl

.PHONY: default all compile examples fake check stage install clean check_version
