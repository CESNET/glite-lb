# Default values
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-client
version=0.0.0
PREFIX=/opt/glite

glite_location=/opt/glite
globus_prefix=/opt/globus
nothrflavour=gcc32
thrflavour=gcc32pthr
expat_prefix=/opt/expat
ares_prefix=/opt/ares

-include Makefile.inc

VPATH=${top_srcdir}/src:${top_srcdir}/test
AT3=perl -I${top_srcdir}/project ${top_srcdir}/project/at3

SUFFIXES = .T 

DEBUG:=-g -O0 -Wall

GLOBUSINC:= -I${globus_prefix}/include/${nothrflavour}
GLOBUSTHRINC:= -I${globus_prefix}/include/${thrflavour}

CFLAGS:=${DEBUG} \
	-I${top_srcdir}/src -I${top_srcdir}/interface \
	-I${stagedir}/include \
	-I${glite_location}/include \
	-I${expat_prefix}/include \
	-I${ares_prefix}/include \
	${COVERAGE_FLAGS} 


CXXFLAGS:=${CFLAGS}

GLOBUS_LIBS:=-L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lglobus_gssapi_gsi_${nothrflavour} \

GLOBUS_THRLIBS:=-L${globus_prefix}/lib \
	-lglobus_common_${thrflavour} \
	-lglobus_gssapi_gsi_${thrflavour}

EXPAT_LIBS:=-L${expat_prefix}/lib \
	-lexpat

ARES_LIBS:=-L${ares_prefix}/lib \
	-lares

EXT_LIB:= ${EXPAT_LIBS} \
    ${ARES_LIBS}

TEST_LIBS:=-L${cppunit_prefix}/lib -lcppunit
TEST_INC:=-I${cppunit_prefix}/include

LDFLAGS:=-L${stagedir}/lib \
	${COVERAGE_FLAGS}

COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
CXXCOMPILE:=libtool --mode=compile ${CXX} ${CXXFLAGS}
LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS}
INSTALL:=libtool --mode=install install

LIBOBJS:=connection.o consumer.o notification.o prod_proto.o \
	producer.o uiwrap.o

PLUSOBJS:=Event.o Job.o JobStatus.o Notification.o ServerConnection.o
PUB_HDRS:=CountRef.h Event.h JobJobStatus.h Notification.h ServerConnection.h \
	LoggingExceptions.h

LIBTHROBJS:=${LIBOBJS:.o=.thr.o}
LIBLOBJS:=${LIBOBJS:.o=.lo}
LIBTHRLOBJS:=${LIBOBJS:.o=.thr.lo}

PLUSTHROBJS:=${PLUSOBJS:.o=.thr.o}
PLUSLOBJS:=${PLUSOBJS:.o=.lo}
PLUSTHRLOBJS:=${PLUSOBJS:.o=.thr.lo}

LIB:=libglite_lb_client_${nothrflavour}.la
THRLIB:=libglite_lb_client_${thrflavour}.la

PLUSLIB:=libglite_lb_clientpp_${nothrflavour}.la
THRPLUSLIB:=libglite_lb_clientpp_${thrflavour}.la

TOOLS:=dump load purge

${LIB}: ${LIBOBJS}
	${LINK} -o $@ ${LIBLOBJS} -rpath ${glite_location}/lib -lglite_lb_common_${nothrflavour}

${THRLIB}: ${LIBTHROBJS}
	${LINK} -o $@ ${LIBTHRLOBJS} -rpath ${glite_location}/lib -lglite_lb_common_${thrflavour}

${PLUSLIB}: ${PLUSOBJS}
	${LINK} -o $@ ${PLUSLOBJS} -rpath ${glite_location}/lib ${LIB}

${THRPLUSLIB}: ${PLUSTHROBJS}
	${LINK} -o $@ ${PLUSTHRLOBJS} -rpath ${glite_location}/lib ${THRLIB}

logevent: logevent.o args.o
	${LINK} -o $@ logevent.o args.o ${LIB} ${EXT_LIB} ${GLOBUS_LIBS}

${TOOLS}: %: %.o
	${LINK} -o $@ $< ${LIB} ${EXT_LIB} ${GLOBUS_LIBS}

${TOOLS}: ${LIB}

${PLUSOBJS}: %.o: %.cpp
	${CXXCOMPILE} ${GLOBUSINC} -c $<

${PLUSTHROBJS}: %.thr.o: %.cpp
	${CXXCOMPILE} ${GLOBUSTHRINC} -o $@ -c $<

${LIBOBJS}: %.o: %.c
	${COMPILE} ${GLOBUSINC} -c $<

${LIBTHROBJS}: %.thr.o: %.c
	${COMPILE} ${GLOBUSTHRINC} -o $@ -c $<

# catches $TOOLS and logevent compilation
%.o: %.c 
	${CC} ${CFLAGS} ${GLOBUSINC} -c $<

%.c: %.c.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

%.cpp: %.cpp.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

default: all

compile all: ${LIB} ${THRLIB} ${TOOLS} logevent ${PLUSLIB} ${THRPLUSLIB}

check: compile check.producer

check.producer: producer_test
	./producer_test

producer_test: producer_test.o prod_proto_test.o
	${LINKXX} -o $@ ${LIB} ${TEST_LIBS} $+ ${EXT_LIB} ${GLOBUS_LIBS}

producer_test.o: %.o: %.cpp
	${CXX} -c ${CXXFLAGS} ${TEST_INC} $<
	
stage: compile
	$(MAKE) install PREFIX=${stagedir}
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
	mkdir -p ${PREFIX}/bin
	mkdir -p ${PREFIX}/lib
	mkdir -p ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${LIB} ${THRLIB} ${PLUSLIB} ${THRPLUSLIB} ${PREFIX}/lib
	${INSTALL} -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	for p in ${TOOLS} logevent; do \
		${INSTALL} -m 755 "$$p" "${PREFIX}/bin/glite-lb-$$p"; \
	done

clean:

