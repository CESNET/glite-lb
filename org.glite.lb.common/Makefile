# Default values
top_srcdir=..
stagedir=.
globalprefix=glite
classads_prefix=/opt/classads
lbprefix=lb
package=glite-lb-common
version=0.0.0
PREFIX=/opt/glite
prefix=

expat_prefix=/opt/expat

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

jobiddir=${top_srcdir}/../org.glite.jobid.api-c

VPATH=${top_srcdir}/src:${top_srcdir}/interface:${top_srcdir}/test:${top_srcdir}/doc
AT3=${stagedir}${prefix}/sbin/glite-lb-at3

SUFFIXES = .T

CC=gcc

SOURCES=\
	doc/*.dox doc/*.tex.T \
	examples/perf_*.log \
	interface/*.h interface/*.h.T \
	m4/*.m4 \
	project/.post* \
	src/*.c src/*.c.T src/*.sh \
	test/*.cpp test/*.cpp.T \
	LICENSE Makefile
SOURCES_EXEC=src/*.sh

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


ifneq (${expat_prefix},/usr)
        expatlib := -L${expat_prefix}/${libdir} -L${expat_prefix}/lib 
endif
ifneq (${classads_prefix},/usr)
        classadslib := -L${classads_prefix}/${libdir} -L${classads_prefix}/lib 
endif

CPPUNIT_LIBS?=-L${cppunit_prefix}/${libdir} -lcppunit
CPPUNIT_CFLAGS?=-I${cppunit_prefix}/include
EXPAT_CFLAGS?=-I${expat_prefix}/include
EXPAT_LIBS?=${expatlib} -lexpat
CLASSADS_CFLAGS?=-I${classads_prefix}/include -I${classads_prefix}/include/classad
CLASSADS_LIBS?=${classadslib} -lclassad

CFLAGS:=${DEBUG} -I${top_srcdir}/src -I${top_srcdir}/interface \
	-I. \
	-I${top_srcdir}/test \
	-I${stagedir}${prefix}/include \
	${EXPAT_CFLAGS} \
	${CLASSADS_CFLAGS} \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE \
	-DDATAGRID_EXTENSION ${LB_STANDALONE_FLAGS} \
	-DBUILDING_LB_COMMON ${CFLAGS}

LDFLAGS:=${LDFLAGS} -L${stagedir}${prefix}/${libdir} \
	${COVERAGE_FLAGS}

EXT_LIBS:= ${EXPAT_LIBS} ${JOBID_LIB} ${TRIO_LIB} ${CLASSADS_LIBS}

ifdef LB_PROF
        CFLAGS:= ${CFLAGS} -pg -g
        LDFLAGS:= ${LDFLAGS} -pg
endif

COMPILE:=libtool --mode=compile ${CC}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS} 
LINKXX:=libtool --mode=link  ${CXX} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

OBJS:=${JOBID_OBJS} ${PERF_OBJS} lb_plain_io.o events.o mini_http.o query_rec.o \
	status.o xml_conversions.o xml_parse.o ulm_parse.o param.o \
	events_parse.o il_string.o il_int.o notifid.o \
	il_log.o il_msg.o log_msg.o context.o \
	connpool.o events_json.o
LOBJS:=${OBJS:.o=.lo}

THROBJS:=${OBJS:.o=.thr.o}
THRLOBJS:=${OBJS:.o=.thr.lo}

HDRS:=context.h context-int.h lb_plain_io.h mini_http.h authz.h xml_parse.h \
	xml_conversions.h log_proto.h events_parse.h il_string.h il_msg.h \
	ulm_parse.h connpool.h notifid.h notif_rec.h padstruct.h \
	query_rec.h timeouts.h LoggingExceptions.h CountRef.h ${PERF_HDRS} \
	events_json.h
GEN_HDRS:=events.h jobstat.h common_version.h

NOTHRSTATICLIB:=libglite_lb_common_${nothrflavour}.a
THRSTATICLIB:=libglite_lb_common_${thrflavour}.a
STATICLIB:=libglite_lb_common.a
NOTHRLTLIB:=libglite_lb_common_${nothrflavour}.la
THRLTLIB:=libglite_lb_common_${thrflavour}.la
LTLIB:=libglite_lb_common.la

REPORTS:=${top_srcdir}/reports

ifeq (${thrflavour},)
lib_LTLIBRARIES=${LTLIB}
lib_LIBRARIES=${STATICLIB}
default_commonlib=${LTLIB}
else
lib_LTLIBRARIES=${NOTHRLTLIB} ${THRLTLIB}
lib_LIBRARIES=${NOTHRSTATICLIB} ${THRSTATICLIB}
default_commonlib=${NOTHRLTLIB}
endif

ifdef LB_PERF
STAGE_PERFTEST=mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples/perftest; \
		${INSTALL} -m 644 ${top_srcdir}/examples/*.log ${DESTDIR}${PREFIX}${prefix}/${libdir}/glite-lb/examples/perftest; \
		mkdir -p ${DESTDIR}${PREFIX}${prefix}/sbin; ${INSTALL} -m 755 ${top_srcdir}/src/perftest_common.sh ${DESTDIR}${PREFIX}${prefix}/sbin
else
STAGE_PERFTEST=true
endif

generate: ${GEN_HDRS}

common_version.h: ${top_srcdi${top_srcdir}/project/version.properties
	echo "#define GLITE_LB_COMMON_VERSION \"${version}\"" >$@

all compile: generate ${lib_LTLIBRARIES} ${lib_LIBRARIES}

# In order to use libtool versioning correcty, we must have:
#
# current = major + minor + offset
# revision = patch
# age = minor
#
# where offset is a sum of maximal released minor's of all previous major's
# 

# counted minors: 1.1, 2.0, 3.1, 4.0, 5.1, 6.1, 7.1
offset=5

version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

ifeq (${thrflavour},)
${STATICLIB}: ${THROBJS}
	ar crv $@ ${THROBJS}
	ranlib $@

${LTLIB}: ${THROBJS}
	${LINK} ${version_info} -o $@ ${THRLOBJS} \
		-lglite_security_gss \
		${EXT_LIBS} \
		-lm
else
${NOTHRSTATICLIB}: ${OBJS}
	ar crv $@ ${OBJS}
	ranlib $@

${THRSTATICLIB}: ${THROBJS}
	ar crv $@ ${THROBJS}
	ranlib $@

${NOTHRLTLIB}: ${OBJS}
	${LINK} ${version_info} -o $@ ${LOBJS} \
		-lglite_security_gss_${nothrflavour} \
		${EXT_LIBS} \
		-lm

${THRLTLIB}: ${THROBJS}
	${LINK} ${version_info} -o $@ ${THRLOBJS} \
		-lglite_security_gss_${thrflavour} \
		${EXT_LIBS} \
		-lm
endif

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

check: compile mkreports check.parse check.il

mkreports:
	-mkdir -p ${REPORTS}

check.parse: test_parse
	LD_LIBRARY_PATH=${cares_prefix}/${libdir}:${classads_prefix}/${libdir}:${LD_LIBRARY_PATH} ./test_parse ${REPORTS}/parse.xml

test_parse: parse.o test_main.cpp
	${LINKXX} -o test_parse parse.o ${default_commonlib} ${CPPUNIT_LIBS} ${EXT_LIBS}

check.il: il_test
	LD_LIBRARY_PATH=${cares_prefix}/${libdir}:${classads_prefix}/${libdir}:${LD_LIBRARY_PATH} ./il_test ${REPORTS}/il.xml

il_test: il_test.o il_int_test.o il_string_test.o il_msg_test.o
	${LINKXX} -o $@ $+ ${default_commonlib} ${CPPUNIT_LIBS} ${EXT_LIBS}

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
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	${INSTALL} -m 755 ${lib_LTLIBRARIES} ${DESTDIR}${PREFIX}${prefix}/${libdir}
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	( cd ${top_srcdir}/project && ${INSTALL} -m 644 ChangeLog package.description package.summary ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version} )
#	cp -r C CPP ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix} ;
	(cd ${top_srcdir}/interface && ${INSTALL} -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix}) ;
	${INSTALL} -m 644 ${GEN_HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbprefix} ;
	${INSTALL} -m 644 ${lib_LIBRARIES} ${DESTDIR}${PREFIX}${prefix}/${libdir};
	if [ x${LB_STANDALONE} = xyes ]; then \
		mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/glite/wmsutils/jobid ; \
		${INSTALL} -m 644 glite/wmsutils/jobid/*.h ${DESTDIR}${PREFIX}${prefix}/include/glite/wmsutils/jobid/ ; \
	fi ;
	${STAGE_PERFTEST} ;

clean:
	rm -rvf *.o *.lo .libs lib* *.c *.h *.dox C/ CPP/ test_parse il_test parse.cpp events.tex status.tex
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf Makefile.inc *.spec debian/

%.o: %.c
	${COMPILE} ${CFLAGS} -c $<

il_int_test.o il_string_test.o il_test.o il_msg_test.o parse.o: %.o: %.cpp test_main.cpp
	${CXX} -c ${CFLAGS} ${CPPUNIT_CFLAGS} $<

%.thr.o: %.c
	${COMPILE} ${CFLAGS} -D_REENTRANT -DGLITE_LB_THREADED -o $@ -c $<

%.h: %.h.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

%.cpp: %.cpp.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

%.tex: %.tex.T
	rm -f $@
	${AT3} $< >$@ || ( rm -f $@; false )
	chmod -w $@ >/dev/null

.PHONY: default all generate compile stage check mkreports check.parse check.il testcoverage doc olddoc install clean distclean dist distcheck
