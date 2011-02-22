top_srcdir=..
stagedir=.
package=glite-lb-yaim
version=0.0.0
PREFIX=/opt/glite

-include Makefile.inc
-include ../project/version.properties

all:

install:
	mkdir -p $(PREFIX)/yaim/functions/
	mkdir -p $(PREFIX)/yaim/node-info.d
	mkdir -p $(PREFIX)/yaim/defaults
	mkdir -p $(PREFIX)/yaim/etc/versions
	install -m 0644 $(top_srcdir)/config/defaults/glite-lb.pre $(PREFIX)/yaim/defaults
	for c in gip_lb glite_lb info_service_lb ; do \
		install -m 0644 $(top_srcdir)/config/functions/config_$$c $(PREFIX)/yaim/functions; \
	done
	# dirthy hack for EPEL - flavour-less globus not initialized by gpt
	if test "x$(thrflavour)" = "x"; then \
		grep -v config_globus_clients $(top_srcdir)/config/node-info.d/glite-lb > glite-lb; \
		install -m 0644 glite-lb $(PREFIX)/yaim/node-info.d; \
	else \
		install -m 0644 $(top_srcdir)/config/node-info.d/glite-lb $(PREFIX)/yaim/node-info.d; \
	fi
	echo "$(package) $(module.version)-$(module.age)" > $(PREFIX)/yaim/etc/versions/$(package)

stage:
	$(MAKE) install PREFIX=${stagedir}

check:

clean:
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/ debian/

.PHONY: default all check stage clean
