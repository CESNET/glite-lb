# Default values
top_srcdir=..
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-client
version=0.0.0
CC:=gcc
CXX:=g++

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

# We must follow major version changes of org.glite.lb.common due to
# binary compatibility. However, we may live a life of our own, changing our
# major version more frequently. This variable specifies how many steps ahead
# we are (the number can be even negative).

VERSION_AHEAD=-3

# hacks needed for ETICS:
# - rpath out of installation directory ${PREFIX}
#   (the proper value is ${prefix}/${libdir})
# - installed *.la files should be removed
rpath?=${PREFIX}${prefix}/${libdir}

VPATH=${top_srcdir}/src:${top_srcdir}/interface:${top_srcdir}/test:${top_srcdir}/examples:${top_srcdir}/doc
AT3=${stagedir}${prefix}/sbin/glite-lb-at3
GENSAM=${top_srcdir}/examples/gen_sample_job
check_version_pl=$(shell sbin_dir=${stagedir}${prefix}/sbin; if test -f $${sbin_dir}/glite-lb-check_version.pl; then echo $${sbin_dir}; else echo ${prefix}/sbin; fi)/glite-lb-check_version.pl
CHECK_VERSION:=VERSION=${version} VERSION_AHEAD=${VERSION_AHEAD} perl ${check_version_pl}

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
	reallyrunning.l \
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
	cream_registered.l \
	cream_pending.l \
	cream_idle.l \
	cream_running.l \
	cream_reallyrunning.l \
	cream_done.l \
	cream_failed.l \
	submitted_ft_reg.l \
	running_ft.l \
	done_ft.l \
	aborted_ft.l \

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
COMMON_LIB_NOTHR:=-lglite_lb_common_${nothrflavour}
COMMON_LIB_THR:=-lglite_lb_common_${thrflavour}
COMMON_LIB:=-lglite_lb_common
TRIO_LIB:=-lglite_lbu_trio

CFLAGS:=${DEBUG} \
	-I. \
	-I${top_srcdir}/src -I${top_srcdir}/interface \
	-I${stagedir}${prefix}/include \
	${COVERAGE_FLAGS} \
	-DBUILDING_LB_CLIENT \
	-D_GNU_SOURCE ${LB_STANDALONE_FLAGS} ${LB_PERF_FLAGS} ${CFLAGS}

CXXFLAGS:=${CFLAGS}

EXT_LIB:=

CPPUNIT_LIBS?=-L${cppunit_prefix}/${libdir} -lcppunit
CPPUNIT_CFLAGS?=-I${cppunit_prefix}/include

