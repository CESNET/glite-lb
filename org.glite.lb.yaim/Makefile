top_srcdir=..
stagedir=.
package=glite-lb-yaim
version=0.0.0
PREFIX=/opt/glite
prefix=

-include Makefile.inc
-include ../project/version.properties

all:

install:
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/yaim/functions/
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/yaim/node-info.d
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/yaim/defaults
	mkdir -p ${DESTDIR}${PREFIX}${prefix}/yaim/etc/versions
	install -m 0644 $(top_srcdir)/config/defaults/glite-lb.pre ${DESTDIR}${PREFIX}${prefix}/yaim/defaults
	for c in gip_lb glite_lb info_service_lb ; do \
		install -m 0644 $(top_srcdir)/config/functions/config_$$c ${DESTDIR}${PREFIX}${prefix}/yaim/functions; \
	done
	# dirty hack for EPEL - flavour-less globus not initialized by gpt
	if test "x$(thrflavour)" = "x"; then \
		grep -v config_globus_clients $(top_srcdir)/config/node-info.d/glite-lb > glite-lb; \
		install -m 0644 glite-lb ${DESTDIR}${PREFIX}${prefix}/yaim/node-info.d; \
	else \
		install -m 0644 $(top_srcdir)/config/node-info.d/glite-lb ${DESTDIR}${PREFIX}${prefix}/yaim/node-info.d; \
	fi
	echo "$(package) $(module.version)-$(module.age)" > ${DESTDIR}${PREFIX}${prefix}/yaim/etc/versions/$(package)

stage:
	$(MAKE) install PREFIX=${stagedir}

check:

clean:
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/ debian/

.PHONY: default all check stage clean
