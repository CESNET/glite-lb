# defaults
top_srcdir=..
builddir=build
stagedir=.
distdir=.
globalprefix=glite
lbutilsprefix=lbu
package=glite-lbjp-common-db
PREFIX=/opt/glite
prefix=

mysql-devel_prefix=/opt/mysql
postgresql_prefix=/usr
cppunit_prefix=/opt/cppunit
thrflavour=gcc32pthr
nothrflavour=gcc32

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}
docdir?=${prefix}/share/doc/${package}-${version}

CC=gcc

VPATH=${top_srcdir}/interface:${top_srcdir}/src:${top_srcdir}/examples:${top_srcdir}/test

SOURCES=doc/C.dox examples/*.c interface/*.h project/.post* src/*.c test/*.cpp Makefile

DEBUG:=-g -O0 -W -Wall

CFLAGS:= \
	${DEBUG} \
	-DVERSION=\"${version}\" \
	-I${stagedir}${prefix}/include -I${top_srcdir}/src -I. \
	-I${top_srcdir}/interface \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE \
	-DHAVE_SYSLOG_H=1 ${CFLAGS}

ifdef LBS_DB_PROFILE
	CFLAGS:=${CFLAGS} -DLBS_DB_PROFILE
endif
ifneq (${mysql-devel_prefix},no)
	OBJS:=${OBJS} db-mysql.o
	CFLAGS:=${CFLAGS} -DMYSQL_ENABLED=1
	MYSQL_CFLAGS?=-I${mysql-devel_prefix}/include -I${mysql-devel_prefix}/include/mysql
	MYSQL_LIBS?=-L${mysql-devel_prefix}/${libdir} -L${mysql-devel_prefix}/${libdir}/mysql -lmysqlclient
endif
ifneq (${postgresql_prefix},no)
	OBJS:=${OBJS} db-pg.o
	CFLAGS:=${CFLAGS} -DPSQL_ENABLED=1
	PSQL_CFLAGS?=-I${postgresql_prefix}/include -I${postgresql_prefix}/include/postgresql
	PSQL_LIBS?=-L${postgresql_prefix}/${libdir} -lpq
endif

TEST_LIBS:=-L${cppunit_prefix}/${libdir} -lcppunit
TEST_INC:=-I${cppunit_prefix}/include

LDFLAGS:=${LDFLAGS} -L${stagedir}${prefix}/${libdir} ${COVERAGE_FLAGS}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
COMPILEXX:=libtool --mode=compile ${CXX} ${CFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
SOLINK:=libtool --mode=link ${CC} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS}
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
INSTALL:=libtool --mode=install install

EXT_LIBS:=-lglite_lbu_trio -lglite_lbu_log ${MYSQL_LIBS} ${PSQL_LIBS}
TESTOBJS:=${OBJS} dbtest.o
OBJS:=${OBJS} db.o
HDRS:=db.h
LOBJS:=${OBJS:.o=.lo}
LTESTOBJS:=${TESTOBJS:.o=.lo}

# In order to use libtool versioning correcty, we must have:
#
# current = major + minor + offset
# revision = patch
# age = minor
#
# where offset is a sum of maximal released minor's of all previous major's
# 

# counted minors: 1.0, 2.0
offset=0

version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

all: compile doc

libglite_lbu_db.la: ${LOBJS}
	${SOLINK} ${version_info} -o $@ $+ ${EXT_LIBS}

libglite_lbu_dbtest.la: ${LTESTOBJS}
	${SOLINK} ${version_info} -o $@ $+ ${EXT_LIBS}

dbtest.lo dbtest.o: db.c db.h
	${COMPILE} -DGLITE_LBU_DEFAULT_RESULT_BUFFER_LENGTH=10 -c $< -o $@

db_test_mysql: db_test_mysql.lo libglite_lbu_dbtest.la
	${LINK} -o $@ $+

db_test_psql: db_test_psql.lo libglite_lbu_dbtest.la
	${LINK} -o $@ $+

db_expire: db_expire.lo libglite_lbu_dbtest.la
	${LINK} -o $@ $+

timezone: timezone.lo libglite_lbu_db.la
	${LINKXX} -o $@ $+ ${TEST_LIBS}

compile: libglite_lbu_db.la

check: timezone
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${log4c_prefix}/${libdir} ./timezone

test_coverage:
	-mkdir coverage
	cd coverage && $(MAKE) -f ../Makefile top_srcdir=../../ COVERAGE_FLAGS="-fprofile-arcs -ftest-coverage" check
	cd coverage && for i in `echo ${OBJS} | tr ' ' '\012' | sort -u`; do gcov $$i ; done

examples: db_test_mysql db_test_psql db_expire

doc:

olddoc:
	cp ${top_srcdir}/doc/*.dox .
	echo "PROJECT_NUMBER = ${version}" >> C.dox
	doxygen C.dox

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

install: all
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
#	-mkdir -p ${DESTDIR}${PREFIX}${docdir}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbutilsprefix}
#	-cp -r C ${DESTDIR}${PREFIX}${docdir}
	${INSTALL} -m 755 "libglite_lbu_db.la" "${DESTDIR}${PREFIX}${prefix}/${libdir}/libglite_lbu_db.la"; \
	${INSTALL} -m 644 ${top_srcdir}/interface/${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbutilsprefix}

clean:
	rm -rvf *.o *.lo *.loT .libs lib* *.c *.h *.dox C/ CPP/
	rm -rvf db_expire db_test db_test_mysql db_test_psql timezone
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

db-mysql.o db-mysql.lo: db-mysql.c
	${COMPILE} ${MYSQL_CFLAGS} -c $<

db-pg.o db-pg.lo: db-pg.c
	${COMPILE} ${PSQL_CFLAGS} -c $<

db_test_mysql.o db_test_mysql.lo: db_test.c
	${COMPILE} -DMYSQL_BACKEND=1 -c $< -o $@

db_test_psql.o db_test_psql.lo: db_test.c
	${COMPILE} -DPSQL_BACKEND=1 -c $< -o $@

timezone.o timezone.lo: timezone.cpp
	${COMPILEXX} -c $< -o $@ ${TEST_INC}

%.o %.lo: %.c
	${COMPILE} -c $<

db.lo: db.c db.h db-int.h
db_test_psql.lo: libglite_lbu_dbtest.la db.h db-int.h db_test.c
db_test_mysql.lo: libglite_lbu_dbtest.la db.h db-int.h db_test.c
db-mysql.lo: db-mysql.c db-int.h db.h
db-pg.lo: db-pg.c db-int.h db.h

.PHONY: default all compile check examples doc stage dist distsrc distbin install clean test_coverage distclean dist distcheck
