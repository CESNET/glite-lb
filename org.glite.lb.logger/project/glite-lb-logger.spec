Name:           glite-lb-logger
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Daemons
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.logger/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  cppunit-devel%{?_isa}
BuildRequires:  chrpath
BuildRequires:  glite-lb-common-devel%{?_isa}
BuildRequires:  glite-jobid-api-c-devel%{?_isa}
BuildRequires:  glite-lbjp-common-gss-devel%{?_isa}
BuildRequires:  glite-lbjp-common-trio-devel%{?_isa}
BuildRequires:  glite-lbjp-common-log-devel%{?_isa}
BuildRequires:  libtool
BuildRequires:  pkgconfig
%if 0%{?fedora}
Requires(post): chkconfig
Requires(preun): chkconfig
Requires(preun): initscripts
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
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi --module lb.logger
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make


%check
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
%if 0%{?fedora}
# preserve directory in /var/run
mkdir -p ${RPM_BUILD_ROOT}%{_prefix}/lib/tmpfiles.d
cat > ${RPM_BUILD_ROOT}%{_prefix}/lib/tmpfiles.d/glite-lb-logger.conf <<EOF
d %{_localstatedir}/run/glite 0755 glite glite -
EOF
%else
sed -i 's,\(lockfile=/var/lock\),\1/subsys,' $RPM_BUILD_ROOT/etc/init.d/glite-lb-locallogger
mkdir $RPM_BUILD_ROOT/etc/rc.d
mv $RPM_BUILD_ROOT/etc/init.d $RPM_BUILD_ROOT/etc/rc.d
%endif
find $RPM_BUILD_ROOT -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'
mkdir -p $RPM_BUILD_ROOT/var/lib/glite
mkdir -p $RPM_BUILD_ROOT/var/run/glite
mkdir -p $RPM_BUILD_ROOT/var/spool/glite/lb-locallogger
mkdir -p $RPM_BUILD_ROOT/var/spool/glite/lb-notif
mkdir -p $RPM_BUILD_ROOT/var/spool/glite/lb-proxy
touch $RPM_BUILD_ROOT/var/run/glite/glite-lb-interlogger.sock
touch $RPM_BUILD_ROOT/var/run/glite/glite-lb-notif.sock
touch $RPM_BUILD_ROOT/var/run/glite/glite-lb-proxy.sock
touch $RPM_BUILD_ROOT/var/run/glite/glite-lb-interlogd.pid
touch $RPM_BUILD_ROOT/var/run/glite/glite-lb-logd.pid
touch $RPM_BUILD_ROOT/var/run/glite/glite-lb-notif-interlogd.pid
touch $RPM_BUILD_ROOT/var/run/glite/glite-lb-proxy-interlogd.pid


%clean
rm -rf $RPM_BUILD_ROOT


%pre
getent group glite >/dev/null || groupadd -r glite
getent passwd glite >/dev/null || useradd -r -g glite -d /var/lib/glite -c "gLite user" glite
exit 0


%post
%if 0%{?fedora}
# Fedora 18: systemd_post glite-lb-....service
if [ $1 -eq 1 ] ; then
    # Initial installation
    /bin/systemctl daemon-reload >/dev/null 2>&1 || :
fi
%else
/sbin/chkconfig --add glite-lb-locallogger
if [ $1 -eq 1 ] ; then
	/sbin/chkconfig glite-lb-locallogger off
fi
%endif


%preun
%if 0%{?fedora}
# Fedora 18: systemd_preun glite-lb-logd.service
# Fedora 18: systemd_preun glite-lb-interlogd.service
# Fedora 18: systemd_preun glite-lb-notif-interlogd.service
# Fedora 18: systemd_preun glite-lb-proxy-interlogd.service
if [ $1 -eq 0 ] ; then
    # Package removal, not upgrade
    /bin/systemctl --no-reload disable glite-lb-logd.service > /dev/null 2>&1 || :
    /bin/systemctl --no-reload disable glite-lb-interlogd.service > /dev/null 2>&1 || :
    /bin/systemctl --no-reload disable glite-lb-notif-interlogd.service > /dev/null 2>&1 || :
    /bin/systemctl --no-reload disable glite-lb-proxy-interlogd.service > /dev/null 2>&1 || :
    /bin/systemctl stop glite-lb-logd.service > /dev/null 2>&1 || :
    /bin/systemctl stop glite-lb-interlogd.service > /dev/null 2>&1 || :
    /bin/systemctl stop glite-lb-notif-interlogd.service > /dev/null 2>&1 || :
    /bin/systemctl stop glite-lb-proxy-interlogd.service > /dev/null 2>&1 || :
