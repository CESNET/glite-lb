top_srcdir=..
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
lbutilsprefix=lbu
package=glite-lbu-log
version=0.0.0
PREFIX=/opt/glite
log4c_prefix=/usr

-include Makefile.inc
-include ../project/version.properties

version=${module.version}

CC=gcc
VPATH=${top_srcdir}/interface:${top_srcdir}/src

LOG4C_CFLAGS:=-I${log4c_prefix}/include
LOG4C_LIBS:=-L${log4c_prefix}/${libdir} -L${log4c_prefix}/lib -llog4c

DEBUG:=-g -O0 -W -Wall

CFLAGS:= \
	${DEBUG} \
	-I${stagedir}/include -I${top_srcdir}/src \
	-I${top_srcdir}/interface \
	${LOG4C_CFLAGS}

LDFLAGS:=-L${stagedir}/${libdir}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/${libdir} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

OBJS:=log.o
LOBJS:=${OBJS:.o=.lo}
LTLIB:=libglite_lbu_log.la

# In order to use libtool versioning correcty, we must have:
#
# current = major + minor + offset
# revision = patch
# age = minor
#
# where offset is a sum of maximal released minor's of all previous major's
# 

# counted minors: n/a
offset=0

version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }

default all: compile

compile: ${LTLIB}

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install: compile
	mkdir -p ${PREFIX}/include/${globalprefix}/${lbutilsprefix} 
	${INSTALL} -m 644 ${top_srcdir}/src/log.h ${PREFIX}/include/${globalprefix}/${lbutilsprefix} 
	mkdir -p ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${top_srcdir}/README ${PREFIX}/share/doc/${package}-${version}
	mkdir -p ${PREFIX}/etc/glite-lb
	${INSTALL} -m 644 ${top_srcdir}/config/log4crc ${PREFIX}/etc/glite-lb/log4crc
	${INSTALL} -m 644 ${top_srcdir}/config/log4crc ${PREFIX}/share/doc/${package}-${version}/log4crc.example-production
	${INSTALL} -m 644 ${top_srcdir}/config/log4crc.debugging ${PREFIX}/share/doc/${package}-${version}/log4crc.example-debugging
	mkdir -p ${PREFIX}/${libdir}
	${INSTALL} -m 755 ${LTLIB} ${PREFIX}/${libdir}/${LTLIB}

clean:
	rm -rvf *.o *.lo .libs lib*
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

check:

${LTLIB}: ${LOBJS}
	${LINK} ${version_info} -o $@ $+ ${LOG4C_LIBS}

%.o %.lo: %.c
	${COMPILE} -c $<
