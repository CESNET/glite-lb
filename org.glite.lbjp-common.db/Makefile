# defaults
top_srcdir=..
builddir=build
stagedir=.
distdir=.
globalprefix=glite
lbutilsprefix=lbu
package=glite-lb-utils-db
PREFIX=/opt/glite

mysql-devel_prefix=/opt/mysql
postgresql_prefix=/opt
cppunit_prefix=/opt/cppunit
thrflavour=gcc32pthr
nothrflavour=gcc32

-include Makefile.inc
-include ../project/version.properties

CC=gcc

VPATH=${top_srcdir}/interface:${top_srcdir}/src:${top_srcdir}/examples

DEBUG:=-g -O0 -W -Wall

archlib:=lib
host_cpu:=${shell uname -m}
ifeq (${host_cpu},x86_64) 
	archlib:=lib64
endif   

MYSQL_SONAME:=$(shell ../project/get_soname.sh mysqlclient ${mysql-devel_prefix}/${archlib} ${mysql_prefix}/${archlib} ${mysql-devel_prefix}/lib ${mysql_prefix}/lib)
PSQL_SONAME:=$(shell ../project/get_soname.sh pq ${postgresql_prefix}/${archlib} ${postgresql_prefix}/lib)

MYSQL_CPPFLAGS:=-I${mysql-devel_prefix}/include -I${mysql-devel_prefix}/include/mysql
PSQL_CPPFLAGS=-I`pg_config --includedir`


CFLAGS:= \
	${DEBUG} \
	-DVERSION=\"${module.version}\" \
	-I${stagedir}/include -I${top_srcdir}/src -I. \
	-I${top_srcdir}/interface \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE

ifdef LBS_DB_PROFILE
	CFLAGS:=${CFLAGS} -DLBS_DB_PROFILE
endif
ifneq (${mysql-devel_prefix},no)
	OBJS:=${OBJS} db-mysql.o
	CFLAGS:=${CFLAGS} -DMYSQL_SONAME=\"${MYSQL_SONAME}\"
endif
ifneq (${postgresql_prefix},no)
	OBJS:=${OBJS} db-pg.o
	CFLAGS:=${CFLAGS} -DPSQL_SONAME=\"${PSQL_SONAME}\"
endif

TEST_LIBS:=-L${cppunit_prefix}/lib -lcppunit
TEST_INC:=-I${cppunit_prefix}/include

LDFLAGS:=-L${stagedir}/lib ${COVERAGE_FLAGS}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/lib ${LDFLAGS} 
INSTALL:=libtool --mode=install install

EXT_LIBS:=-lglite_lbu_trio -lpthread -ldl
TESTOBJS:=${OBJS} dbtest.o
OBJS:=${OBJS} db.o
HDRS:=db.h
LOBJS:=${OBJS:.o=.lo}
LTESTOBJS:=${TESTOBJS:.o=.lo}

default all: compile doc

check_soname:
	if [ "${mysql-devel_prefix}" != no ]; then \
		if [ "${MYSQL_SONAME}" = notfound ]; then \
			echo "MySQL shared library not found!"; \
			false; \
		fi \
	fi
	if [ "${postgresql_prefix}" != no ]; then \
		if [ "${PSQL_SONAME}" = notfound ]; then \
			echo "PostgreSQL shared library not found!"; \
			false; \
		fi \
	fi

libglite_lbu_db.la: check_soname ${LOBJS}
	${LINK} -o $@ $+ ${EXT_LIBS}

libglite_lbu_dbtest.la: check_soname ${LTESTOBJS}
	${LINK} -o $@ $+ ${EXT_LIBS}

dbtest.lo dbtest.o: db.c db.h
	${COMPILE} -DGLITE_LBU_DEFAULT_RESULT_BUFFER_LENGTH=10 -c $< -o $@

db_test_mysql: db_test_mysql.lo libglite_lbu_dbtest.la
	${LINK} -o $@ $+ ${EXT_LIBS}

db_test_psql: db_test_psql.lo libglite_lbu_dbtest.la
	${LINK} -o $@ $+ ${EXT_LIBS}

db_expire: db_expire.lo libglite_lbu_dbtest.la
	${LINK} -o $@ $+ ${EXT_LIBS}

compile: libglite_lbu_db.la

check:
	-echo No checks here yet.

test_coverage:
	-mkdir coverage
	cd coverage && $(MAKE) -f ../Makefile top_srcdir=../../ COVERAGE_FLAGS="-fprofile-arcs -ftest-coverage" check
	cd coverage && for i in `echo ${OBJS} | tr ' ' '\012' | sort -u`; do gcov $$i ; done

examples: db_test_mysql db_test_psql db_expire

doc:

olddoc:
	cp ${top_srcdir}/doc/*.dox .
	echo "PROJECT_NUMBER = ${module.version}" >> C.dox
	doxygen C.dox

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

dist: distsrc distbin

distsrc:
	mkdir -p ${top_srcdir}/${package}-${module.version}
	cd ${top_srcdir} && GLOBIGNORE="${package}-${module.version}" && cp -Rf * ${package}-${module.version}
	cd ${top_srcdir} && tar -czf ${distdir}/${package}-${module.version}_src.tar.gz --exclude-from=project/tar_exclude ${package}-${module.version}
	rm -rf ${top_srcdir}/${package}-${module.version}

distbin:
	$(MAKE) install PREFIX=`pwd`/tmpbuilddir${stagedir}
	save_dir=`pwd`; cd tmpbuilddir${stagedir} && tar -czf $$save_dir/${top_srcdir}/${distdir}/${package}-${module.version}_bin.tar.gz *; cd $$save_dir
	rm -rf tmpbuilddir
        
install: all
	-mkdir -p ${PREFIX}/lib
	-mkdir -p ${PREFIX}/share/doc/${package}-${module.version}
	-mkdir -p ${PREFIX}/include/${globalprefix}/${lbutilsprefix}
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${module.version}
	-cp -r C ${PREFIX}/share/doc/${package}-${module.version}
	${INSTALL} -m 755 "libglite_lbu_db.la" "${PREFIX}/lib/libglite_lbu_db.la"; \
	${INSTALL} -m 644 ${top_srcdir}/interface/${HDRS} ${PREFIX}/include/${globalprefix}/${lbutilsprefix}

clean:
	rm -rvf *.o *.lo .libs lib* *.c *.h *.dox C/ CPP/
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/
	rm -rvf db_expire db_test db_test_mysql db_test_psql

db-mysql.o db-mysql.lo: db-mysql.c
	${COMPILE} ${MYSQL_CPPFLAGS} -c $<

db-pg.o db-pg.lo: db-pg.c
	${COMPILE} ${PSQL_CPPFLAGS} -c $<

db_test_mysql.o db_test_mysql.lo: db_test.c
	${COMPILE} -DMYSQL_BACKEND=1 -c $< -o $@

db_test_psql.o db_test_psql.lo: db_test.c
	${COMPILE} -DPSQL_BACKEND=1 -c $< -o $@

%.o %.lo: %.c
	${COMPILE} -c $<

db.lo: db.c db.h db-int.h
db_test_psql.lo: libglite_lbu_dbtest.la db.h db-int.h db_test.c
db_test_mysql.lo: libglite_lbu_dbtest.la db.h db-int.h db_test.c
db-mysql.lo: db-mysql.c db-int.h db.h
db-pg.lo: db-pg.c db-int.h db.h

.PHONY: default all compile check examples doc stage dist distsrc distbin install clean test_coverage check_soname
