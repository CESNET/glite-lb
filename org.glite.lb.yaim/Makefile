top_srcdir=..
stagedir=.
package=glite-lb-yaim
version=0.0.0
PREFIX=/opt/glite
prefix=

-include Makefile.inc
-include ${top_srcdir}/project/version.properties

version=${module.version}

yaim_prefix=/opt/glite
ifeq (${project},emi)
yaim_project=/emi
endif

SOURCES=\
	config/defaults/glite-lb.pre \
	config/functions/* config/functions/emi/* \
	config/node-info.d/glite-lb config/node-info.d/emi/glite-lb \
	Makefile
FILES=glite-lb.pre config_glite_lb LICENSE COPYRIGHT node-version arch service update

all ${FILES}:
	glite_var="${localstatedir}/glite"; \
	if echo "${localstatedir}" | grep 'glite'>/dev/null; then \
		glite_var="${localstatedir}"; \
	fi; \
	sed -e 's:@glite_prefix@:${sysroot}${prefix}:' -e 's:@glite_etc@:${sysconfdir}:' -e "s:@glite_var@:$$glite_var:" $(top_srcdir)/config/defaults/glite-lb.pre > glite-lb.pre; \
	sed -e 's:@glite_prefix@:${sysroot}${prefix}:' -e 's:@glite_etc@:${sysconfdir}:' -e "s:@glite_var@:$$glite_var:" $(top_srcdir)/config/functions/config_glite_lb.in > config_glite_lb

	# metapackage info
	echo -e "${gLiteCopyrightText}\n\n${gLiteLicenseText}" > LICENSE
	cp LICENSE COPYRIGHT
	echo ${version}-${module.age} > node-version
	echo ${platformArch} > arch
	echo "NA" > service
	echo "NA" > update

install: ${FILES}
	mkdir -p ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/functions/
	mkdir -p ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/node-info.d
	mkdir -p ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/defaults
	mkdir -p ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/etc/versions
	install -m 0644 $(top_srcdir)/config/functions/config_gip_lb ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/functions
	install -m 0644 $(top_srcdir)/config/functions${yaim_project}/config_info_service_lb ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/functions
	install -m 0644 config_glite_lb ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/functions
	install -m 0644 ${top_srcdir}/config/node-info.d${yaim_project}/glite-lb ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/node-info.d
	install -m 0644 glite-lb.pre ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/defaults
	echo "$(package) $(version)-$(module.age)" > ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/etc/versions/$(package)
	mkdir -p ${DESTDIR}${PREFIX}${yaim_prefix}/release/glite-LB
	cp -fp LICENSE COPYRIGHT node-version arch service update ${DESTDIR}${PREFIX}${yaim_prefix}/release/glite-LB/

stage:
	$(MAKE) install PREFIX=${stagedir}

check:

clean:
	rm -rvf dist ${package}-*.tar.gz

distclean:
	rm -rvf Makefile.inc *.spec debian/

.PHONY: default all check install stage clean distclean dist distcheck
