# defaults
top_srcdir=..
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
globalprefix=glite
package=glite-lbjp-common-gss
version=1.0.0
PREFIX=/opt/glite
prefix=

glite_location=/opt/glite
globus_prefix=/opt/globus
cppunit_prefix=/opt/cppunit

CC=gcc

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

# In order to use libtool versioning correcty, we must have:
#
# current = major + minor + offset
# revision = patch
# age = minor
#
# where offset is a sum of maximal released minor's of all previous major's
#

# counted minors: gsoap-plugin 1.5, gss 2.1
offset=6
version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

VPATH=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/examples:${top_srcdir}/project

SOURCES=\
	examples/*.c examples/*.h \
	interface/*.h \
	project/.post* project/*.pc.in \
	project/doc/*.cls project/doc/*.pdf project/doc/*.tex project/doc/Makefile \
	src/*.c \
	test/*.cpp \
	Makefile

DEBUG:=-g -O0 -W -Wall -Wno-unused-parameter
# not for globus, gsoap: -Werror

CFLAGS:= ${DEBUG} \
	-DVERSION=\"${version}\" \
	-I. -I${top_srcdir}/interface \
	-I${stagedir}${prefix}/include \
	${COVERAGE_FLAGS} -D_GNU_SOURCE -DDATAGRID_EXTENSION ${CFLAGS}

LDFLAGS:=${LDFLAGS} ${COVERAGE_FLAGS}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS}
SOLINK:=libtool --mode=link ${CC} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS}
LINKXX:=libtool --mode=link  ${CXX} ${LDFLAGS}
INSTALL:=libtool --mode=install install

CPPUNIT_LIBS?=-L${cppunit_prefix}/${libdir} -lcppunit
CPPUNIT_CFLAGS?=-I${cppunit_prefix}/include

GLOBUS_NOTHR_INC:= -I${globus_prefix}/include/${nothrflavour}
GLOBUS_THR_INC:= -I${globus_prefix}/include/${thrflavour}

GLOBUS_NOTHR_LIBS:= -L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} 
GLOBUS_THR_LIBS:= -L${globus_prefix}/lib \
	-lglobus_common_${thrflavour} 

GLOBUS_GSSAPI_NOTHR_LIBS=-lglobus_gssapi_gsi_${nothrflavour}
GLOBUS_GSSAPI_THR_LIBS=-lglobus_gssapi_gsi_${thrflavour}

KERBEROS_GSSAPI_LIBDIR?=${kerberos_prefix}/lib
KERBEROS_GSSAPI_INCDIR?=${kerberos_prefix}/include
KERBEROS_GSSAPI_LIBNAME?=gssapi_krb5

ifeq (${kerberos_prefix}, /usr)
KERBEROS_GSSAPI_LIBS?=-l${KERBEROS_GSSAPI_LIBNAME}
else
KERBEROS_GSSAPI_LIBS?=-L${KERBEROS_GSSAPI_LIBDIR} -l${KERBEROS_GSSAPI_LIBNAME}
endif
KERBEROS_GSSAPI_CFLAGS?=-I${KERBEROS_GSSAPI_INCDIR}

GLOBUS_GSSAPI_GSI_LIBS?=${GLOBUS_THR_LIBS} ${GLOBUS_GSSAPI_THR_LIBS}
GLOBUS_GSSAPI_GSI_CFLAGS?=${GLOBUS_THR_INC}

GLOBUS_COMMON_LIBS?=${GLOBUS_THR_LIBS}
GLOBUS_COMMON_CFLAGS?=${GLOBUS_THR_INC}

ifeq ($(gssapi_provider),kerberos)
GSSAPI_THR_LIBS=${GLOBUS_COMMON_LIBS} ${KERBEROS_GSSAPI_LIBS} -lssl
GSSAPI_THR_INC=${GLOBUS_COMMON_CFLAGS} ${KERBEROS_GSSAPI_CFLAGS} -DNO_GLOBUS_GSSAPI
GSSAPI_NOTHR_LIBS=${GLOBUS_NOTHR_LIBS} ${KERBEROS_GSSAPI_LIBS} -lssl
GSSAPI_NOTHR_INC=${GLOBUS_NOTHR_INC} ${KERBEROS_GSSAPI_CFLAGS} -DNO_GLOBUS_GSSAPI
GSSAPI_PROVIDER_PKG=
else
GSSAPI_THR_LIBS=${GLOBUS_GSSAPI_GSI_LIBS}
GSSAPI_THR_INC=${GLOBUS_GSSAPI_GSI_CFLAGS}
GSSAPI_NOTHR_LIBS=${GLOBUS_NOTHR_LIBS} ${GLOBUS_GSSAPI_NOTHR_LIBS}
GSSAPI_NOTHR_INC=${GLOBUS_NOTHR_INC}
GSSAPI_PROVIDER_PKG=globus-gssapi-gsi
endif

LIBCARES_CFLAGS?=-I${cares_prefix}/include
LIBCARES_LIBS?=-L${cares_prefix}/${libdir} -L${cares_prefix}/lib -lcares

HDRS:=glite_gss.h

GSS_OBJS:=glite_gss.o
GSS_LOBJS:=${GSS_OBJS:.o=.lo}
GSS_THROBJS:=${GSS_OBJS:.o=.thr.o}
GSS_THRLOBJS:=${GSS_OBJS:.o=.thr.lo}

GSS_NOTHRSTATICLIB:=libglite_security_gss_${nothrflavour}.a
GSS_THRSTATICLIB:=libglite_security_gss_${thrflavour}.a
GSS_STATICLIB:=libglite_security_gss.a
GSS_NOTHRLTLIB:=libglite_security_gss_${nothrflavour}.la
GSS_THRLTLIB:=libglite_security_gss_${thrflavour}.la
GSS_LTLIB:=libglite_security_gss.la

ifeq ($(thrflavour),)
lib_LTLIBRARIES=${GSS_LTLIB}
lib_LIBRARIES=${GSS_STATICLIB}
default_gss=${GSS_LTLIB}
else
lib_LTLIBRARIES=${GSS_NOTHRLTLIB} ${GSS_THRLTLIB}
lib_LIBRARIES=${GSS_NOTHRSTATICLIB} ${GSS_THRSTATICLIB}
default_gss=${GSS_NOTHRLTLIB}
endif

PKG_CONFIG_FILES=gss.pc

ifeq ($(thrflavour),)
${GSS_STATICLIB}: ${GSS_THROBJS}
	ar crv $@ ${GSS_THROBJS}
	ranlib $@

${GSS_LTLIB}: ${GSS_THROBJS}
	${SOLINK} ${version_info} -o $@ ${GSS_THRLOBJS} ${GSSAPI_THR_LIBS} ${LIBCARES_LIBS} -lpthread
else
${GSS_NOTHRSTATICLIB}: ${GSS_OBJS}
	ar crv $@ ${GSS_OBJS}
	ranlib $@

${GSS_THRSTATICLIB}: ${GSS_THROBJS}
	ar crv $@ ${GSS_THROBJS}
	ranlib $@

${GSS_NOTHRLTLIB}: ${GSS_OBJS}
	${SOLINK} ${version_info} -o $@ ${GSS_LOBJS} ${GSSAPI_NOTHR_LIBS} ${LIBCARES_LIBS}

${GSS_THRLTLIB}: ${GSS_THROBJS}
	${SOLINK} ${version_info} -o $@ ${GSS_THRLOBJS} ${GSSAPI_THR_LIBS} ${LIBCARES_LIBS} -lpthread
endif


all compile: $(lib_LTLIBRARIES) ${lib_LIBRARIES} examples ${PKG_CONFIG_FILES}

check: compile check.gss

check.gss: test_gss
	# ./test_gss out.xml
	echo test_gss not run automatically util we have got some credentials in X509_USER_PROXY

test_gss: test_gss.o
	${LINKXX} -o $@ test_gss.o ${default_gss} ${CPPUNIT_LIBS} ${LIBCARES_LIBS}

test_coverage:
	-mkdir coverage
	cd coverage && $(MAKE) -f ../Makefile top_srcdir=../../ COVERAGE_FLAGS="-fprofile-arcs -ftest-coverage" check
	cd coverage && for i in ${OBJS}; do gcov -o .libs/ $$i ; done

gss_server: gss_server.o example_common.o
	${LINK} -o $@ ${default_gss} $^ 

gss_client: gss_client.o example_common.o
	${LINK} -o $@ ${default_gss} $^

examples:

doc:

stage:
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

install:
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}/pkgconfig
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/glite/security
	${INSTALL} -m 755 ${lib_LTLIBRARIES} ${DESTDIR}${PREFIX}${prefix}/${libdir}
	cd ${top_srcdir}/interface && ${INSTALL} -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/glite/security/
	if [ x${DOSTAGE} = xyes ]; then \
		install -m 644 ${lib_LIBRARIES} ${DESTDIR}${PREFIX}${prefix}/${libdir}; \
	fi
	${INSTALL} -m 644 gss.pc ${DESTDIR}${PREFIX}${prefix}/${libdir}/pkgconfig/glite-security-gss.pc

clean:
	rm -rf *.o *.lo *.a *.la .libs test_gss gss_server gss_client *.pc
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

%.o: %.c
	${COMPILE} ${GSSAPI_NOTHR_INC} ${LIBCARES_CFLAGS} -o $@ -c $<

%.thr.o: %.c
	${COMPILE} -D_REENTRANT -DGLITE_LBU_THREADED ${GSSAPI_THR_INC} ${LIBCARES_CFLAGS} -o $@ -c $<

%.pc: %.pc.in
	sed -e "s,@PREFIX@,${PREFIX}${prefix}," -e "s/@VERSION@/${version}/" -e "s/@LIBDIR@/${libdir}/" -e "s/@GSSAPI_PROVIDER@/${GSSAPI_PROVIDER_PKG}/" $< > $@

test_gss.o: %.o: %.cpp
	${CXX} -c ${CFLAGS} ${GLOBUS_GSSAPI_GSI_CFLAGS} ${CPPUNIT_CFLAGS} -Wno-error $<

.PHONY: default all compile check check.gss test_coverage examples doc stage install clean distclean dist distcheck
