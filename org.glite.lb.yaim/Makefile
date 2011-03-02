top_srcdir=..
stagedir=.
package=glite-lb-yaim
version=0.0.0
PREFIX=/opt/glite
prefix=

-include Makefile.inc
-include ../project/version.properties

yaim_prefix=/opt/glite
node_info=${top_srcdir}/config/node-info.d/glite-lb
node_info_project=${top_srcdir}/config/node-info.d/${project}/glite-lb

FILES=glite-lb.pre glite-lb

all ${FILES}:
	if test -f "${node_info_project}"; then \
		cp ${node_info_project} . ; \
	else \
		cp ${node_info} . ; \
	fi
	glite_var="${localstatedir}/glite"; \
	if echo "${localstatedir}" | grep 'glite'>/dev/null; then \
		glite_var="${localstatedir}"; \
	fi; \
	sed -e 's:@glite_prefix@:${sysroot}${prefix}:' -e 's:@glite_etc@:${sysconfdir}:' -e "s:@glite_var@:$$glite_var:" $(top_srcdir)/config/defaults/glite-lb.pre > glite-lb.pre

install: ${FILES}
	mkdir -p ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/functions/
	mkdir -p ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/node-info.d
	mkdir -p ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/defaults
	mkdir -p ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/etc/versions
	for c in gip_lb glite_lb info_service_lb ; do \
		install -m 0644 $(top_srcdir)/config/functions/config_$$c ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/functions; \
	done
	install -m 0644 glite-lb ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/node-info.d
	install -m 0644 glite-lb.pre ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/defaults
	echo "$(package) $(module.version)-$(module.age)" > ${DESTDIR}${PREFIX}${yaim_prefix}/yaim/etc/versions/$(package)

stage:
	$(MAKE) install PREFIX=${stagedir}

check:

clean:
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/ debian/ ${FILES}

.PHONY: default all check stage clean
