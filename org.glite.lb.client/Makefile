# Default values
top_srcdir=.
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-client
version=0.0.0

globus_prefix=/opt/globus
nothrflavour=gcc32
thrflavour=gcc32pthr

CC:=gcc
CXX:=g++

-include Makefile.inc
-include ../project/version.properties

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/interface:${top_srcdir}/test:${top_srcdir}/examples:${top_srcdir}/doc
AT3=${stagedir}/sbin/glite-lb-at3
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
	-I. \
	-I${top_srcdir}/src -I${top_srcdir}/interface \
	-I${stagedir}/include \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE ${LB_STANDALONE_FLAGS} ${LB_PERF_FLAGS}

CXXFLAGS:=${CFLAGS}

EXT_LIB:= 

TEST_LIBS:=-L${cppunit_prefix}/lib -lcppunit
TEST_INC:=-I${cppunit_prefix}/include


LDFLAGS:=-L${stagedir}/lib \
	${COVERAGE_FLAGS}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
CXXCOMPILE:=libtool --mode=compile ${CXX} ${CXXFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
INSTALL:=libtool --mode=install install

LIBOBJS:=connection.o consumer.o notification.o prod_proto.o \
	producer.o uiwrap.o statistics.o
FAKELIBOBJS:=consumer_fake.o producer_fake.o

PLUSOBJS:=Event.o Job.o JobStatus.o Notification.o ServerConnection.o

HDRS:=consumer.h notification.h query.h statistics.h \
	Job.h Notification.h ServerConnection.h 
FAKE_HDRS:=consumer_fake.h producer_fake.h
GEN_HDRS:=JobStatus.h producer.h interface_version.h

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
EXAMPLES:=log_usertag_proxy job_log job_reg feed_shark notify query_ext query_seq_code stats abort_job change_acl stresslog flood_proxy dagids stress_context parse_eventsfile

EXAMPLES_CL=user_jobs job_status 
EXAMPLES_CL_THR=user_jobs_threaded
FAKE_EXAMPLES:=job_log_fake

MAN_GZ:=glite-lb-logevent.1.gz
MAN = $(MAN_GZ:.gz=)

# version_info=-version-info `echo ${version} | cut -d. -f1,2 | tr . :`

# counted minor versions: 
offset=0

version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

ifdef LB_STANDALONE
compile all: generate ${LIB} ${THRLIB} ${TOOLS} logevent examples ${MAN_GZ}
else
# FIXME:
#compile all: generate ${LIB} ${THRLIB} ${PLUSLIB} ${THRPLUSLIB} ${TOOLS} logevent examples ${MAN_GZ}
compile all: generate ${LIB} ${THRLIB} ${TOOLS} logevent examples ${MAN_GZ}
endif

generate: ${GEN_HDRS}

interface_version.h: ${top_srcdir}/project/version.properties
	echo "#define GLITE_LB_CLIENT_INTERFACE \"${version}\"" >$@ 

${LIB}: ${LIBOBJS}
	${LINK} ${version_info} -o $@ ${LIBLOBJS} -rpath ${PREFIX}/lib \
			${COMMON_LIB} \
			-lglite_security_gss_${nothrflavour}

${THRLIB}: ${LIBTHROBJS}
	${LINK} ${version_info} -o $@ ${LIBTHRLOBJS} -rpath ${PREFIX}/lib \
			${COMMON_LIB_THR} \
			-lglite_security_gss_${thrflavour}

${FAKELIB}: ${FAKELIBOBJS}
	${LINK} ${version_info} -o $@ ${FAKELIBLOBJS} -rpath ${PREFIX}/lib \
			${COMMON_LIB} \
			-lglite_security_gss_${nothrflavour}

${FAKETHRLIB}: ${FAKELIBTHROBJS}
	${LINK} ${version_info} -o $@ ${FAKELIBTHRLOBJS} -rpath ${PREFIX}/lib \
			${COMMON_LIB_THR} \
			-lglite_security_gss_${thrflavour}

${PLUSLIB}: ${PLUSOBJS}
	${LINK} ${version_info} -o $@ ${PLUSLOBJS} -rpath ${PREFIX}/lib ${LIB}

${THRPLUSLIB}: ${PLUSTHROBJS}
	${LINK} ${version_info} -o $@ ${PLUSTHRLOBJS} -rpath ${PREFIX}/lib ${THRLIB}

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

${LIBOBJS} ${FAKELIBOBJS}: %.o: %.c
	${COMPILE} ${GLOBUSINC} -c $<

${LIBTHROBJS} ${FAKELIBTHROBJS}: %.thr.o: %.c
	${COMPILE} ${GLOBUSTHRINC} -o $@ -c $<

logevent_fake.o: logevent.c
	${COMPILE} ${GLOBUSINC} -c $< -o $@

${MAN_GZ}: ${MAN}
	cp $? .
	gzip $(notdir $?)


perftest_logjobs.o: perftest_logjobs.c
	${CC} ${CFLAGS} -DLB_PERF_DROP ${GLOBUSINC} -c $<

# catches $TOOLS and logevent compilation
%.o: %.c 
	${CC} ${CFLAGS} ${GLOBUSINC} -c $<

%.h: %.h.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

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

man: ${MAN_GZ}

stage:  compile ${FAKELIB} ${FAKETHRLIB} 
	$(MAKE) install PREFIX=${stagedir}
	${INSTALL} -m 644 ${FAKELIB} ${FAKETHRLIB} ${stagedir}/lib

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
	mkdir -p ${PREFIX}/share/man/man1
ifdef LB_STANDALONE
	${INSTALL} -m 644 ${LIB} ${THRLIB} ${PREFIX}/lib
else
# FIXME:
#	${INSTALL} -m 644 ${LIB} ${THRLIB} ${PLUSLIB} ${THRPLUSLIB} ${PREFIX}/lib
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

clean:
	rm -rvf *.o *.lo .libs lib* *.c *.cpp *.h *.dox producer_test C/ CPP/
	rm -rvf ${LIB} ${THRLIB} ${TOOLS} logevent ${PLUSLIB} ${THRPLUSLIB} ${MAN_GZ}
	rm -rvf ${EXAMPLES} ${EXAMPLES_CL} ${EXAMPLES_CL_THR} ${sh_PROGS}
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

check_version:
	${CHECK_VERSION} ${stagedir}/include/glite/lb/interface_version.h

.PHONY: default all compile examples fake check stage dist distsrc distbin install clean check_version
