%{!?_pkgdocdir: %global _pkgdocdir %{_docdir}/%{name}-%{version}}

Name:           glite-lb-harvester
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Daemons
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.harvester/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  chrpath
BuildRequires:  docbook-utils
BuildRequires:  glite-jobid-api-c-devel
BuildRequires:  glite-lb-client-devel
BuildRequires:  glite-lb-common-devel
BuildRequires:  glite-lbjp-common-gss-devel
BuildRequires:  glite-lbjp-common-db-devel
BuildRequires:  glite-lbjp-common-log-devel
BuildRequires:  glite-lbjp-common-trio-devel
BuildRequires:  libtool
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
BuildRequires:  pkgconfig
%if 0%{?fedora}
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
perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=%{_prefix} --libdir=%{_lib} --docdir=%{_pkgdocdir} --project=emi --module lb.harvester
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
install -m 0644 LICENSE project/ChangeLog $RPM_BUILD_ROOT%{_pkgdocdir}
%if 0%{?fedora}
# preserve directory in /var/run
mkdir -p ${RPM_BUILD_ROOT}%{_prefix}/lib/tmpfiles.d
cat > ${RPM_BUILD_ROOT}%{_prefix}/lib/tmpfiles.d/glite-lb-harvester.conf <<EOF
d %{_localstatedir}/run/glite 0755 glite glite -
EOF
%endif
find $RPM_BUILD_ROOT -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'
mkdir -p $RPM_BUILD_ROOT/var/lib/glite
mkdir -p $RPM_BUILD_ROOT/var/run/glite


%clean
rm -rf $RPM_BUILD_ROOT


%pre
getent group glite >/dev/null || groupadd -r glite
getent passwd glite >/dev/null || useradd -r -g glite -d /var/lib/glite -c "gLite user" glite
exit 0


%post
%if 0%{?fedora}
# Fedora 18: systemd_post glite-lb-harvester.service
if [ $1 -eq 1 ] ; then
    # Initial installation
    /bin/systemctl daemon-reload >/dev/null 2>&1 || :
fi
%else
/sbin/chkconfig --add glite-lb-harvester
if [ $1 -eq 1 ] ; then
	/sbin/chkconfig glite-lb-harvester off
fi

# upgrade from L&B harvester <= 1.3.4 (L&B <= 4.0.1)
if [ ! -d /var/run/glite ]; then
  mkdir -p /var/run/glite
  chown -R glite:glite /var/run/glite
fi
[ -f /var/glite/glite-lb-harvester.pid -a ! -f /var/run/glite/glite-lb-harvester.pid ] && cp -pv /var/glite/glite-lb-harvester.pid /var/run/glite/ || :
%endif


%preun
%if 0%{?fedora}
# Fedora 18: systemd_preun glite-lb-harvester.service
if [ $1 -eq 0 ] ; then
    # Package removal, not upgrade
    /bin/systemctl --no-reload disable glite-lb-harvester.service > /dev/null 2>&1 || :
    /bin/systemctl stop glite-lb-harvester.service > /dev/null 2>&1 || :
fi
%else
if [ $1 -eq 0 ] ; then
    /sbin/service glite-lb-harvester stop >/dev/null 2>&1
    /sbin/chkconfig --del glite-lb-harvester
fi
%endif


%postun
%if 0%{?fedora}
# Fedora 18: systemd_postun_with_restart glite-lb-harvester.service
/bin/systemctl daemon-reload >/dev/null 2>&1 || :
if [ $1 -ge 1 ] ; then
    # Package upgrade, not uninstall
    /bin/systemctl try-restart glite-lb-harvester.service >/dev/null 2>&1 || :
fi
%else
if [ "$1" -ge "1" ] ; then
    /sbin/service glite-lb-harvester condrestart >/dev/null 2>&1 || :
fi
%endif


%files
%defattr(-,root,root)
%dir %attr(0755, glite, glite) %{_localstatedir}/lib/glite
%dir %attr(0755, glite, glite) %{_localstatedir}/run/glite
%dir %{_pkgdocdir}/
%dir %{_sysconfdir}/glite-lb/
%dir %{_libdir}/glite-lb/
%dir %{_libdir}/glite-lb/examples/
%dir %{_datadir}/glite/
%if 0%{?fedora}
%{_prefix}/lib/tmpfiles.d/glite-lb-harvester.conf
%{_unitdir}/glite-lb-harvester.service
%else
%{_initrddir}/glite-lb-harvester
%endif
%{_bindir}/glite-lb-harvester
%{_libdir}/glite-lb/examples/glite-lb-harvester-test.sh
%{_libdir}/glite-lb/examples/glite-lb-harvester-dbg
%{_pkgdocdir}/ChangeLog
%{_pkgdocdir}/LICENSE
%{_pkgdocdir}/README
%{_datadir}/glite/*
%{_mandir}/man1/glite-lb-harvester.1.gz


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
