# defaults
top_srcdir=..
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
globalprefix=glite
package=gsoap-plugin
version=1.0.0
PREFIX=/opt/glite
prefix=

glite_location=/opt/glite
gsoap_prefix=/opt/gsoap

CC=gcc

gsoap_versions_noflavours?=2.6.2 2.7.0f 2.7.6b 2.7.6d 2.7.9d 2.7.9b
gsoap_versions_flavours?=2.7.10

-include Makefile.inc
-include ../Makefile.inc
-include ../project/version.properties
-include ../../project/version.properties

version=${module.version}
gsoap_versions_flavours:=${shell if ! echo ${gsoap_versions_flavours} | grep '\<${gsoap_default_version}\>' >/dev/null; then echo "${gsoap_default_version} "; else echo ''; fi} ${gsoap_versions_flavours}

GSPLUGIN_DEBUG?=no
GSPLUGIN_VERSION_CHECKING?=yes
CC=gcc

# In order to use libtool versioning correcty, we must have:
#
# current = major + minor + offset
# revision = patch
# age = minor
#
# where offset is a sum of maximal released minor's of all previous major's
# 

# counted minors: 1.5, 2.1
offset=6
version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

ext_repository:=${shell if [ -d "${repository}/externals" ]; then \
	echo "${repository}/externals"; \
	else echo "${repository}"; \
	fi}

gsoap_version=${gsoap_default_version}
gsoap_prefix?=${ext_repository}/${gsoap_name}/${gsoap_version}/${gsoap_platform}
ifdef lang
langflavour:=_${lang}
langsuffix:=.${lang}
endif

VPATH=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/examples

default: all

DEBUG:=-g -O0 -W -Wall -Wno-unused-parameter
# not for globus and gsoap: -Werror
ifeq ($(GSPLUGIN_DEBUG),yes)
	DEBUG:=${DEBUG} -DGSPLUGIN_DEBUG=1
endif
# gsoap logs: -DDEBUG
ifeq ($(GSOAP_DEBUG),yes)
	DEBUG:=${DEBUG} -DDEBUG=1
endif

#ifeq ($(GSPLUGIN_VERSION_CHECKING),yes)
#	DEBUG:=${DEBUG} -DCHECK_GSOAP_VERSION
#endif

CFLAGS:= ${CFLAGS} ${DEBUG} \
	-DVERSION=\"${version}\" \
	-DWITH_NONAMESPACES \
	-I. -I${top_srcdir}/interface \
	-I${stagedir}${prefix}/include \
	${COVERAGE_FLAGS} -D_GNU_SOURCE -DDATAGRID_EXTENSION -DWITH_IPV6

LDFLAGS:=${LDFLAGS} ${COVERAGE_FLAGS}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
COMPILEXX:=libtool --mode=compile ${CXX} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS}
LINKXX:=libtool --mode=link ${CXX} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS}
INSTALL:=libtool --mode=install install
LINK_lang:=PATH=${top_srcdir}/project/libtoolhack:${PATH} libtool --mode=link ${CXX} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS}
ifeq (${lang},c)
LINK_lang:=${LINK}
endif
ifeq (${lang},cxx)
LINK_lang:=${LINKXX}
endif

gsoaplibdir=$(shell if test -d "${gsoap_prefix}/${libdir}"; then echo "${gsoap_prefix}/${libdir}"; else echo "${gsoap_prefix}/lib"; fi)
GSOAP_LIBS?=-L${gsoaplibdir} -lgsoap

EX_NOTHRLIBS:=-L${stagedir}${prefix}/${libdir} -lglite_security_gss_${nothrflavour}
EX_THRLIBS:=-L${stagedir}${prefix}/${libdir} -lglite_security_gss_${thrflavour}
EX_LIBS:=-L${stagedir}${prefix}/${libdir} -lglite_security_gss
HDRS:=glite_gsplugin.h glite_gscompat.h glite_gsplugin-int.h
EXAMPLES:=wscalc_clt_ex wscalc_srv_ex wscalc_srv_ex2
TESTS:=test_cxx

NOTHRSTATICLIB:=libglite_security_gsoap_plugin_${nothrflavour}${langflavour}.a
THRSTATICLIB:=libglite_security_gsoap_plugin_${thrflavour}${langflavour}.a
STATICLIB:=libglite_security_gsoap_plugin${langflavour}.a
NOTHRLTLIB:=libglite_security_gsoap_plugin_${nothrflavour}${langflavour}.la
THRLTLIB:=libglite_security_gsoap_plugin_${thrflavour}${langflavour}.la
LTLIB:=libglite_security_gsoap_plugin${langflavour}.la

