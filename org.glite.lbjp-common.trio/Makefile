# Default values
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
package=glite-lb-utils-trio
version=0.0.0
PREFIX=/opt/glite
lbuprefix=lbu

-include Makefile.inc
-include ../project/version.properties

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/test::${top_srcdir}/doc

CC=gcc

DEBUG:=-g -O0 -Wall

CFLAGS:=${DEBUG} -I${top_srcdir}/src -I${top_srcdir}/interface -I. -DDATAGRID_EXTENSION

COMPILE:=libtool --mode=compile ${CC}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/lib ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} -rpath ${stagedir}/lib ${LDFLAGS} 
INSTALL:=libtool --mode=install install

REPORTS:=${top_srcdir}/reports
TEST_LIBS:=-L${cppunit_prefix}/lib -lcppunit -ldl
TEST_INC:=-I${cppunit_prefix}/include


OBJS:=trio.o strio.o escape.o
LOBJS:=${OBJS:.o=.lo}

HDRS:=trio.h escape.h

STATICLIB:=libglite_lbu_trio.a
LTLIB:=libglite_lbu_trio.la

default: all

all compile: ${STATICLIB} ${LTLIB}

# to use libtool versioning correcty, we should have:
#
# current = major + minor + offset
# revision = patch
# age = minor
#
# where offset is a sum of maximal released minor's of all previous major's
# 
# version_info=-version-info `echo ${version} | cut -d. -f1,2 | tr . :`

# counted minors: n/a
offset=0

version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

${STATICLIB}: ${OBJS}
	ar crv $@ ${OBJS}
	ranlib $@

${LTLIB}: ${OBJS}
	${LINK} ${version_info} -o $@ ${LOBJS} 

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

check: compile mkreports runtest

mkreports:
	-mkdir ${REPORTS}

runtest: trio_test
	./trio_test ${REPORTS}/trio.xml

trio_test: trio_test.cpp
	${CXX} -c ${CFLAGS} ${TEST_INC} $<
	${LINKXX} -o $@ trio_test.o ${LTLIB} ${TEST_LIBS}
	

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
	mkdir -p ${PREFIX}/lib
	mkdir -p ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${LTLIB} ${PREFIX}/lib
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	if [ x${DOSTAGE} = xyes ]; then \
		mkdir -p ${PREFIX}/include/${globalprefix}/${lbuprefix} ; \
		(cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${lbuprefix}) ; \
		install -m 644 ${STATICLIB} ${PREFIX}/lib; \
	fi

clean:

%.o: %.c
	${COMPILE} ${CFLAGS} -c $<
