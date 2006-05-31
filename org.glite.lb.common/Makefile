# Default values
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-common
version=0.0.0
PREFIX=/opt/glite

glite_location=/opt/glite
globus_prefix=/opt/globus
nothrflavour=gcc32
thrflavour=gcc32pthr
expat_prefix=/opt/expat

-include Makefile.inc
-include ../project/version.properties

jobiddir=${top_srcdir}/../org.glite.wms-utils.jobid

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/test
AT3=perl -I${top_srcdir}/project ${top_srcdir}/project/at3

SUFFIXES = .T

CC=gcc

DEBUG:=-g -O0 -Wall
GLOBUSINC:= -I${globus_prefix}/include/${nothrflavour}
GLOBUSTHRINC:= -I${globus_prefix}/include/${thrflavour}

ifdef LB_STANDALONE
	JOBID_OBJS:=cjobid.o strmd5.o
	LB_STANDALONE_FLAGS:=-DLB_STANDALONE
else
	JOBID_LIB:=-lglite_wmsutils_cjobid
endif

PERF_HDRS:=lb_perftest.h
PERF_OBJS:=lb_perftest.o

CFLAGS:=${DEBUG} -I${top_srcdir}/src -I${top_srcdir}/interface \
	-I. \
	-I${top_srcdir}/test \
	-I${stagedir}/include \
	-I${glite_location}/include \
	-I${expat_prefix}/include \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE \
	-DDATAGRID_EXTENSION ${LB_STANDALONE_FLAGS}

# We must follow major version changes of org.glite.lb.client-interface due to
# binary compatibility. However, we may live a life of our own, changing our
# major version more frequently. This variable specifies how many steps ahead
# we are.

VERSION_AHEAD=1

GLOBUS_LIBS:=-L${globus_prefix}/lib \
        -lglobus_common_${nothrflavour} \
        -lglobus_gssapi_gsi_${nothrflavour} \

GLOBUS_THRLIBS:=-L${globus_prefix}/lib \
        -lglobus_common_${thrflavour} \
        -lglobus_gssapi_gsi_${thrflavour} \

GLITE_LIBS:=-L${glite_location}/lib

ifneq (${expat_prefix},/usr)
	EXPAT_LIBS:=-L${expat_prefix}/lib
endif
EXPAT_LIBS:=${EXPAT_LIBS} -lexpat

LDFLAGS:=-L${stagedir}/lib \
    ${COVERAGE_FLAGS}

EXT_LIBS:= ${EXPAT_LIBS} ${GLOBUS_LIBS} ${JOBID_LIB}
EXT_THRLIBS:= ${EXPAT_LIBS} ${GLOBUS_THRLIBS} ${JOBID_LIB}

TEST_LIBS:=-L${cppunit_prefix}/lib -lcppunit
TEST_INC:=-I${cppunit_prefix}/include

COMPILE:=libtool --mode=compile ${CC}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/lib ${LDFLAGS} 
LINKXX:=libtool --mode=link  ${CXX} -rpath ${stagedir}/lib ${LDFLAGS} 
INSTALL:=libtool --mode=install install

OBJS:=${JOBID_OBJS} ${PERF_OBJS} lb_plain_io.o escape.o events.o mini_http.o query_rec.o \
	status.o xml_conversions.o xml_parse.o ulm_parse.o param.o \
	events_parse.o il_string.o il_int.o notifid.o \
	il_log.o il_msg.o log_msg.o context.o trio.o strio.o lb_maildir.o 
LOBJS:=${OBJS:.o=.lo}

TRIO_OBJS:=escape.o trio.o strio.o
TRIO_LIB:=libglite_lb_trio.a

MAILDIR_OBJS:=lb_maildir.lo
MAILDIR_LIB:=libglite_lb_maildir.la

THROBJS:=${OBJS:.o=.thr.o}
THRLOBJS:=${OBJS:.o=.thr.lo}

HDRS:=context-int.h lb_plain_io.h mini_http.h authz.h xml_parse.h \
	xml_conversions.h log_proto.h events_parse.h il_string.h il_msg.h \
	escape.h ulm_parse.h trio.h lb_maildir.h ${PERF_HDRS}

STATICLIB:=libglite_lb_common_${nothrflavour}.a
THRSTATICLIB:=libglite_lb_common_${thrflavour}.a
LTLIB:=libglite_lb_common_${nothrflavour}.la
THRLTLIB:=libglite_lb_common_${thrflavour}.la

REPORTS:=${top_srcdir}/reports