LDFLAGS:=${LDFLAGS} -L${stagedir}${prefix}/${libdir} -L${PREFIX}${prefix}/${libdir} ${COVERAGE_FLAGS}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
CXXCOMPILE:=libtool --mode=compile ${CXX} ${CXXFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
INSTALL?=libtool --mode=install install

LIBOBJS:=connection.o consumer.o notification.o prod_proto.o \
	producer.o uiwrap.o statistics.o

PLUSOBJS:=Event.o Job.o JobStatus.o Notification.o ServerConnection.o stat_fields.o

HDRS:=consumer.h notification.h statistics.h prod_proto.h connection.h \
	Job.h Notification.h ServerConnection.h stat_fields.h
GEN_HDRS:=Event.h JobStatus.h producer.h

LIBTHROBJS:=${LIBOBJS:.o=.thr.o}
LIBLOBJS:=${LIBOBJS:.o=.lo}
LIBTHRLOBJS:=${LIBOBJS:.o=.thr.lo}

PLUSTHROBJS:=${PLUSOBJS:.o=.thr.o}
PLUSLOBJS:=${PLUSOBJS:.o=.lo}
PLUSTHRLOBJS:=${PLUSOBJS:.o=.thr.lo}

NOTHRLIB:=libglite_lb_client_${nothrflavour}.la
THRLIB:=libglite_lb_client_${thrflavour}.la
LIB:=libglite_lb_client.la
NOTHRPLUSLIB:=libglite_lb_clientpp_${nothrflavour}.la
THRPLUSLIB:=libglite_lb_clientpp_${thrflavour}.la
PLUSLIB:=libglite_lb_clientpp.la
ifeq (${thrflavour},)
c_LIBRARIES=${LIB}
plus_LIBRARIES=${PLUSLIB}
default_clientlib=${LIB}
default_clientpluslib=${PLUSLIB}
default_commonlib=${COMMON_LIB}
default_clientlib_thr=${LIB}
default_commonlib_thr=${COMMON_LIB}
else
c_LIBRARIES=${NOTHRLIB} ${THRLIB}
plus_LIBRARIES=${NOTHRPLUSLIB} ${THRPLUSLIB}
default_clientlib=${NOTHRLIB}
default_clientpluslib=${NOTHRPLUSLIB}
default_commonlib=${COMMON_LIB_NOTHR}
default_clientlib_thr=${THRLIB}
default_commonlib_thr=${COMMON_LIB_THR}
endif
lib_LTLIBRARIES=${c_LIBRARIES} ${plus_LIBRARIES}

TOOLS:=${LB_PERF_TOOLS}
EXAMPLES_SRC:=log_usertag_proxy.c job_log.c job_reg.c feed_shark.c query_ext.c query_seq_code.c stats.c stats-duration-fromto.c abort_job.c change_acl.c stresslog.c flood_proxy.c dagids.c stress_context.c parse_eventsfile.c test_changed_jdl.c done_failed_events.c
# sandbox_collection_reg_dummy.c
EXAMPLES:=${EXAMPLES_SRC:.c=}

# TODO: migrate them here from branch_RC31_3
# EXAMPLES_PLUS_SRC:=indexed_attrs_plus.cpp job_status_plus.cpp query_events_plus.cpp listener.cpp user_jobs_plus.cpp job_log_plus.cpp notify_plus.cpp
# EXAMPLES_PLUS:=${EXAMPLES_PLUS_SRC:.cpp=}

EXAMPLES_CL_SRC:=user_jobs.c job_status.c multiple_user_jobs.c
EXAMPLES_CL:=${EXAMPLES_CL_SRC:.c=}

EXAMPLES_CL_THR_SRC:=user_jobs_threaded.c job_status_threaded.c
EXAMPLES_CL_THR:=${EXAMPLES_CL_THR_SRC:.c=}

MAN_GZ:=glite-lb-logevent.1.gz glite-lb-notify.1.gz
MAN8_GZ:=glite-lb-dump.8.gz glite-lb-load.8.gz
MAN = $(MAN_GZ:.gz=)
MAN8 = $(MAN8_GZ:.gz=)

PLUS_EXTRA_LIB:=-lglite_jobid


# version_info=-version-info `echo ${version} | cut -d. -f1,2 | tr . :`

# counted minor versions: 1.0, 2.3, 3.2, 4.1
offset=6

version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

ifeq (${project},emi)
m4dir=${PREFIX}${prefix}/share/emi/build/m4
else
m4dir=${PREFIX}${prefix}/share/build/m4
endif

ifdef LB_STANDALONE
compile all: generate ${c_LIBRARIES} ${TOOLS} logevent notify register_sandbox examples ${MAN_GZ} ${MAN8_GZ}
else
compile all: check_version generate ${lib_LTLIBRARIES} ${TOOLS} logevent notify register_sandbox examples ${MAN_GZ} ${MAN8_GZ}
endif

generate: ${GEN_HDRS}

default: all

ifeq (${thrflavour},)
${LIB}: ${LIBTHROBJS}
	${LINK} ${version_info} -o $@ ${LIBTHRLOBJS} -rpath ${rpath} \
			${COMMON_LIB} ${TRIO_LIB} \
			-lglite_security_gss

${PLUSLIB}: ${PLUSTHROBJS} ${LIB}
	${LINKXX} ${version_info} -o $@ ${PLUSTHRLOBJS} -rpath ${rpath} ${LIB} ${PLUS_EXTRA_LIB}
else
${NOTHRLIB}: ${LIBOBJS}
	${LINK} ${version_info} -o $@ ${LIBLOBJS} -rpath ${rpath} \
			${COMMON_LIB_NOTHR} ${TRIO_LIB} \
			-lglite_security_gss_${nothrflavour}

${THRLIB}: ${LIBTHROBJS}
	${LINK} ${version_info} -o $@ ${LIBTHRLOBJS} -rpath ${rpath} \
			${COMMON_LIB_THR} ${TRIO_LIB} \
			-lglite_security_gss_${thrflavour}

${NOTHRPLUSLIB}: ${PLUSOBJS} ${NOTHRLIB}
	${LINKXX} ${version_info} -o $@ ${PLUSLOBJS} -rpath ${rpath} ${NOTHRLIB} ${PLUS_EXTRA_LIB}

${THRPLUSLIB}: ${PLUSTHROBJS} ${THRLIB}
	${LINKXX} ${version_info} -o $@ ${PLUSTHRLOBJS} -rpath ${rpath} ${THRLIB} ${PLUS_EXTRA_LIB}
endif

logevent: logevent.o args.o
	${LINK} -o $@ logevent.o args.o ${default_clientlib} ${EXT_LIB} 

notify: notify.o
	${LINKXX} -o $@ notify.o ${default_clientpluslib} ${EXT_LIB} ${PLUS_EXTRA_LIB}

register_sandbox: %: %.o
	${LINK} -o $@ $@.o ${default_clientlib} ${EXT_LIB}

${TOOLS} ${EXAMPLES} ${EXAMPLES_CL}: %: %.o
	${LINK} -o $@ $< ${default_clientlib} ${default_commonlib} ${EXT_LIB} 

${EXAMPLES_CL_THR}: %: %.o
	${LINK} -o $@ $< ${default_clientlib_thr} ${default_commonlib_thr} ${EXT_LIB} -lpthread

${TOOLS}: ${default_clientlib}

${PLUSOBJS}: %.o: %.cpp
	${CXXCOMPILE} -c $<

${PLUSTHROBJS}: %.thr.o: %.cpp
	${CXXCOMPILE} -o $@ -c $<

${EXAMPLES_PLUS}: ${default_clientpluslib}

${EXAMPLES_PLUS}: %: %.o
	${LINKXX} -o $@ $< ${default_clientpluslib}

${LIBOBJS}: %.o: %.c
	${COMPILE} -c $<

${LIBTHROBJS}: %.thr.o: %.c
	${COMPILE} -o $@ -c $<

${MAN_GZ}: ${MAN}
	rm -f ${MAN_GZ} ${MAN}
	cp $? .
	gzip -9 -f $(notdir $?)

${MAN8_GZ}: ${MAN8}
	cp $? .
	gzip -9 -f $(notdir $?)

perftest_logjobs.o: perftest_logjobs.c
	${CC} ${CFLAGS} -DLB_PERF_DROP -c $<

# catches $TOOLS and logevent compilation
%.lo %.o: %.c
	${CC} ${CFLAGS} -c $<

%.h: %.h.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

%.lo %.o: %.cpp
	${CXX} ${CFLAGS} ${GLOBUSINC} -c $<

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

%.cpp: %.cpp.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

%.sh: %.l gen_begin gen_sample_job
	rm -f $@
	-chmod +x ${GENSAM}
	$(GENSAM) $< >$@ || rm -f $@
	chmod -w,+x $@ > /dev/null

examples: ${EXAMPLES} ${EXAMPLES_PLUS} ${EXAMPLES_CL} ${EXAMPLES_CL_THR} ${sh_PROGS}

check: compile 
# shut up check.producer

check.producer: producer_test
	./producer_test

producer_test: producer_test.o prod_proto_test.o
	${LINKXX} -o $@ ${default_clientlib} ${CPPUNIT_LIBS} $+ ${EXT_LIB}

producer_test.o: %.o: %.cpp
	${CXX} -c ${CXXFLAGS} ${CPPUNIT_CFLAGS} $<

man: ${MAN_GZ} ${MAN8_GZ}

stage:  compile
	$(MAKE) install PREFIX=${stagedir} staging=1

link:
	-ln -s ${top_srcdir}/src/EventAttrNames.pl .
	-ln -s ${top_srcdir}/src/StatusAttrNames.pl .
	-ln -s ${top_srcdir}/src/uiwrap.T .

Event.h Event.cpp: link

JobStatus.h JobStatus.cpp: StatusAttrNames.pl

#EventAttrNames.pl:
#	rm -f $@
#	ln -s ${stagedir}${prefix}/share/perl/gLite/LB/$@ $@

dist: distsrc distbin

install:
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/bin
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/sbin
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix}
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}/examples/
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/man/man1
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/man/man8
	mkdir -p ${DESTDIR}${m4dir}
