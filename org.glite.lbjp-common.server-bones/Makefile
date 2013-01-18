# defaults
top_srcdir=..
stagedir=.
globalprefix=glite
lbutilsprefix=lbu
package=glite-lbjp-common-server-bones
version=0.0.1
PREFIX=/opt/glite
prefix=

CC=gcc

default: all

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

VPATH=${top_srcdir}/src:${top_srcdir}/examples

SOURCES=\
	examples/*.c examples/*.sh \
	interface/*.h \
	project/.post* \
	src/*.c \
	Makefile
SOURCES_EXEC=examples/*.sh

DEBUG:=-g -O0 -Wall
CFLAGS:=${DEBUG} \
	-I${top_srcdir}/interface \
	-I${stagedir}${prefix}/include \
	-D_GNU_SOURCE ${CFLAGS}
LDFLAGS:=${LDFLAGS} -L${stagedir}${prefix}/${libdir}

ifdef LB_PROF
	CFLAGS:= ${CFLAGS} -pg -g
	LDFLAGS:= ${LDFLAGS} -pg
endif

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}${prefix}/${libdir} ${LDFLAGS}
INSTALL:=libtool --mode=install install

STATICLIB:=libglite_lbu_server_bones.a
LTLIB:=libglite_lbu_server_bones.la

OBJS:=srvbones.o
LOBJS:=${OBJS:.o=.lo}

HDRS:=srvbones.h

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

all: compile

compile: ${STATICLIB} ${LTLIB} example

${STATICLIB}: ${OBJS}
	ar crv $@ ${OBJS}
	ranlib $@

${LTLIB}: ${LOBJS}
	${LINK} ${version_info} -o $@ ${LOBJS} -lglite_lbu_log

stage: compile
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

check: example
	LOG4C_RCPATH=${PREFIX}${sysconfdir}/glite-lb ${top_srcdir}/examples/run_test.sh

example: srv_example cnt_example

srv_example: srv_example.o ${LTLIB}
	${LINK} -o $@ ${LTLIB} srv_example.o

cnt_example: cnt_example.o ${LTLIB}
	${LINK} -o $@ cnt_example.o

doc:

install:
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbutilsprefix}
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/${libdir}
	${INSTALL} -m 755 ${LTLIB} ${DESTDIR}${PREFIX}${prefix}/${libdir}
	${INSTALL} -m 644 ${STATICLIB} ${DESTDIR}${PREFIX}${prefix}/${libdir} ; \
	cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${DESTDIR}${PREFIX}${prefix}/include/${globalprefix}/${lbutilsprefix}

clean:
	rm -rvf *.o *.lo .libs lib* srv_example cnt_example
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf project/changelog Makefile.inc *.spec debian/

%.o %.lo: %.c
	${COMPILE} -c $<

.PHONY: default all compile stage check example doc install clean distclean dist distcheck
