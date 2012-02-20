Summary: @SUMMARY@
Name: glite-lb-server
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: ASL 2.0
Vendor: EMI
Group: System Environment/Daemons
BuildRequires: expat
BuildRequires: globus-gssapi-gsi-devel
BuildRequires: glite-jobid-api-c-devel
BuildRequires: glite-lb-common-devel
BuildRequires: glite-lb-state-machine-devel
BuildRequires: glite-lb-types
BuildRequires: glite-lb-ws-interface
BuildRequires: glite-lbjp-common-db-devel
BuildRequires: glite-lbjp-common-gss-devel
BuildRequires: glite-lbjp-common-gsoap-plugin-devel
BuildRequires: glite-lbjp-common-log-devel
BuildRequires: glite-lbjp-common-maildir-devel
BuildRequires: glite-lbjp-common-server-bones-devel
BuildRequires: glite-lbjp-common-trio-devel
BuildRequires: gridsite-devel
BuildRequires: gsoap-devel
BuildRequires: libxml2
BuildRequires: mysql-devel
BuildRequires: c-ares-devel
BuildRequires: cppunit-devel
BuildRequires: gridsite-shared
BuildRequires: flex
BuildRequires: voms-devel
BuildRequires: classads-devel
BuildRequires: libtool
BuildRequires: lcas-devel
BuildRequires: c-ares
BuildRequires: classads
BuildRequires: libxml2-devel
BuildRequires: expat-devel
BuildRequires: voms
BuildRequires: bison
BuildRequires: chrpath
Requires: mysql-server
# for upgrade from EMI-1:
# new function glite_srvbones_daemon_listen() in 2.2.0
Requires: glite-lbjp-common-server-bones%{?_isa} >= 2.2.0
Requires(post): chkconfig
Requires(preun): chkconfig
Requires(preun): initscripts
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.server/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.server
make


%check
make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
sed -i 's,\(lockfile=/var/lock\),\1/subsys,' $RPM_BUILD_ROOT/etc/init.d/glite-lb-bkserverd
find $RPM_BUILD_ROOT -name '*.la' -exec rm -rf {} \;
find $RPM_BUILD_ROOT -name '*.a' -exec rm -rf {} \;
find $RPM_BUILD_ROOT -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'


%clean
rm -rf $RPM_BUILD_ROOT


%pre
getent group glite >/dev/null || groupadd -r glite
getent passwd glite >/dev/null || useradd -r -g glite -d /var/glite -c "gLite user" glite
mkdir -p /var/glite /var/log/glite 2>/dev/null || :
chown glite:glite /var/glite /var/log/glite
exit 0


%post
/sbin/chkconfig --add glite-lb-bkserverd


%preun
if [ $1 -eq 0 ] ; then
    /sbin/service glite-lb-bkserverd stop >/dev/null 2>&1
    /sbin/chkconfig --del glite-lb-bkserverd
fi


%postun
if [ "$1" -ge "1" ] ; then
    /sbin/service glite-lb-bkserverd condrestart >/dev/null 2>&1 || :
fi


%files
%defattr(-,root,root)
/etc/glite-lb-dbsetup.sql
%dir /etc/cron.d/
%dir /etc/glite-lb/
%dir /usr/%{_lib}/modules/
%dir /usr/include/glite/
%dir /usr/include/glite/lb/
%dir /usr/share/doc/%{name}-%{version}/
%config(noreplace) /etc/logrotate.d/glite-lb-purge
%config(noreplace) /etc/logrotate.d/glite-lb-lcas
%config(noreplace) /etc/mysql/conf.d/glite-lb-server.cnf
/etc/cron.d/*
/etc/glite-lb-index.conf.template
/etc/glite-lb/*
/etc/init.d/glite-lb-bkserverd
/usr/%{_lib}/modules/lcas_lb.mod
/usr/%{_lib}/modules/liblcas_lb.so
/usr/%{_lib}/modules/liblcas_lb.so.0
/usr/%{_lib}/modules/liblcas_lb.so.0.0.0
/usr/include/glite/lb/index.h
/usr/include/glite/lb/lb_authz.h
/usr/include/glite/lb/store.h
/usr/include/glite/lb/srv_perf.h
/usr/bin/glite-lb-bkindex
/usr/bin/glite-lb-mon-db
/usr/bin/glite-lb-bkserverd
/usr/sbin/glite-lb-notif-keeper
/usr/share/doc/%{name}-%{version}/ChangeLog
/usr/share/doc/%{name}-%{version}/LICENSE
/usr/share/doc/%{name}-%{version}/package.description
/usr/share/doc/%{name}-%{version}/package.summary
/usr/share/man/man1/glite-lb-bkindex.8.gz
/usr/share/man/man1/glite-lb-bkserverd.8.gz
/usr/share/man/man1/glite-lb-mon-db.1.gz
/usr/share/man/man8/glite-lb-bkindex.8.gz
/usr/share/man/man8/glite-lb-bkserverd.8.gz
/usr/share/man/man8/glite-lb-mon-db.1.gz


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