OBJS:=${GSS_OBJS} glite_gsplugin${langsuffix}.o
LOBJS:=${OBJS:.o=.lo}
THROBJS:=${OBJS:.o=.thr.o}
THRLOBJS:=${OBJS:.o=.thr.lo}

dotless_ver:=${shell echo ${gsoap_version} | tr -d '.'}
NOTHRSTATICLIB_S:=libglite_security_gsoap_plugin_${dotless_ver}_${nothrflavour}${langflavour}.a
THRSTATICLIB_S:=libglite_security_gsoap_plugin_${dotless_ver}_${thrflavour}${langflavour}.a
STATICLIB_S:=libglite_security_gsoap_plugin_${dotless_ver}${langflavour}.a
NOTHRLTLIB_S:=libglite_security_gsoap_plugin_${dotless_ver}_${nothrflavour}${langflavour}.la
THRLTLIB_S:=libglite_security_gsoap_plugin_${dotless_ver}_${thrflavour}${langflavour}.la
LTLIB_S:=libglite_security_gsoap_plugin_${dotless_ver}${langflavour}.la

ifeq (${thrflavour},)
lib_LTLIBRARIES:=${STATICLIB} ${LTLIB}
lib_LTLIBRARIES_S:=${LTLIB_S}
LTLIB_TEST_S:=libglite_security_gsoap_plugin_${dotless_ver}_cxx.la
default_ltlib_s:=${LTLIB_S}
else
lib_LTLIBRARIES:=${NOTHRSTATICLIB} ${NOTHRLTLIB} ${THRSTATICLIB} ${THRLTLIB}
lib_LTLIBRARIES_S:=${NOTHRLTLIB_S} ${THRLTLIB_S}
LTLIB_TEST_S:=libglite_security_gsoap_plugin_${dotless_ver}_${nothrflavour}_cxx.la
default_ltlib_s:=${NOTHRLTLIB_S}
endif

ifeq (${gsoap_version},${gsoap_default_version})
OBJS_S:=glite_gsplugin${langsuffix}.o
GSOAP_DEFAULT_LIBS:=${GSOAP_LIBS}
else
OBJS_S:=glite_gsplugin${langsuffix}.o stdsoap2${langsuffix}.o
GSOAP_DEFAULT_LIBS:=
endif
LOBJS_S:=${OBJS_S:.o=.lo}
THROBJS_S:=${OBJS_S:.o=.thr.o}
THRLOBJS_S:=${OBJS_S:.o=.thr.lo}

ifeq (${thrflavour},)
${STATICLIB}: ${THROBJS}
	ar crv $@ ${THROBJS}
	ranlib $@

${LTLIB}: ${THROBJS}
	${LINK_lang} ${version_info} -o $@ ${THRLOBJS} ${EX_LIBS}

${STATICLIB_S}: ${THROBJS_S}
	ar crv $@ ${THROBJS_S}
	ranlib $@

${LTLIB_S}: ${THROBJS_S}
	${LINK_lang} ${version_info} -o $@ ${LOBJS_S} ${EX_LIBS} ${GSOAP_DEFAULT_LIBS}
else
${NOTHRSTATICLIB}: ${OBJS}
	ar crv $@ ${OBJS}
	ranlib $@

${THRSTATICLIB}: ${THROBJS}
	ar crv $@ ${THROBJS}
	ranlib $@

${NOTHRLTLIB}: ${OBJS} 
	${LINK_lang} ${version_info} -o $@ ${LOBJS} ${EX_NOTHRLIBS}

${THRLTLIB}: ${THROBJS}
	${LINK_lang} ${version_info} -o $@ ${THRLOBJS} ${EX_THRLIBS}

${NOTHRSTATICLIB_S}: ${OBJS_S}
	ar crv $@ ${OBJS_S}
	ranlib $@

${THRSTATICLIB_S}: ${THROBJS_S}
	ar crv $@ ${THROBJS_S}
	ranlib $@

${NOTHRLTLIB_S}: ${OBJS_S} 
	${LINK_lang} ${version_info} -o $@ ${LOBJS_S} ${EX_NOTHRLIBS} ${GSOAP_DEFAULT_LIBS}

${THRLTLIB_S}: ${THROBJS_S}
	${LINK_lang} ${version_info} -o $@ ${THRLOBJS_S} ${EX_THRLIBS} ${GSOAP_DEFAULT_LIBS}
endif