ifdef LB_PERF
STAGE_PERFTEST=mkdir -p ${PREFIX}/examples/perftest; \
		install -m 644 ${top_srcdir}/examples/*.log ${PREFIX}/examples/perftest; \
		install -m 755 ${top_srcdir}/src/perftest_common.sh ${PREFIX}/sbin
else
STAGE_PERFTEST=true
endif

default: all

#all compile: check_version ${STATICLIB} ${LTLIB} ${THRSTATICLIB} ${THRLTLIB} ${TRIO_LIB} ${MAILDIR_LIB}
all compile: ${STATICLIB} ${LTLIB} ${THRSTATICLIB} ${THRLTLIB} ${TRIO_LIB} ${MAILDIR_LIB}

# FIXME: to use libtool versioning correcty, we should have:
#
# current = major + minor + offset
# revision = patch
# age = minor
#
# where offset is a sum of maximal released minor's of all previous major's
# 
# version_info=-version-info `echo ${version} | cut -d. -f1,2 | tr . :`


# counted minors: 1.1
offset=1

version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

${STATICLIB}: ${OBJS}
	ar crv $@ ${OBJS}
	ranlib $@

${TRIO_LIB}: ${TRIO_OBJS}
	ar crv $@ ${TRIO_OBJS}
	ranlib $@

${MAILDIR_LIB}: ${MAILDIR_OBJS}
	${LINK} ${version_info} -o $@ ${MAILDIR_OBJS}

${THRSTATICLIB}: ${THROBJS}
	ar crv $@ ${THROBJS}
	ranlib $@

${LTLIB}: ${OBJS}
	${LINK} ${version_info} -o $@ ${LOBJS} \
		-lglite_security_gss_${nothrflavour} \
		${EXT_LIBS} \
		-lm

${THRLTLIB}: ${THROBJS}
	${LINK} ${version_info} -o $@ ${THRLOBJS} \
		-lglite_security_gss_${thrflavour} \
		${EXT_THRLIBS} \
		-lm

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

check: compile mkreports check.parse check.il

mkreports:
	-mkdir ${REPORTS}

check.parse: test_parse
	./test_parse ${REPORTS}/parse.xml

test_parse: parse.cpp
	${CXX} -c ${CFLAGS} ${TEST_INC} $<
	${LINKXX} -o test_parse parse.o ${LTLIB} ${TEST_LIBS} ${EXT_LIBS}

check.il: il_test
	./il_test ${REPORTS}/il.xml

il_test: il_test.o il_int_test.o il_string_test.o il_msg_test.o
	${LINKXX} -o $@ $+ ${LTLIB} ${TEST_LIBS} ${EXT_LIBS}

test_coverage:
	-mkdir coverage
	cd coverage && $(MAKE) -f ../Makefile top_srcdir=../../ COVERAGE_FLAGS="-fprofile-arcs -ftest-coverage" check
	cd coverage && for i in ${OBJS}; do gcov -o .libs/ $$i ; done

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

cjobid.c strmd5.c:
	if [ ! -d ${jobiddir} ]; then echo "Directory ${jobiddir} not found"; exit 1; fi
	mkdir -p glite/wmsutils/jobid
	rm -f jobid && ln -fs . jobid
	cp -rv ${jobiddir}/interface/glite/wmsutils/jobid/cjobid.h glite/wmsutils/jobid/
	cp -rv ${jobiddir}/src/jobid/*.h glite/wmsutils/jobid/
	cp -rv ${jobiddir}/src/jobid/*.[ch] .

install:
	mkdir -p ${PREFIX}/lib
	mkdir -p ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${LTLIB} ${THRLTLIB} ${MAILDIR_LIB} ${PREFIX}/lib
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	if [ x${DOSTAGE} = xyes ]; then \
		mkdir -p ${PREFIX}/include/${globalprefix}/${lbprefix} ; \
		(cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${lbprefix}) ; \
		install -m 644 ${STATICLIB} ${THRSTATICLIB} ${TRIO_LIB} ${PREFIX}/lib; \
		if [ x${LB_STANDALONE} = xyes ]; then \
			mkdir -p ${PREFIX}/include/glite/wmsutils/jobid ; \
			install -m 644 glite/wmsutils/jobid/*.h ${PREFIX}/include/glite/wmsutils/jobid/ ; \
		fi ; \
		${STAGE_PERFTEST} ; \
	fi

clean:


%.o: %.c
	${COMPILE} ${GLOBUSINC} ${CFLAGS} -c $<

il_int_test.o il_string_test.o il_test.o il_msg_test.o: %.o: %.cpp
	${CXX} -c ${CFLAGS} ${TEST_INC} $<

%.thr.o: %.c
	${COMPILE} ${GLOBUSTHRINC} ${CFLAGS} -o $@ -c $<

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

check_version:
	@perl -ne '/#define GLITE_LB_CLIENT_INTERFACE "(\d+)\.\d+\.\d+"/; \
		$$iface=$$1; \
		$$_="${version}"; /(\d+)\.\d+\.\d+/; \
		if ($$iface + ${VERSION_AHEAD} != $$1) { \
			print "error: Major version of the interface ($$iface+${VERSION_AHEAD}) does not match implementation ($$1)\n" ;\
			exit 1;  \
		} \
		' ${stagedir}/include/glite/lb/interface_version.h
