# defaults
top_srcdir=..
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
globalprefix=glite
package=gss
version=1.0.0
PREFIX=/opt/glite
prefix=

glite_location=/opt/glite
globus_prefix=/opt/globus
cppunit_prefix=/opt/cppunit

CC=gcc

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

VPATH=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/examples

default: all

DEBUG:=-g -O0 -W -Wall -Wno-unused-parameter
# not for globus, gsoap: -Werror

CFLAGS:= ${CFLAGS} ${DEBUG} \
	-DVERSION=\"${version}\" \
	-I. -I${top_srcdir}/interface \
	-I${stagedir}${prefix}/include \
	${COVERAGE_FLAGS} -D_GNU_SOURCE -DDATAGRID_EXTENSION

LDFLAGS:=${LDFLAGS} ${COVERAGE_FLAGS}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS} 
LINKXX:=libtool --mode=link  ${CXX} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS}
INSTALL:=libtool --mode=install install

CPPUNIT_LIBS?=-L${cppunit_prefix}/${libdir} -lcppunit
CPPUNIT_CFLAGS?=-I${cppunit_prefix}/include

GLOBUS_NOTHR_INC:= -I${globus_prefix}/include/${nothrflavour}
GLOBUS_THR_INC:= -I${globus_prefix}/include/${thrflavour}

GLOBUS_NOTHR_LIBS:= -L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} 
GLOBUS_THR_LIBS:= -L${globus_prefix}/lib \
	-lglobus_common_${thrflavour} 

GLOBUS_GSSAPI_GSI_LIBS?=${GLOBUS_NOTHR_LIBS}
GLOBUS_GSSAPI_GSI_CFLAGS?=${GLOBUS_NOTHR_INC}

GLOBUS_GSSAPI_NOTHR_LIBS=-lglobus_gssapi_gsi_${nothrflavour}
GLOBUS_GSSAPI_THR_LIBS=-lglobus_gssapi_gsi_${thrflavour}

KERBEROS_GSSAPI_LIBS=-lgssapi_krb5 
KERBEROS_GSSAPI_CFLAGS=-DNO_GLOBUS_GSSAPI

ifeq ($(gssapi_provider),kerberos)
GSSAPI_NOTHR_LIBS=${KERBEROS_GSSAPI_LIBS} -lssl_${nothrflavour}
GSSAPI_THR_LIBS=${KERBEROS_GSSAPI_LIBS} -lssl_${thrflavour}
GSSAPI_THR_INC=${KERBEROS_GSSAPI_CFLAGS}
GSSAPI_NOTHR_INC=${KERBEROS_GSSAPI_CFLAGS}
else
GSSAPI_NOTHR_LIBS=${GLOBUS_GSSAPI_NOTHR_LIBS}
GSSAPI_THR_LIBS=${GLOBUS_GSSAPI_THR_LIBS}
GSSAPI_THR_INC=
GSSAPI_NOTHR_INC=
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


ifeq ($(thrflavour),)
${GSS_STATICLIB}: ${GSS_OBJS}
	ar crv $@ ${GSS_OBJS}
	ranlib $@

${GSS_LTLIB}: ${GSS_OBJS}
	${LINK} ${version_info} -o $@ ${GSS_LOBJS} ${GLOBUS_NOTHR_LIBS} ${GSSAPI_NOTHR_LIBS} ${LIBCARES_LIBS}
else
${GSS_NOTHRSTATICLIB}: ${GSS_OBJS}
	ar crv $@ ${GSS_OBJS}
	ranlib $@

${GSS_THRSTATICLIB}: ${GSS_THROBJS}
	ar crv $@ ${GSS_THROBJS}
	ranlib $@

${GSS_NOTHRLTLIB}: ${GSS_OBJS}
	${LINK} ${version_info} -o $@ ${GSS_LOBJS} ${GLOBUS_NOTHR_LIBS} ${GSSAPI_NOTHR_LIBS} ${LIBCARES_LIBS}

${GSS_THRLTLIB}: ${GSS_THROBJS}
	${LINK} ${version_info} -o $@ ${GSS_THRLOBJS} ${GLOBUS_THR_LIBS} ${GSSAPI_THR_LIBS} ${LIBCARES_LIBS}
endif


all compile: $(lib_LTLIBRARIES) ${lib_LIBRARIES} examples

check: compile check.gss

check.gss: test_gss
	# ./test_gss out.xml
	echo test_gss not run automatically util we have got some credentials in X509_USER_PROXY

test_gss: test_gss.o
	${LINKXX} -o $@ test_gss.o ${default_gss} ${CPPUNIT_LIBS} ${GLOBUS_GSSAPI_GSI_LIBS} ${LIBCARES_LIBS}

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
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/glite/security
	${INSTALL} -m 755 ${lib_LTLIBRARIES} ${DESTDIR}${PREFIX}${prefix}/${libdir}
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	cd ${top_srcdir}/interface && ${INSTALL} -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/glite/security/
	if [ x${DOSTAGE} = xyes ]; then \
		install -m 644 ${lib_LIBRARIES} ${DESTDIR}${PREFIX}${prefix}/${libdir}; \
	fi

clean:
	rm -rf *.o *.lo *.a *.la .libs test_gss gss_server gss_client

distclean:
	rm -rvf Makefile.inc *.spec debian/

%.o: %.c
	${COMPILE} ${GLOBUS_NOTHR_INC} ${GSSAPI_NOTHR_INC} ${LIBCARES_CFLAGS} -o $@ -c $<

%.thr.o: %.c
	${COMPILE} ${GLOBUS_THR_INC} ${GSSAPI_THR_INC} ${LIBCARES_CFLAGS} -o $@ -c $<

test_gss.o: %.o: %.cpp
	${CXX} -c ${CFLAGS} ${GLOBUS_GSSAPI_GSI_CFLAGS} ${CPPUNIT_CFLAGS} -Wno-error $<
