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

CC:=gcc
CXX:=g++

-include Makefile.inc


VPATH=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/examples:${top_srcdir}/doc
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
	cancelled.l \
	shallow_resub_complex.l shallow_resub_simple.l shallow_resub_simple2.l \
	resubmission.l resubmitted.l


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
	-I${mysql_prefix}/include -I${mysql_prefix}/include/mysql \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE


CXXFLAGS:=${CFLAGS}

ifneq (${mysql_prefix},/usr)
	ifeq ($(shell echo ${mysql_version} | cut -d. -f1,2),4.1)
		mysqlib := -L${mysql_prefix}/lib/mysql
	else
		mysqlib := -L${mysql_prefix}/lib
	endif
endif

ifneq (${expat_prefix},/usr)
	EXPAT_LIBS:=-L${expat_prefix}/lib
endif
EXPAT_LIBS := ${EXPAT_LIBS} -lexpat

ARES_LIBS:=-L${ares_prefix}/lib \
	-lares

EXT_LIB:= ${mysqlib} -lmysqlclient -lz\
	${EXPAT_LIBS} \
	${ARES_LIBS}

TEST_LIBS:=-L${cppunit_prefix}/lib -lcppunit
TEST_INC:=-I${cppunit_prefix}/include

COMMON_LIB=-lglite_lb_common_${nothrflavour}

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

TOOLS:=dump load purge lb_dump_exporter
EXAMPLES:=log_usertag_proxy job_log job_reg feed_shark notify query_ext query_seq_code stats abort_job change_acl lbmon stresslog

EXAMPLES_CL=user_jobs job_status 
FAKE_EXAMPLES:=job_log_fake

MAN_GZ:=glite-lb-logevent.1.gz
MAN = $(MAN_GZ:.gz=)

# version_info=-version-info `echo ${version} | cut -d. -f1,2 | tr . :`

# counted minor versions: 2.1
offset=1

version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

${LIB}: ${LIBOBJS}
	${LINK} ${version_info} -o $@ ${LIBLOBJS} -rpath ${glite_location}/lib \
			-lglite_lb_common_${nothrflavour} \
			-lglite_security_gss_${nothrflavour}

${THRLIB}: ${LIBTHROBJS}
	${LINK} ${version_info} -o $@ ${LIBTHRLOBJS} -rpath ${glite_location}/lib \
			-lglite_lb_common_${thrflavour} \
			-lglite_security_gss_${thrflavour}

${FAKELIB}: ${FAKELIBOBJS}
	${LINK} ${version_info} -o $@ ${FAKELIBLOBJS} -rpath ${glite_location}/lib \
			-lglite_lb_common_${nothrflavour} \
			-lglite_security_gss_${nothrflavour}

${FAKETHRLIB}: ${FAKELIBTHROBJS}
	${LINK} ${version_info} -o $@ ${FAKELIBTHRLOBJS} -rpath ${glite_location}/lib \
			-lglite_lb_common_${thrflavour} \
			-lglite_security_gss_${thrflavour}

${PLUSLIB}: ${PLUSOBJS}
	${LINK} ${version_info} -o $@ ${PLUSLOBJS} -rpath ${glite_location}/lib ${LIB}

${THRPLUSLIB}: ${PLUSTHROBJS}
	${LINK} ${version_info} -o $@ ${PLUSTHRLOBJS} -rpath ${glite_location}/lib ${THRLIB}

logevent: logevent.o args.o
	${LINK} -o $@ logevent.o args.o ${LIB} ${EXT_LIB} 

lbmon-db: lbmon-db.o
	${LINK} -o $@ lbmon-db.o -lmysqlclient -lglite_lb_bkserver ${LIB} ${EXT_LIB} 

${TOOLS} ${EXAMPLES}: %: %.o
	${LINK} -o $@ $< ${LIB} ${EXT_LIB} 

${EXAMPLES_CL}: %: %.o
	${LINK} -o $@ $< ${LIB} ${COMMON_LIB} ${EXT_LIB} 

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


compile all: check_version ${LIB} ${THRLIB} ${TOOLS} logevent lbmon-db ${PLUSLIB} ${THRPLUSLIB} examples ${MAN_GZ}

examples: ${EXAMPLES} ${EXAMPLES_CL} ${sh_PROGS}

fake: ${FAKE_EXAMPLES}

check: compile check.producer

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
	
install:
	mkdir -p ${PREFIX}/bin
	mkdir -p ${PREFIX}/sbin
	mkdir -p ${PREFIX}/lib
	mkdir -p ${PREFIX}/share/doc/${package}-${version}
	mkdir -p ${PREFIX}/share/man/man1
	${INSTALL} -m 644 ${LIB} ${THRLIB} ${PLUSLIB} ${THRPLUSLIB} ${PREFIX}/lib
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${top_srcdir}/doc/README-fake ${top_srcdir}/doc/README-notify ${PREFIX}/share/doc/${package}-${version}
	mkdir -p ${PREFIX}/examples
	for p in logevent ; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/bin/glite-lb-$$p"; \
	done
	for p in ${TOOLS} ; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/sbin/glite-lb-$$p"; \
	done
	for p in ${EXAMPLES} lbmon-db ${EXAMPLES_CL} ${sh_PROGS} ; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/examples/glite-lb-$$p"; \
	done
	${INSTALL} -m 755 ${top_srcdir}/examples/export.sh "${PREFIX}/examples/glite-lb-export.sh"
	${INSTALL} -m 644 ${MAN_GZ} ${PREFIX}/share/man/man1

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

.PHONY: default all compile examples fake check stage dist distsrc distbin install clean check_version
