# Default values
top_srcdir=..
stagedir=.
globalprefix=glite
classads_prefix=/opt/classads
lbprefix=lb
package=glite-lb-common
version=0.0.0
PREFIX=/opt/glite

nothrflavour=gcc32
thrflavour=gcc32pthr
expat_prefix=/opt/expat

-include Makefile.inc
-include ../project/version.properties

jobiddir=${top_srcdir}/../org.glite.jobid.api-c

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/interface:${top_srcdir}/test:${top_srcdir}/doc
AT3=${stagedir}/sbin/glite-lb-at3

SUFFIXES = .T

CC=gcc

DEBUG:=-g -O0 -Wall

ifdef LB_STANDALONE
	JOBID_OBJS:=cjobid.o strmd5.o
	LB_STANDALONE_FLAGS:=-DLB_STANDALONE
else
	JOBID_LIB:=-lglite_jobid
endif
TRIO_LIB:=-lglite_lbu_trio

PERF_HDRS:=lb_perftest.h
PERF_OBJS:=lb_perftest.o

CFLAGS:=${DEBUG} -I${top_srcdir}/src -I${top_srcdir}/interface \
	-I. \
	-I${top_srcdir}/test \
	-I${stagedir}/include \
	-I${expat_prefix}/include \
	-I${classads_prefix}/include \
	-I${classads_prefix}/include/classad \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE \
	-DDATAGRID_EXTENSION ${LB_STANDALONE_FLAGS} \
	-DBUILDING_LB_COMMON 

ifneq (${expat_prefix},/usr)
	EXPAT_LIBS:=-L${expat_prefix}/${libdir} -L${expat_prefix}/lib
endif

EXPAT_LIBS:=${EXPAT_LIBS} -lexpat

ifneq (${classads_prefix},/usr)
        classadslib := -L${classads_prefix}/${libdir} -L${classads_prefix}/lib 
endif

classadslib := ${classadslib} -lclassad


LDFLAGS:=-L${stagedir}/${libdir} -L${stagedir}/lib \
	${COVERAGE_FLAGS}

EXT_LIBS:= ${EXPAT_LIBS} ${JOBID_LIB} ${TRIO_LIB} ${classadslib}
EXT_THRLIBS:= ${EXPAT_LIBS} ${JOBID_LIB} ${TRIO_LIB} ${classadslib}

TEST_LIBS:=-L${cppunit_prefix}/${libdir} -lcppunit
TEST_INC:=-I${cppunit_prefix}/include

ifdef LB_PROF
        CFLAGS:= ${CFLAGS} -pg -g
        LDFLAGS:= ${LDFLAGS} -pg
endif

COMPILE:=libtool --mode=compile ${CC}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/${libdir} ${LDFLAGS} 
LINKXX:=libtool --mode=link  ${CXX} -rpath ${stagedir}/${libdir} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

OBJS:=${JOBID_OBJS} ${PERF_OBJS} lb_plain_io.o events.o mini_http.o query_rec.o \
	status.o xml_conversions.o xml_parse.o ulm_parse.o param.o \
	events_parse.o il_string.o il_int.o notifid.o \
	il_log.o il_msg.o log_msg.o context.o \
	connpool.o 
LOBJS:=${OBJS:.o=.lo}

THROBJS:=${OBJS:.o=.thr.o}
THRLOBJS:=${OBJS:.o=.thr.lo}

HDRS:=context.h context-int.h lb_plain_io.h mini_http.h authz.h xml_parse.h \
	xml_conversions.h log_proto.h events_parse.h il_string.h il_msg.h \
	ulm_parse.h connpool.h notifid.h notif_rec.h padstruct.h \
	query_rec.h timeouts.h LoggingExceptions.h CountRef.h ${PERF_HDRS} 
GEN_HDRS:=events.h jobstat.h common_version.h

STATICLIB:=libglite_lb_common_${nothrflavour}.a
THRSTATICLIB:=libglite_lb_common_${thrflavour}.a
LTLIB:=libglite_lb_common_${nothrflavour}.la
THRLTLIB:=libglite_lb_common_${thrflavour}.la

REPORTS:=${top_srcdir}/reports

