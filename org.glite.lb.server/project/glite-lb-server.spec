%global _hardened_build 1

%{!?_pkgdocdir: %global _pkgdocdir %{_docdir}/%{name}-%{version}}

%if 0%{?rhel} >= 7 || 0%{?fedora}
%global mysqlconfdir %{_sysconfdir}/my.cnf.d
%else
%global mysqlconfdir %{_sysconfdir}/mysql/conf.d
%endif

# condor classads requires 2011 ISO C++ standard since Fedora 19
%if 0%{?rhel} >= 7 || 0%{?fedora} >= 19
%global classad_cxxflags -std=c++11
%endif

Name:           glite-lb-server
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Daemons
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.server/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

# gssapi.h is needed explicitly for glite-lb-server, but the proper package is
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
%if 0%{?rhel} <= 6 && ! 0%{?fedora}
BuildRequires:  classads-devel
%else
BuildRequires:  condor-classads-devel
%endif
BuildRequires:  libtool
BuildRequires:  libxml2-devel
BuildRequires:  expat-devel
BuildRequires:  bison
BuildRequires:  chrpath
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
BuildRequires:  pkgconfig
Requires:       crontabs
Requires:       mysql-server%{?_isa}
# for upgrade from EMI-1:
# new function glite_srvbones_daemon_listen() in 2.2.0
Requires:       glite-lbjp-common-server-bones%{?_isa} >= 2.2.0
Requires:       glite-lb-client-progs%{?_isa}
Requires:       glite-lb-utils%{?_isa}
%if 0%{?rhel} >= 7 || 0%{?fedora}
Requires(post): systemd
Requires(preun): systemd
Requires(postun): systemd
BuildRequires:  systemd
%else
Requires(post): chkconfig
Requires(preun): chkconfig
Requires(preun): initscripts
%endif

%description
@DESCRIPTION@


%prep
%setup -q


%build
perl ./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --docdir=%{_pkgdocdir} --mysqlconfdir=%{mysqlconfdir} --sysdefaultdir=%{_sysconfdir}/sysconfig --project=emi
CFLAGS="%{?optflags}" CXXFLAGS="%{?optflags} %{?classad_cxxflags}" LDFLAGS="%{?__global_ldflags}" make %{?_smp_mflags}


%check
CFLAGS="%{?optflags}" CXXFLAGS="%{?optflags} %{?classad_cxxflags}" LDFLAGS="%{?__global_ldflags}" make check


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
%if 0%{?rhel} >= 7 || 0%{?fedora}
# preserve directory in /var/run
mkdir -p %{buildroot}%{_tmpfilesdir}
cat > %{buildroot}%{_tmpfilesdir}/glite-lb-server.conf <<EOF
d %{_localstatedir}/run/glite 0755 glite glite -
EOF
%endif
install -m 0644 ChangeLog LICENSE %{buildroot}%{_pkgdocdir}
rm -f %{buildroot}%{_libdir}/*.a
find %{buildroot} -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'
mkdir -p %{buildroot}%{_localstatedir}/lib/glite/dump
mkdir -p %{buildroot}%{_localstatedir}/lib/glite/purge
mkdir -p %{buildroot}%{_localstatedir}/log/glite
mkdir -p %{buildroot}%{_localstatedir}/run/glite


%clean
rm -rf %{buildroot}


%pre
getent group glite >/dev/null || groupadd -r glite
getent passwd glite >/dev/null || useradd -r -g glite -d /var/lib/glite -c "gLite user" glite
exit 0


%post
%if 0%{?rhel} >= 7 || 0%{?fedora}
%systemd_post glite-lb-bkserverd.service
%else
/sbin/chkconfig --add glite-lb-bkserverd
if [ $1 -eq 1 ] ; then
    /sbin/chkconfig glite-lb-bkserverd off
fi

# upgrade from lb.server <= 3.0.1 (L&B <= 4.0.1)
if [ ! -d %{_localstatedir}/run/glite ]; then
  mkdir -p %{_localstatedir}/run/glite
  chown -R glite:glite %{_localstatedir}/run/glite
fi
[ -f /var/glite/glite-lb-bkserverd.pid -a ! -f %{_localstatedir}/run/glite/glite-lb-bkserverd.pid ] && cp -pv /var/glite/glite-lb-bkserverd.pid %{_localstatedir}/run/glite/ || :
# upgrade from L&B server <= 3.0.2 (L&B <= 4.0.1)
[ -f /var/glite/lb_server_stats -a ! -f %{_localstatedir}/lib/glite/lb_server_stats ] && mv -v /var/glite/lb_server_stats %{_localstatedir}/lib/glite/ || :
%endif


%preun
%if 0%{?rhel} >= 7 || 0%{?fedora}
%systemd_preun glite-lb-bkserverd.service
%else
if [ $1 -eq 0 ] ; then
    /sbin/service glite-lb-bkserverd stop >/dev/null 2>&1
    /sbin/chkconfig --del glite-lb-bkserverd
fi
%endif


%postun
%if 0%{?rhel} >= 7 || 0%{?fedora}
%systemd_postun_with_restart glite-lb-bkserverd.service
%else
if [ "$1" -ge "1" ] ; then
    /sbin/service glite-lb-bkserverd condrestart >/dev/null 2>&1 || :
fi
%endif


%files
%defattr(-,root,root)
%dir %attr(0755, glite, glite) %{_localstatedir}/lib/glite
%dir %attr(0755, glite, glite) %{_localstatedir}/lib/glite/dump
%dir %attr(0755, glite, glite) %{_localstatedir}/lib/glite/purge
%dir %attr(0755, glite, glite) %{_localstatedir}/log/glite
%dir %attr(0755, glite, glite) %{_localstatedir}/run/glite
%dir %attr(0755, glite, glite) %{_localstatedir}/spool/glite
%dir %attr(0775, glite, glite) %{_localstatedir}/spool/glite/lb-locallogger
%dir %attr(0755, glite, glite) %{_localstatedir}/spool/glite/lb-notif
%dir %attr(0755, glite, glite) %{_localstatedir}/spool/glite/lb-proxy
%dir %{_datadir}/glite/
%dir %{_pkgdocdir}/
%dir %{_sysconfdir}/glite-lb/
%config(noreplace) %{_sysconfdir}/cron.d/%{name}-*
%config(noreplace) %{_sysconfdir}/glite-lb/*
%config(noreplace) %{_sysconfdir}/logrotate.d/glite-lb-server
%config(noreplace) %{mysqlconfdir}/glite-lb-server.cnf
%config(noreplace missingok) %{_sysconfdir}/sysconfig/glite-lb
%{_pkgdocdir}/ChangeLog
%{_pkgdocdir}/LICENSE
%{_pkgdocdir}/glite-lb
%if 0%{?rhel} >= 7 || 0%{?fedora}
%{_tmpfilesdir}/glite-lb-server.conf
%{_unitdir}/glite-lb-bkserverd.service
%else
%{_initrddir}/glite-lb-bkserverd
%endif
%{_bindir}/*
%{_sbindir}/*
%{_datadir}/glite/*
%{_mandir}/man1/*.1*
%{_mandir}/man8/*.8*


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