ifdef LB_STANDALONE
	${INSTALL} -m 755 ${c_LIBRARIES} ${DESTDIR}${PREFIX}${prefix}/${libdir}
else
	${INSTALL} -m 755 ${lib_LTLIBRARIES} ${DESTDIR}${PREFIX}${prefix}/${libdir}
	if test -n "${staging}"; then \
		install -m 755 ${plus_LIBRARIES} ${DESTDIR}${PREFIX}${prefix}/${libdir}; \
	fi
endif
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	( cd ${top_srcdir}/project && ${INSTALL} -m 644 ChangeLog package.description package.summary ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version} )
	${INSTALL} -m 644 ${top_srcdir}/doc/README-notify ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	(cd ${top_srcdir}/interface && ${INSTALL} -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix}) 
	${INSTALL} -m 644 ${GEN_HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix} 
ifndef LB_STANDALONE
#	cp -r C CPP ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
#       cp -r ${top_srcdir}/doc/api/{Makefile,api.tex,fig} ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}/api
endif
	for p in logevent notify register_sandbox; do \
		${INSTALL} -m 755 "$$p" "${DESTDIR}${PREFIX}${prefix}/bin/glite-lb-$$p"; \
	done
	for p in ${TOOLS} ; do \
		${INSTALL} -m 755 "$$p" "${DESTDIR}${PREFIX}${prefix}/sbin/glite-lb-$$p"; \
	done
	for p in ${EXAMPLES} ${EXAMPLES_PLUS} ${EXAMPLES_CL} ${EXAMPLES_CL_THR} ${sh_PROGS} ; do \
		${INSTALL} -m 755 "$$p" "${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples/glite-lb-$$p"; \
	done
	for p in ${EXAMPLES_SRC} ${EXAMPLES_PLUS_SRC} ${EXAMPLES_CL_SRC} ${EXAMPLES_CL_THR_SRC} ; do \
		${INSTALL} -m 644 "${top_srcdir}/examples/$$p" "${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}/examples/"; \
	done
	${INSTALL} -m 644 ${top_srcdir}/examples/Makefile "${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}/examples/"
	${INSTALL} -m 644 ${top_srcdir}/examples/README.examples "${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}/examples/"
	${INSTALL} -m 755 ${top_srcdir}/src/export.sh "${DESTDIR}${PREFIX}${prefix}/sbin/glite-lb-export.sh"
	${INSTALL} -m 755 ${top_srcdir}/examples/notify.pl ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples/glite-lb-notify.pl
	${INSTALL} -m 755 ${top_srcdir}/examples/sandbox_transfers.sh ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples/glite-lb-sandbox_transfers.sh
	${INSTALL} -m 644 ${MAN_GZ} ${DESTDIR}${PREFIX}${prefix}/share/man/man1
	${INSTALL} -m 644 ${MAN8_GZ} ${DESTDIR}${PREFIX}${prefix}/share/man/man8
	${INSTALL} -m 644 ${top_srcdir}/m4/glite_lb.m4 ${DESTDIR}${m4dir}/