fi
%else
if [ $1 -eq 0 ] ; then
    /sbin/service glite-lb-locallogger stop >/dev/null 2>&1
    /sbin/chkconfig --del glite-lb-locallogger
fi
%endif


%postun
%if 0%{?fedora}
# Fedora 18: systemd_postun_with_restart glite-lb-logd.service
# Fedora 18: systemd_postun_with_restart glite-lb-interlogd.service
# Fedora 18: systemd_postun_with_restart glite-lb-notif-interlogd.service
# Fedora 18: systemd_postun_with_restart glite-lb-proxy-interlogd.service
/bin/systemctl daemon-reload >/dev/null 2>&1 || :
if [ $1 -ge 1 ] ; then
    # Package upgrade, not uninstall
    /bin/systemctl try-restart glite-lb-logd.service >/dev/null 2>&1 || :
    /bin/systemctl try-restart glite-lb-interlogd.service >/dev/null 2>&1 || :
    /bin/systemctl try-restart glite-lb-notif-interlogd.service >/dev/null 2>&1 || :
    /bin/systemctl try-restart glite-lb-proxy-interlogd.service >/dev/null 2>&1 || :
fi
%else
if [ "$1" -ge "1" ] ; then
    /sbin/service glite-lb-locallogger condrestart >/dev/null 2>&1 || :
fi
%endif


%files
%defattr(-,root,root)
%dir %attr(0755, glite, glite) %{_localstatedir}/lib/glite
%dir %attr(0755, glite, glite) %{_localstatedir}/run/glite
%dir %attr(0755, glite, glite) %{_localstatedir}/spool/glite
%dir %attr(0755, glite, glite) %{_localstatedir}/spool/glite/lb-locallogger
%dir %attr(0755, glite, glite) %{_localstatedir}/spool/glite/lb-notif
%dir %attr(0755, glite, glite) %{_localstatedir}/spool/glite/lb-proxy
%doc LICENSE project/ChangeLog
%ghost %{_localstatedir}/run/glite/glite-lb-interlogger.sock
%ghost %{_localstatedir}/run/glite/glite-lb-notif.sock
%ghost %{_localstatedir}/run/glite/glite-lb-proxy.sock
%ghost %{_localstatedir}/run/glite/glite-lb-interlogd.pid
%ghost %{_localstatedir}/run/glite/glite-lb-logd.pid
%ghost %{_localstatedir}/run/glite/glite-lb-notif-interlogd.pid
%ghost %{_localstatedir}/run/glite/glite-lb-proxy-interlogd.pid
%if 0%{?fedora}
%{_prefix}/lib/tmpfiles.d/glite-lb-logger.conf
%{_unitdir}//glite-lb-logd.service
%{_unitdir}//glite-lb-interlogd.service
%{_unitdir}//glite-lb-notif-interlogd.service
%{_unitdir}//glite-lb-proxy-interlogd.service
%else
%{_initrddir}/glite-lb-locallogger
%endif
%{_bindir}/glite-lb-interlogd
%{_bindir}/glite-lb-logd
%{_bindir}/glite-lb-notif-interlogd
%{_bindir}/glite-lb-proxy-interlogd
%{_mandir}/man8/glite-lb-interlogd.8.gz
%{_mandir}/man8/glite-lb-logd.8.gz

%files devel
%defattr(-,root,root)
%dir %{_includedir}/glite/
%dir %{_includedir}/glite/lb/
%{_includedir}/glite/lb/*.h


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
