# Default values
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-client
version=0.0.0
PREFIX=/opt/glite

glite_location=/opt/glite
globus_prefix=/opt/globus
nothrflavour=gcc32
thrflavour=gcc32pthr
expat_prefix=/opt/expat
ares_prefix=/opt/ares

-include Makefile.inc

VPATH=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/examples
AT3=perl -I${top_srcdir}/project ${top_srcdir}/project/at3
GENSAM=${top_srcdir}/examples/gen_sample_job

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
	cancelled.l 

sh_PROGS = $(l_SRC:.l=.sh)


DEBUG:=-g -O0 -Wall

GLOBUSINC:= -I${globus_prefix}/include/${nothrflavour}
GLOBUSTHRINC:= -I${globus_prefix}/include/${thrflavour}

CFLAGS:=${DEBUG} \
	-I${top_srcdir}/src -I${top_srcdir}/interface \
	-I${stagedir}/include \
	-I${glite_location}/include \
	-I${expat_prefix}/include \
	-I${ares_prefix}/include \
	${COVERAGE_FLAGS} 


CXXFLAGS:=${CFLAGS}

GLOBUS_LIBS:=-L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lglobus_gssapi_gsi_${nothrflavour} \

GLOBUS_THRLIBS:=-L${globus_prefix}/lib \
	-lglobus_common_${thrflavour} \
	-lglobus_gssapi_gsi_${thrflavour}

EXPAT_LIBS:=-L${expat_prefix}/lib \
	-lexpat

ARES_LIBS:=-L${ares_prefix}/lib \
	-lares

EXT_LIB:= ${EXPAT_LIBS} \
    ${ARES_LIBS}

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
	producer.o uiwrap.o
TESTLIBOBJS:=consumer_fake.o producer_fake.o

PLUSOBJS:=Event.o Job.o JobStatus.o Notification.o ServerConnection.o
PUB_HDRS:=CountRef.h Event.h JobJobStatus.h Notification.h ServerConnection.h \
	LoggingExceptions.h

LIBTHROBJS:=${LIBOBJS:.o=.thr.o}
LIBLOBJS:=${LIBOBJS:.o=.lo}
LIBTHRLOBJS:=${LIBOBJS:.o=.thr.lo}

PLUSTHROBJS:=${PLUSOBJS:.o=.thr.o}
PLUSLOBJS:=${PLUSOBJS:.o=.lo}
PLUSTHRLOBJS:=${PLUSOBJS:.o=.thr.lo}

TESTLIBTHROBJS:=${TESTLIBOBJS:.o=.thr.o}
TESTLIBLOBJS:=${TESTLIBOBJS:.o=.lo}
TESTLIBTHRLOBJS:=${TESTLIBOBJS:.o=.thr.lo}

LIB:=libglite_lb_client_${nothrflavour}.la
THRLIB:=libglite_lb_client_${thrflavour}.la
TESTLIB:=libglite_lb_client_test_${nothrflavour}.la
TESTTHRLIB:=libglite_lb_client_test_${thrflavour}.la

PLUSLIB:=libglite_lb_clientpp_${nothrflavour}.la
THRPLUSLIB:=libglite_lb_clientpp_${thrflavour}.la

TOOLS:=dump load purge
#EXAMPLES:=log_usertag_proxy job_reg feed_shark notify
EXAMPLES:=job_reg feed_shark notify
FAKE_EXAMPLES:=job_log_fake job_reg_fake
FAKE_EXAMPLES_SOURCES:=job_log.c job_reg.c

version_info=-version-info `echo ${version} | cut -d. -f1,2 | tr . :`

${LIB}: ${LIBOBJS}
	${LINK} ${version_info} -o $@ ${LIBLOBJS} -rpath ${glite_location}/lib -lglite_lb_common_${nothrflavour}

${THRLIB}: ${LIBTHROBJS}
	${LINK} ${version_info} -o $@ ${LIBTHRLOBJS} -rpath ${glite_location}/lib -lglite_lb_common_${thrflavour}

${TESTLIB}: ${TESTLIBOBJS}
	${LINK} ${version_info} -o $@ ${TESTLIBLOBJS} -rpath ${glite_location}/lib -lglite_lb_common_${nothrflavour}

