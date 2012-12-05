Name:           glite-lb-server
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Daemons
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.server/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  expat
# gssapi is needed explicitly for glite-lb-server, but the proper package is
# known only in glite-lbjp-common-gss-devel:
#  - gssapi from Globus (globus-gssapi-gsi-devel)
#  - gssapi from MIT Kerberos (krb5-devel)
#  - gssapi from Heimdal Kerberos
#BuildRequires: globus-gssapi-gsi-devel
BuildRequires:  glite-jobid-api-c-devel
BuildRequires:  glite-lb-common-devel
BuildRequires:  glite-lb-state-machine-devel
BuildRequires:  glite-lb-types
BuildRequires:  glite-lb-ws-interface
BuildRequires:  glite-lbjp-common-db-devel
BuildRequires:  glite-lbjp-common-gss-devel
BuildRequires:  glite-lbjp-common-gsoap-plugin-devel
BuildRequires:  glite-lbjp-common-log-devel
BuildRequires:  glite-lbjp-common-maildir-devel
BuildRequires:  glite-lbjp-common-server-bones-devel
BuildRequires:  glite-lbjp-common-trio-devel
BuildRequires:  gridsite-devel
BuildRequires:  gsoap-devel
BuildRequires:  c-ares-devel
BuildRequires:  cppunit-devel
BuildRequires:  flex
BuildRequires:  voms-devel
BuildRequires:  classads-devel
BuildRequires:  libtool
BuildRequires:  libxml2-devel
BuildRequires:  expat-devel
BuildRequires:  bison
BuildRequires:  chrpath
BuildRequires:  pkgconfig
Requires:       mysql-server
# for upgrade from EMI-1:
# new function glite_srvbones_daemon_listen() in 2.2.0
Requires:       glite-lbjp-common-server-bones%{?_isa} >= 2.2.0
Requires:       glite-lb-client-progs
Requires:       glite-lb-utils
Requires(post): chkconfig
Requires(preun): chkconfig
Requires(preun): initscripts

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
install -m 0644 LICENSE project/ChangeLog $RPM_BUILD_ROOT/usr/share/doc/%{name}-%{version}
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
if [ $1 -eq 1 ] ; then
	/sbin/chkconfig glite-lb-bkserverd off
fi


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
%dir /etc/cron.d/
%dir /etc/glite-lb/
%dir /usr/share/doc/%{name}-%{version}
%dir /usr/share/glite/
/usr/share/doc/%{name}-%{version}/ChangeLog
/usr/share/doc/%{name}-%{version}/LICENSE
/usr/share/doc/%{name}-%{version}/glite-lb
%config(noreplace) /etc/logrotate.d/glite-lb-server
%config(noreplace) /etc/mysql/conf.d/glite-lb-server.cnf
%config(noreplace) /etc/glite-lb/*
%config(noreplace missingok) /etc/sysconfig/glite-lb
/etc/cron.d/*
/etc/init.d/glite-lb-bkserverd
%{_bindir}/*
/usr/sbin/*
/usr/share/glite/*
/usr/share/man/man1/glite-lb-mon-db.1.gz
/usr/share/man/man8/glite-lb-bkindex.8.gz
/usr/share/man/man8/glite-lb-bkserverd.8.gz
/usr/share/man/man8/glite-lb-setup.8.gz


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
