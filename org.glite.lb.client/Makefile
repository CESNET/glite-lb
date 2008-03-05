# Default values
top_srcdir=.
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-client
version=0.0.0

glite_location=/opt/glite
globus_prefix=/opt/globus
nothrflavour=gcc32
thrflavour=gcc32pthr


CC:=gcc
CXX:=g++

-include Makefile.inc
-include ../project/version.properties

PREFIX?=${glite_location}

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/examples:${top_srcdir}/doc
AT3=perl -I${top_srcdir}/project ${top_srcdir}/project/at3
GENSAM=${top_srcdir}/examples/gen_sample_job
CHECK_VERSION:=VERSION=${version} VERSION_AHEAD=0 perl ${top_srcdir}/project/check_version.pl

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
	shallow_resub_complex.l shallow_resub_simple.l shallow_resub_simple2.l \
	resubmission.l resubmitted.l


sh_PROGS = $(l_SRC:.l=.sh)

GLOBUSINC:= -I${globus_prefix}/include/${nothrflavour}
GLOBUSTHRINC:= -I${globus_prefix}/include/${thrflavour}

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

CFLAGS:=${DEBUG} \
	-I${top_srcdir}/src -I${top_srcdir}/interface \
	-I${stagedir}/include \
	-I${glite_location}/include \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE ${LB_STANDALONE_FLAGS} ${LB_PERF_FLAGS}

CXXFLAGS:=${CFLAGS}

archlib:=lib
LDFLAGS:=-L${stagedir}/lib
host_cpu:=${shell uname -m}
ifeq (${host_cpu},x86_64)
    archlib:=lib64
    LDFLAGS:=-L${stagedir}/lib64 -L${stagedir}/lib ${COVERAGE_FLAGS}
endif

CJOBID_LIB:= -lglite_wmsutils_cjobid

TEST_LIBS:=-L${cppunit_prefix}/lib -lcppunit
TEST_INC:=-I${cppunit_prefix}/include


COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
CXXCOMPILE:=libtool --mode=compile ${CXX} ${CXXFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
INSTALL:=libtool --mode=install install

LIBOBJS:=connection.o consumer.o notification.o prod_proto.o \
	producer.o uiwrap.o statistics.o
FAKELIBOBJS:=consumer_fake.o producer_fake.o

PLUSOBJS:=Event.o Job.o JobStatus.o Notification.o ServerConnection.o
PUB_HDRS:=CountRef.h Event.h JobJobStatus.h Notification.h ServerConnection.h \
	LoggingExceptions.h

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

TOOLS:=dump load purge lb_dump_exporter ${LB_PERF_TOOLS}
EXAMPLES:=log_usertag_proxy job_log job_reg feed_shark notify query_ext query_seq_code stats abort_job change_acl stresslog flood_proxy dagids stress_context parse_eventsfile

EXAMPLES_CL=user_jobs job_status multiple_user_jobs
EXAMPLES_CL_THR=user_jobs_threaded
FAKE_EXAMPLES:=job_log_fake

EXAMPLES_PLUS= job_status_plus
# EXAMPLES_PLUS= indexed_attrs_plus.cpp  job_status_plus.cpp  query_events_plus.cpp listener.cpp         user_jobs_plus.cpp job_log_plus.cpp        notify_plus.cpp


MAN_GZ:=glite-lb-logevent.1.gz
MAN8_GZ:=glite-lb-dump.8.gz glite-lb-load.8.gz glite-lb-purge.8.gz
MAN = $(MAN_GZ:.gz=)
MAN8 = $(MAN8_GZ:.gz=)

PLUS_EXTRA_LIB:=-lglite_wmsutils_jobid -lglite_wmsutils_exception


# version_info=-version-info `echo ${version} | cut -d. -f1,2 | tr . :`

# counted minor versions: 
offset=0

version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

default: all

${LIB}: ${LIBOBJS}
	${LINK} ${version_info} -o $@ ${LIBLOBJS} -rpath ${PREFIX}/lib \
			${COMMON_LIB} \
			${CJOBID_LIB} \
			-lglite_security_gss_${nothrflavour}

${THRLIB}: ${LIBTHROBJS}
	${LINK} ${version_info} -o $@ ${LIBTHRLOBJS} -rpath ${PREFIX}/lib \
			${COMMON_LIB_THR} \
			${CJOBID_LIB} \
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
	${COMPILE} ${GLOBUSINC} ${TEST_INC} -c $< -o $@

${PLUSOBJS}: %.o: %.cpp
	${CXXCOMPILE} ${GLOBUSINC} -c $<

${PLUSTHROBJS}: %.thr.o: %.cpp
	${CXXCOMPILE} ${GLOBUSTHRINC} -o $@ -c $<


${EXAMPLES_PLUS}: ${PLUSLIB}

${EXAMPLES_PLUS}: %: %.o
	${LINKXX} -o $@ $< ${PLUSLIB}

${LIBOBJS} ${FAKELIBOBJS}: %.o: %.c
	${COMPILE} ${GLOBUSINC} -c $<

${LIBTHROBJS} ${FAKELIBTHROBJS}: %.thr.o: %.c
	${COMPILE} ${GLOBUSTHRINC} -o $@ -c $<

logevent_fake.o: logevent.c
	${COMPILE} ${GLOBUSINC} -c $< -o $@

${MAN_GZ}: ${MAN}
	cp $? .
	gzip $(notdir $?)

${MAN8_GZ}: ${MAN8}
	cp $? .
	gzip $(notdir $?)

perftest_logjobs.o: perftest_logjobs.c
	${CC} ${CFLAGS} -DLB_PERF_DROP ${GLOBUSINC} -c $<

# catches $TOOLS and logevent compilation
%.o: %.c 
	${CC} ${CFLAGS} ${GLOBUSINC} -c $<

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

default: all


ifdef LB_STANDALONE
compile all: ${LIB} ${THRLIB} ${TOOLS} logevent examples ${MAN_GZ} ${MAN8_GZ}
else
compile all: check_version ${LIB} ${THRLIB} ${TOOLS} logevent ${PLUSLIB} ${THRPLUSLIB} examples ${MAN_GZ} ${MAN8_GZ}
endif

examples: ${EXAMPLES} ${EXAMPLES_CL} ${EXAMPLES_CL_THR} ${sh_PROGS}

fake: ${FAKE_EXAMPLES}

check: compile 
# shut up check.producer

check.producer: producer_test
	./producer_test

producer_test: producer_test.o prod_proto_test.o
	${LINKXX} -o $@ ${LIB} ${TEST_LIBS} $+ ${EXT_LIB}

producer_test.o: %.o: %.cpp
	${CXX} -c ${CXXFLAGS} ${TEST_INC} ${GLOBUSINC} $<

man: ${MAN_GZ} ${MAN8_GZ}

stage:  compile ${FAKELIB} ${FAKETHRLIB} 
	$(MAKE) install PREFIX=${stagedir}
	${INSTALL} -m 644 ${FAKELIB} ${FAKETHRLIB} ${stagedir}/lib

Event.cpp: EventAttrNames.pl

JobStatus.cpp: StatusAttrNames.pl

EventAttrNames.pl StatusAttrNames.pl:
	ln -s ${stagedir}/share/${lbprefix}/at3/$@ $@

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
	mkdir -p ${PREFIX}/bin
	mkdir -p ${PREFIX}/sbin
	mkdir -p ${PREFIX}/lib
	mkdir -p ${PREFIX}/share/doc/${package}-${version}
	mkdir -p ${PREFIX}/share/man/man1
	mkdir -p ${PREFIX}/share/man/man8
ifdef LB_STANDALONE
	${INSTALL} -m 644 ${LIB} ${THRLIB} ${PREFIX}/lib
else
	${INSTALL} -m 644 ${LIB} ${THRLIB} ${PLUSLIB} ${THRPLUSLIB} ${PREFIX}/lib
endif
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${top_srcdir}/doc/README-fake ${top_srcdir}/doc/README-notify ${PREFIX}/share/doc/${package}-${version}
	mkdir -p ${PREFIX}/examples
	for p in logevent; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/bin/glite-lb-$$p"; \
	done
	for p in ${TOOLS} ; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/sbin/glite-lb-$$p"; \
	done
	for p in ${EXAMPLES} ${EXAMPLES_CL} ${EXAMPLES_CL_THR} ${sh_PROGS} ; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/examples/glite-lb-$$p"; \
	done
	${INSTALL} -m 755 ${top_srcdir}/src/export.sh "${PREFIX}/sbin/glite-lb-export.sh"
	${INSTALL} -m 644 ${MAN_GZ} ${PREFIX}/share/man/man1
	${INSTALL} -m 644 ${MAN8_GZ} ${PREFIX}/share/man/man8

clean:
	rm -rf *.o *.lo .libs lib* *.c *.cpp producer_test
	rm -rf ${LIB} ${THRLIB} ${TOOLS} logevent ${PLUSLIB} ${THRPLUSLIB} ${MAN_GZ} ${MAN8_GZ}
	rm -rf ${EXAMPLES} ${EXAMPLES_CL} ${EXAMPLES_CL_THR} ${sh_PROGS}
	rm -f EventAttrNames.pl StatusAttrNames.pl


check_version:
	${CHECK_VERSION} ${stagedir}/include/glite/lb/interface_version.h

.PHONY: default all compile examples fake check stage dist distsrc distbin install clean check_version