all compile: \
	${lib_LTLIBRARIES} \
	all-libs-with-soap examples

check: check.cxx

# C/C++ compatibility test
# (C flavour, gsoap 2.7.10 ==> C++ test fails)
check.cxx:
	${MAKE} lang=cxx -f ${top_srcdir}/Makefile ${LTLIB_TEST_S} test_cxx
	LD_LIBRARY_PATH=${cares_prefix}/${libdir}:${LD_LIBRARY_PATH} ./test_cxx

examples: ${EXAMPLES}

all-libs-with-soap:
	for v in ${gsoap_versions_noflavours}; do	\
		dir=`echo $$v | tr -d .`; \
		mkdir $$dir; \
		${MAKE} -C $$dir -f ../Makefile gsoap_version=$$v top_srcdir=../.. lang= libs-with-soap || exit $?; \
	done
	for v in ${gsoap_versions_flavours}; do	\
		dir=`echo $$v | tr -d .`; \
		mkdir $$dir; \
		${MAKE} -C $$dir -f ../Makefile gsoap_version=$$v top_srcdir=../.. lang=c libs-with-soap || exit $?; \
		${MAKE} -C $$dir -f ../Makefile gsoap_version=$$v top_srcdir=../.. lang=cxx libs-with-soap || exit $?; \
	done

libs-with-soap: ${lib_LTLIBRARIES_S}

gsoap_srcname=gsoap-`echo ${gsoap_version} | cut -d. -f1,2`

link-gsoap:
	if [ ${gsoap_version} = ${gsoap_default_version} ]; then \
		ln -sf ${gsoap_prefix}/include/stdsoap2.h stdsoap2.h; \
	elif [ -f ${top_srcdir}/src/stdsoap2_${gsoap_version}.c ]; then \
		ln -sf ${top_srcdir}/src/stdsoap2_${gsoap_version}.c stdsoap2.c; \
		ln -sf ${top_srcdir}/src/stdsoap2_${gsoap_version}.h stdsoap2.h; \
	elif [ -f ${ext_repository}/${gsoap_name}/${gsoap_version}/src/stdsoap2.c ]; then \
		ln -sf ${ext_repository}/${gsoap_name}/${gsoap_version}/src/stdsoap2.[ch] .; \
	elif [ -f ${ext_repository}/${gsoap_name}/${gsoap_version}/${gsoap_platform}/stdsoap2.c ]; then \
		ln -sf ${ext_repository}/${gsoap_name}/${gsoap_version}/${gsoap_platform}/stdsoap2.[ch] .; \
	elif [ -f ${ext_repository}/${gsoap_name}/${gsoap_version}/${gsoap_platform}/src/stdsoap2.c ]; then \
		ln -sf ${ext_repository}/${gsoap_name}/${gsoap_version}/${gsoap_platform}/src/stdsoap2.c .; \
		ln -sf ${ext_repository}/${gsoap_name}/${gsoap_version}/${gsoap_platform}/include/stdsoap2.h .; \
	elif [ -f ${ext_repository}/${gsoap_name}/${gsoap_version}/src/stdsoap2.c ]; then \
		ln -sf ${ext_repository}/${gsoap_name}/${gsoap_version}/src/stdsoap2.[ch] .; \
	elif [ -f ${ext_repository}/${gsoap_name}/${gsoap_version}/src/${gsoap_srcname}/soapcpp2/stdsoap2.c ]; then \
		ln -sf ${ext_repository}/${gsoap_name}/${gsoap_version}/src/${gsoap_srcname}/soapcpp2/stdsoap2.[ch] .; \
	else false ; \
	fi

stdsoap2.c stdsoap2.h: link-gsoap

GSOAP_FPREFIX:= GSOAP_

WSCALC_CLT_OBJS = wscalc_clt_ex.o ${GSOAP_FPREFIX}C.o ${GSOAP_FPREFIX}Client.o
WSCALC_SRV_OBJS = wscalc_srv_ex.o ${GSOAP_FPREFIX}C.o ${GSOAP_FPREFIX}Server.o
WSCALC_SRV2_OBJS = wscalc_srv_ex2.o ${GSOAP_FPREFIX}C.o ${GSOAP_FPREFIX}Server.o

wscalc_clt_ex.o wscalc_srv_ex.o wscalc_srv_ex2.o: ${GSOAP_FPREFIX}H.h

wscalc_clt_ex: ${WSCALC_CLT_OBJS} ${default_ltlib_s}
	${LINK_lang} -o $@ ${WSCALC_CLT_OBJS} ${default_ltlib_s} ${GSOAP_DEFAULT_LIBS}