${TESTTHRLIB}: ${TESTLIBTHROBJS}
	${LINK} ${version_info} -o $@ ${TESTLIBTHRLOBJS} -rpath ${glite_location}/lib -lglite_lb_common_${thrflavour}

${PLUSLIB}: ${PLUSOBJS}
	${LINK} ${version_info} -o $@ ${PLUSLOBJS} -rpath ${glite_location}/lib ${LIB}

${THRPLUSLIB}: ${PLUSTHROBJS}
	${LINK} ${version_info} -o $@ ${PLUSTHRLOBJS} -rpath ${glite_location}/lib ${THRLIB}

logevent: logevent.o args.o
	${LINK} -o $@ logevent.o args.o ${LIB} ${EXT_LIB} ${GLOBUS_LIBS}

log_usertag_proxy: log_usertag_proxy.o
	${LINK} -o $@ log_usertag_proxy.o ${LIB} ${EXT_LIB} ${GLOBUS_LIBS}

logevent_fake: logevent_fake.o args.o
	${LINK} -o $@ logevent_fake.o args.o ${TESTLIB} ${EXT_LIB} ${GLOBUS_LIBS}

${TOOLS} ${EXAMPLES}: %: %.o
	${LINK} -o $@ $< ${LIB} ${EXT_LIB} ${GLOBUS_LIBS}

${FAKE_EXAMPLES}: %: %.o ${TESTLIB}
	${LINK} -o $@ $< ${TESTLIB} ${EXT_LIB} ${GLOBUS_LIBS}

${TOOLS}: ${LIB}

${FAKE_EXAMPLES:=.o}: ${FAKE_EXAMPLES_SOURCES}
	${COMPILE} ${GLOBUSINC} -DUSE_CALLBACKS -c $< -o $@

${PLUSOBJS}: %.o: %.cpp
	${CXXCOMPILE} ${GLOBUSINC} -c $<

${PLUSTHROBJS}: %.thr.o: %.cpp
	${CXXCOMPILE} ${GLOBUSTHRINC} -o $@ -c $<

${LIBOBJS} ${TESTLIBOBJS}: %.o: %.c
	${COMPILE} ${GLOBUSINC} -c $<

${LIBTHROBJS} ${TESTLIBTHROBJS}: %.thr.o: %.c
	${COMPILE} ${GLOBUSTHRINC} -o $@ -c $<

logevent_fake.o: logevent.c
	${COMPILE} ${GLOBUSINC} -c $< -o $@

# catches $TOOLS and logevent compilation
%.o: %.c 
	${CC} ${CFLAGS} ${GLOBUSINC} -c $<

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


compile all: check_version ${LIB} ${THRLIB} ${TOOLS} logevent ${PLUSLIB} ${THRPLUSLIB} ${TESTLIB} ${TESTTHRLIB} examples

examples: ${EXAMPLES} ${FAKE_EXAMPLES} ${sh_PROGS} logevent_fake

check: compile check.producer

check.producer: producer_test
	./producer_test

producer_test: producer_test.o prod_proto_test.o
	${LINKXX} -o $@ ${LIB} ${TEST_LIBS} $+ ${EXT_LIB} ${GLOBUS_LIBS}

producer_test.o: %.o: %.cpp
	${CXX} -c ${CXXFLAGS} ${TEST_INC} $<
	
stage: compile
	$(MAKE) install PREFIX=${stagedir}
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
	mkdir -p ${PREFIX}/lib
	mkdir -p ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${LIB} ${THRLIB} ${PLUSLIB} ${THRPLUSLIB} ${TESTLIB} ${TESTTHRLIB} ${PREFIX}/lib
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	mkdir -p ${PREFIX}/examples
	for p in ${TOOLS} logevent; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/bin/glite-lb-$$p"; \
	done
	for p in ${TOOLS} logevent ${EXAMPLES} ${sh_PROGS}; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/examples/glite-lb-$$p"; \
	done

clean:

check_version:
	@perl -ne '/#define GLITE_LB_CLIENT_INTERFACE "(\d+)\.\d+\.\d+"/; \
		$$iface=$$1; \
		$$_="${version}"; /(\d+)\.\d+\.\d+/; \
		if ($$iface != $$1) { \
			print "error: Major version of the interface ($$iface) does not match implementation ($$1)\n" ;\
			exit 1;  \
		} \
		' ${stagedir}/include/glite/lb/interface_version.h
