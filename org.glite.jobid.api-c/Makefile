# Default values
top_srcdir=..
stagedir=.
globalprefix=glite
jobidprefix=jobid
package=glite-jobid-api-c
version=0.0.0

CC:=gcc
CXX:=g++

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/interface:${top_srcdir}/test

DEBUG:=-g -O0 -Wall

CFLAGS:=${DEBUG} \
	-I${top_srcdir}/interface -I${top_srcdir}/src \
	${COVERAGE_FLAGS} \
	-D_GNU_SOURCE ${CFLAGS}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
CXXCOMPILE:=libtool --mode=compile ${CXX} ${CXXFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
INSTALL:=libtool --mode=install install

SOURCES=interface/*.h src/*.c src/*.h project/.post* test/*.cpp Makefile

LIBOBJS:=cjobid.o strmd5.o 
HDRS:=cjobid.h strmd5.h

LIBLOBJS:=${LIBOBJS:.o=.lo}

LIB:=libglite_jobid.la

os=${shell uname}
DL_LIBS:=
ifeq (${os},Linux)
	DL_LIBS:=-ldl
endif
TEST_LIBS:=-L${cppunit_prefix}/${libdir} -lcppunit ${DL_LIBS}
TEST_INC:=-I${cppunit_prefix}/include
LIBCRYPTO_LIBS:=-lcrypto
LIBCRYPTO_CFLAGS:=

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
	${LINK} ${version_info} -o $@ ${LIBLOBJS} -rpath ${stagedir}${prefix}/${libdir} ${LIBCRYPTO_LIBS}

check: compile base64_test
	./base64_test base64_test.xml

base64_test: %: %.cpp compile
	${CXX} -c ${CFLAGS} ${TEST_INC} $<
	${LINKXX} -o $@ $@.o ${LIB} ${TEST_LIBS}

clean:
	rm -rvf *.o *.lo .libs/ lib* *.c *.cpp *.h
	rm -vf base64_test base64_test.xml
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

stage:
	$(MAKE) install PREFIX=${stagedir}

install: ${LIB}
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${jobidprefix}
	${INSTALL} -m 755 ${LIB} ${DESTDIR}${PREFIX}${prefix}/${libdir}
	(cd ${top_srcdir}/interface; ${INSTALL} -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${jobidprefix})

b64r.c: ${top_srcdir}/gen-b64r.pl
	${top_srcdir}/gen-b64r.pl > $@

strmd5.o: b64r.c strmd5.c

%.lo %.o: %.c
	${COMPILE} -o $@ -I. -c $<

.PHONY: default all compile check stage install clean distclean dist distcheck
