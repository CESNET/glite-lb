# Default values
top_srcdir=..
stagedir=.
globalprefix=glite
jobidprefix=jobid
package=glite-jobid-api-c
version=0.0.0

CC:=gcc
CXX:=g++

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/interface:${top_srcdir}/test

DEBUG:=-g -O0 -Wall

CFLAGS:=${DEBUG} \
	-I${top_srcdir}/interface -I${top_srcdir}/src \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
CXXCOMPILE:=libtool --mode=compile ${CXX} ${CXXFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
INSTALL:=libtool --mode=install install

LIBOBJS:=cjobid.o strmd5.o 
HDRS:=cjobid.h strmd5.h

LIBLOBJS:=${LIBOBJS:.o=.lo}

LIB:=libglite_jobid.la

TEST_LIBS:=-L${cppunit_prefix}/${libdir} -lcppunit -ldl
TEST_INC:=-I${cppunit_prefix}/include

compile all: ${LIB}

# In order to use libtool versioning correcty, we must have:
#
# current = major + minor + offset
# revision = patch
# age = minor
#
# where offset is a sum of maximal released minor's of all previous major's
# 

# counted minors: 1.0
offset=0
version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split /\./,"${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

${LIB}: ${LIBOBJS}
	${LINK} ${version_info} -o $@ ${LIBLOBJS} -rpath ${stagedir}${prefix}/${libdir}

check: compile base64_test
	./base64_test base64_test.xml

base64_test: %: %.cpp compile
	${CXX} -c ${CFLAGS} ${TEST_INC} $<
	${LINKXX} -o $@ $@.o ${LIB} ${TEST_LIBS}

clean:
	rm -rvf *.o *.lo .libs/ lib* *.c *.cpp *.h
	rm -vf base64_test base64_test.xml

distclean:
	rm -rvf Makefile.inc *.spec debian/

stage:
	$(MAKE) install PREFIX=${stagedir}

install: ${LIB}
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${jobidprefix}
	${INSTALL} -m 644 ${LIB} ${DESTDIR}${PREFIX}${prefix}/${libdir}
	(cd ${top_srcdir}/interface; ${INSTALL} -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${jobidprefix})


%.o: %.c
	${COMPILE} -o $@ -c $< 


.PHONY: default all compile check stage install clean distclean