wscalc_srv_ex: ${WSCALC_SRV_OBJS} ${default_ltlib_s}
	${LINK_lang} -o $@ ${WSCALC_SRV_OBJS} ${default_ltlib_s} ${GSOAP_DEFAULT_LIBS}

wscalc_srv_ex2: ${WSCALC_SRV2_OBJS} ${default_ltlib_s}
	${LINK_lang} -o $@ ${WSCALC_SRV2_OBJS} ${default_ltlib_s} ${GSOAP_DEFAULT_LIBS}

test_cxx: test_gsplugin_cxx.o ${GSOAP_FPREFIX}C.cxx.o
	${LINKXX} -o $@ $+ ${LTLIB_TEST_S}

soapcpp:=${shell if [ -x ${gsoap_prefix}/bin/soapcpp2 ]; then \
	echo ${gsoap_prefix}/bin/soapcpp2; \
	else echo ${gsoap_prefix}/soapcpp2; \
	fi}

${GSOAP_FPREFIX}H.h ${GSOAP_FPREFIX}C.c ${GSOAP_FPREFIX}Server.c ${GSOAP_FPREFIX}Client.c ${GSOAP_FPREFIX}ServerLib.c ${GSOAP_FPREFIX}ClientLib.c: calc.h.S
	${soapcpp} -c -p ${GSOAP_FPREFIX} ${top_srcdir}/examples/calc.h.S

wscalc_clt_ex.o: wscalc_clt_ex.c
	${CC} -c ${CFLAGS} -o $@ $<

doc:

stage:
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

install:
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/glite/security
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	-mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/glite/security/
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${DESTDIR}${PREFIX}${prefix}/share/doc/${package}-${version}
	cd ${top_srcdir}/interface && ${INSTALL} -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/glite/security/
	for v in ${gsoap_versions_noflavours}; do \
		${MAKE} -C `echo $$v | tr -d .` -f ../Makefile install-soaplib gsoap_version=$$v lang= PREFIX=${PREFIX} top_srcdir=${top_srcdir}/.. || exit $?; \
	done
	for v in ${gsoap_versions_flavours}; do \
		${MAKE} -C `echo $$v | tr -d .` -f ../Makefile install-soaplib gsoap_version=$$v lang=c PREFIX=${PREFIX} top_srcdir=${top_srcdir}/.. || exit $?; \
		${MAKE} -C `echo $$v | tr -d .` -f ../Makefile install-soaplib gsoap_version=$$v lang=cxx PREFIX=${PREFIX} top_srcdir=${top_srcdir}/.. || exit $?; \
	done

install-soaplib:
	${INSTALL} -m 755 ${lib_LTLIBRARIES_S} ${DESTDIR}${PREFIX}${prefix}/${libdir}

clean:
	rm -rvf ${lib_LTLIBRARIES}
	rm -rvf ${lib_LTLIBRARIES_S}
	rm -rvf ${LTLIB_TEST_S}
	rm -rvf *.c *.h *.xml *.nsmap *.o *.lo .libs ${EXAMPLES} ${TESTS}
	rm -rvf CalcService.wsdl wscalc.xsd
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/ debian/
	for v in ${gsoap_versions_noflavours} ${gsoap_versions_flavours}; do \
		rm -rvf `echo $$v | tr -d .`; \
	done

%.c.o: %.c
	${COMPILE} -o $@ -c $<

%.cxx.o: %.c
	${COMPILEXX} -o $@ -c $<

%.o: %.c
	${COMPILE} -o $@ -c $<

%.c.thr.o: %.c
	${COMPILE} -o $@ -c $<

%.cxx.thr.o: %.c
	${COMPILEXX} -o $@ -c $<

%.thr.o: %.c
	${COMPILE} -o $@ -c $<

test_gsplugin_cxx.o: %.o: %.cpp stdsoap2.h
	${COMPILEXX} -c ${GLOBUS_INC} $<

glite_gsplugin${langsuffix}.o glite_gsplugin${langsuffix}.thr.o: soap_version.h

soap_version.h: stdsoap2.h
	head stdsoap2.h | \
	perl -ne '/^\s*stdsoap2.h\s+([0-9])\.([0-9])\.(\S+)\s.*/ && printf "#define GSOAP_VERSION %d%02d%02d\n#define GSOAP_MIN_VERSION \"$$3\"\n#ident \"soap_version.h $$1.$$2.$$3\"\n",$$1,$$2,$$3' >$@
