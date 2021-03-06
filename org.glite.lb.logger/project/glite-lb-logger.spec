%global _hardened_build 1

Name:           glite-lb-logger
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Daemons
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.logger/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  cppunit-devel
BuildRequires:  chrpath
BuildRequires:  glite-jobid-api-c-devel
BuildRequires:  glite-lb-common-devel
BuildRequires:  glite-lbjp-common-gss-devel
BuildRequires:  glite-lbjp-common-trio-devel
BuildRequires:  glite-lbjp-common-log-devel
BuildRequires:  libtool
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
BuildRequires:  pkgconfig
Requires(pre):  shadow-utils
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


%package        devel
Summary:        Development files for gLite L&B logger
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       glite-lb-common-devel%{?_isa}
Requires:       glite-lbjp-common-gss-devel%{?_isa}
Requires:       glite-lbjp-common-log-devel%{?_isa}

%description    devel
This package contains header files for building plugins for gLite L&B logger.


%prep
%setup -q


%build
./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --sysdefaultdir=%{_sysconfdir}/sysconfig --project=emi
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
%if 0%{?rhel} >= 7 || 0%{?fedora}
# preserve directory in /var/run
mkdir -p %{buildroot}%{_tmpfilesdir}
cat > %{buildroot}%{_tmpfilesdir}/glite-lb-logger.conf <<EOF
d %{_localstatedir}/run/glite 0755 glite glite -
EOF
%else
sed -i 's,\(lockfile=/var/lock\),\1/subsys,' %{buildroot}/etc/rc.d/init.d/glite-lb-locallogger
%endif
find %{buildroot} -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'
mkdir -p %{buildroot}%{_localstatedir}/lib/glite
mkdir -p %{buildroot}%{_localstatedir}/run/glite
touch %{buildroot}%{_localstatedir}/run/glite/glite-lb-interlogger.sock
touch %{buildroot}%{_localstatedir}/run/glite/glite-lb-notif.sock
touch %{buildroot}%{_localstatedir}/run/glite/glite-lb-proxy.sock


%clean
rm -rf %{buildroot}


%pre
getent group glite >/dev/null || groupadd -r glite
getent passwd glite >/dev/null || useradd -r -g glite -d %{_localstatedir}/lib/glite -c "gLite user" glite
exit 0


%post
%if 0%{?rhel} >= 7 || 0%{?fedora}
%systemd_post glite-lb-logd.service glite-lb-interlogd.service glite-lb-notif-interlogd.service glite-lb-proxy-interlogd.service
%else
/sbin/chkconfig --add glite-lb-logd
/sbin/chkconfig --add glite-lb-interlogd
/sbin/chkconfig --add glite-lb-notif-interlogd
/sbin/chkconfig --add glite-lb-proxy-interlogd

if [ $1 -eq 1 ] ; then
  /sbin/chkconfig glite-lb-logd off
  /sbin/chkconfig glite-lb-interlogd off
  /sbin/chkconfig glite-lb-notif-interlogd off
  /sbin/chkconfig glite-lb-proxy-interlogd off
fi

# upgrade from lb.logger <= 2.4.10 (L&B <= 4.0.1)
if [ ! -d %{_localstatedir}/run/glite ]; then
  mkdir -p %{_localstatedir}/run/glite
  chown -R glite:glite %{_localstatedir}/run/glite
fi
for i in logd interlogd notif-interlogd proxy-interlogd; do
  [ -f /var/glite/glite-lb-$i.pid -a ! -f %{_localstatedir}/run/glite/glite-lb-$i.pid ] && cp -pv /var/glite/glite-lb-$i.pid %{_localstatedir}/run/glite/ || :
done

# upgrade from lb.logger <= 2.4.15 (L&B <= 4.0.4)
/sbin/chkconfig --del glite-lb-locallogger
# notif and proxy interlogd deamons won't be restarted when upgrading from
# L&B 4.0.4 (startup moved from lb.server to lb.logger)
%endif


%preun
%if 0%{?rhel} >= 7 || 0%{?fedora}
%systemd_preun glite-lb-logd.service glite-lb-interlogd.service glite-lb-notif-interlogd.service glite-lb-proxy-interlogd.service
%else
if [ $1 -eq 0 ] ; then
    /sbin/service glite-lb-logd stop >/dev/null 2>&1
    /sbin/service glite-lb-interlogd stop >/dev/null 2>&1
    /sbin/service glite-lb-notif-interlogd stop >/dev/null 2>&1
    /sbin/service glite-lb-proxy-interlogd stop >/dev/null 2>&1
    /sbin/chkconfig --del glite-lb-logd
    /sbin/chkconfig --del glite-lb-interlogd
    /sbin/chkconfig --del glite-lb-notif-interlogd
    /sbin/chkconfig --del glite-lb-proxy-interlogd
fi
%endif


%postun
%if 0%{?rhel} >= 7 || 0%{?fedora}
%systemd_postun_with_restart glite-lb-logd.service glite-lb-interlogd.service glite-lb-notif-interlogd.service glite-lb-proxy-interlogd.service
%else
if [ "$1" -ge "1" ] ; then
    /sbin/service glite-lb-logd condrestart >/dev/null 2>&1 || :
    /sbin/service glite-lb-interlogd condrestart >/dev/null 2>&1 || :
    /sbin/service glite-lb-notif-interlogd condrestart >/dev/null 2>&1 || :
    /sbin/service glite-lb-proxy-interlogd condrestart >/dev/null 2>&1 || :
fi
%endif


%files
%{!?_licensedir:%global license %doc}
%defattr(-,root,root)
%doc ChangeLog
%license LICENSE
%dir %attr(-, glite, glite) %{_localstatedir}/lib/glite
%dir %attr(-, glite, glite) %{_localstatedir}/run/glite
%dir %attr(-, glite, glite) %{_localstatedir}/spool/glite
# keep additional group permissions for locallogger directory,
# for storing data files locally by external components (like CREAM)
%dir %attr(-, glite, glite) %{_localstatedir}/spool/glite/lb-locallogger
%dir %attr(-, glite, glite) %{_localstatedir}/spool/glite/lb-notif
%dir %attr(-, glite, glite) %{_localstatedir}/spool/glite/lb-proxy
%dir %{_datadir}/glite-lb-logger
%ghost %{_localstatedir}/run/glite/glite-lb-interlogger.sock
%ghost %{_localstatedir}/run/glite/glite-lb-notif.sock
%ghost %{_localstatedir}/run/glite/glite-lb-proxy.sock
%if 0%{?rhel} >= 7 || 0%{?fedora}
%{_tmpfilesdir}/glite-lb-logger.conf
%{_unitdir}/glite-lb-logd.service
%{_unitdir}/glite-lb-interlogd.service
%{_unitdir}/glite-lb-notif-interlogd.service
%{_unitdir}/glite-lb-proxy-interlogd.service
%else
%{_initrddir}/glite-lb-locallogger
%{_initrddir}/glite-lb-logd
%{_initrddir}/glite-lb-interlogd
%{_initrddir}/glite-lb-notif-interlogd
%{_initrddir}/glite-lb-proxy-interlogd
%endif
%{_sbindir}/glite-lb-interlogd
%{_sbindir}/glite-lb-logd
%{_sbindir}/glite-lb-notif-interlogd
%{_sbindir}/glite-lb-proxy-interlogd
%{_datadir}/glite-lb-logger/lb_krb_ticket.sh
%{_mandir}/man8/glite-lb-interlogd.8*
%{_mandir}/man8/glite-lb-logd.8*

%files devel
%defattr(-,root,root)
%{_includedir}/glite/lb/*.h


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