clean:
	rm -rvf *.o *.lo .libs lib* *.c *.cpp *.h *.dox producer_test notify C/ CPP/
	rm -rvf ${lib_LTLIBRARIES} ${TOOLS} logevent  register_sandbox ${MAN_GZ} ${MAN8_GZ}
	rm -rvf ${EXAMPLES} ${EXAMPLES_CL} ${EXAMPLES_CL_THR} ${sh_PROGS}
	rm -f EventAttrNames.pl StatusAttrNames.pl uiwrap.T

distclean:
	rm -rvf Makefile.inc *.spec debian/

check_version:
	${CHECK_VERSION} ${stagedir}${prefix}/include/glite/lb/common_version.h

post-install:
	rm -rfv ${PREFIX}${prefix}/${libdir}/*.la

post-install-rpath:
	for file in `find ${PREFIX}${prefix}/${libdir} -type f` \
	            `find ${PREFIX}${prefix}/bin -type f` \
	            `find ${PREFIX}${prefix}/sbin -type f` \
	; do \
		if test -h $$file; then continue; fi; \
		orpath=`chrpath -l $$file 2>/dev/null`; \
		if test $$? = 0 ; then \
			orpath=`echo $$orpath | sed 's:.*RPATH=::'`; \
			rpath=`echo $$orpath | sed "s:${PREFIX}${prefix}:${sysroot}${prefix}:g" | tr ':' '\n' | grep -v '^$$' | tr '\n' ':' | sed 's/:$$//'`; \
			if test "$$orpath" = "$$rpath"; then continue; fi; \
			echo "$$file"; \
			echo " -> $$rpath"; \
			chrpath -r $$rpath $$file; \
			chrpath -d $$file; \
		fi; \
	done

.PHONY: default all compile examples check stage install clean distclean check_version link post-install post-install-rpath