ifdef LB_PERF
STAGE_PERFTEST=mkdir -p ${PREFIX}/examples/perftest; \
		${INSTALL} -m 644 ${top_srcdir}/examples/*.log ${PREFIX}/examples/perftest; \
		mkdir -p ${PREFIX}/sbin; ${INSTALL} -m 755 ${top_srcdir}/src/perftest_common.sh ${PREFIX}/sbin
else
STAGE_PERFTEST=true
endif

default: all

generate: ${GEN_HDRS}

common_version.h: ${top_srcdir}/project/version.properties
	echo "#define GLITE_LB_COMMON_VERSION \"${version}\"" >$@

all compile: generate ${STATICLIB} ${LTLIB} ${THRSTATICLIB} ${THRLTLIB}

# In order to use libtool versioning correcty, we must have:
#
# current = major + minor + offset
# revision = patch
# age = minor
#
# where offset is a sum of maximal released minor's of all previous major's
# 

# counted minors: 1.1, 3.1
offset=2

version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

${STATICLIB}: ${OBJS}
	ar crv $@ ${OBJS}
	ranlib $@

${THRSTATICLIB}: ${THROBJS}
	ar crv $@ ${THROBJS}
	ranlib $@

${LTLIB}: ${OBJS}
	${LINKXX} ${version_info} -o $@ ${LOBJS} \
		-lglite_security_gss_${nothrflavour} \
		${EXT_LIBS} \
		-lm

${THRLTLIB}: ${THROBJS}
	${LINKXX} ${version_info} -o $@ ${THRLOBJS} \
		-lglite_security_gss_${thrflavour} \
		${EXT_THRLIBS} \
		-lm

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

check: compile mkreports check.parse check.il

mkreports:
	-mkdir -p ${REPORTS}

check.parse: test_parse
	LD_LIBRARY_PATH=${cares_prefix}/${libdir}:${classads_prefix}/${libdir}:${LD_LIBRARY_PATH} ./test_parse ${REPORTS}/parse.xml

test_parse: parse.o test_main.cpp
	${LINKXX} -o test_parse parse.o ${LTLIB} ${TEST_LIBS} ${EXT_LIBS}

check.il: il_test
	LD_LIBRARY_PATH=${cares_prefix}/${libdir}:${classads_prefix}/${libdir}:${LD_LIBRARY_PATH} ./il_test ${REPORTS}/il.xml

il_test: il_test.o il_int_test.o il_string_test.o il_msg_test.o
	${LINKXX} -o $@ $+ ${LTLIB} ${TEST_LIBS} ${EXT_LIBS}

test_coverage:
	-mkdir coverage
	cd coverage && $(MAKE) -f ../Makefile top_srcdir=../../ COVERAGE_FLAGS="-fprofile-arcs -ftest-coverage" check
	cd coverage && for i in ${OBJS}; do gcov -o .libs/ $$i ; done

cjobid.c strmd5.c:
	if [ ! -d ${jobiddir} ]; then echo "Directory ${jobiddir} not found"; exit 1; fi
	mkdir -p glite/jobid
	rm -f jobid && ln -fs . jobid
	cp -rv ${jobiddir}/interface/cjobid.h glite/jobid/
	cp -rv ${jobiddir}/src/jobid/*.h glite/jobid/
	cp -rv ${jobiddir}/src/jobid/*.[ch] .

doc:

olddoc: generate events.tex status.tex
	cp ${top_srcdir}/doc/*.dox .
	echo "PROJECT_NUMBER = ${version}" >> C.dox
	echo "PROJECT_NUMBER = ${version}" >> CPP.dox
	doxygen C.dox
	doxygen CPP.dox

install: 
	mkdir -p ${PREFIX}/${libdir}
	mkdir -p ${PREFIX}/share/doc/${package}-${version}/examples
	${INSTALL} -m 644 ${LTLIB} ${THRLTLIB} ${MAILDIR_LIB} ${PREFIX}/${libdir}
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	( cd ${top_srcdir}/project && ${INSTALL} -m 644 ChangeLog package.description package.summary ${PREFIX}/share/doc/${package}-${version} )
#	cp -r C CPP ${PREFIX}/share/doc/${package}-${version}
	mkdir -p ${PREFIX}/include/${globalprefix}/${lbprefix} ;
	(cd ${top_srcdir}/interface && ${INSTALL} -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${lbprefix}) ;
	${INSTALL} -m 644 ${GEN_HDRS} ${PREFIX}/include/${globalprefix}/${lbprefix} ;
	${INSTALL} -m 644 ${STATICLIB} ${THRSTATICLIB} ${PREFIX}/${libdir};
	if [ x${LB_STANDALONE} = xyes ]; then \
		mkdir -p ${PREFIX}/include/glite/wmsutils/jobid ; \
		${INSTALL} -m 644 glite/wmsutils/jobid/*.h ${PREFIX}/include/glite/wmsutils/jobid/ ; \
	fi ;
	${STAGE_PERFTEST} ;

clean:
	rm -rvf *.o *.lo .libs lib* *.c *.h *.dox C/ CPP/ test_parse il_test parse.cpp events.tex status.tex
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/ debian/

%.o: %.c
	${COMPILE} ${CFLAGS} -c $<

il_int_test.o il_string_test.o il_test.o il_msg_test.o parse.o: %.o: %.cpp test_main.cpp
	${CXX} -c ${CFLAGS} ${TEST_INC} $<

%.thr.o: %.c
	${COMPILE} ${CFLAGS} -D_REENTRANT -DGLITE_LB_THREADED -o $@ -c $<

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

%.tex: %.tex.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null
