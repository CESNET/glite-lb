%{!?_pkgdocdir: %global _pkgdocdir %{_docdir}/%{name}-%{version}}

Name:           glite-lb-logger-msg
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Daemons
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.logger-msg/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

# for EMI: using activemq-cpp-library (for EPEL <= 6)
# for EPEL: always using activemq-cpp-devel (supported since EPEL 6)
%if 0%{?rhel} >= 7 || 0%{?fedora}
BuildRequires:  activemq-cpp-devel
%else
# only in EMI third-party repository
BuildRequires:  activemq-cpp-library
%endif
BuildRequires:  cppunit-devel
BuildRequires:  glite-lb-logger-devel
BuildRequires:  glite-lbjp-common-log-devel
BuildRequires:  glite-lbjp-common-trio-devel
BuildRequires:  libtool
BuildRequires:  perl
%if 0%{?rhel} >= 7 || 0%{?fedora}
BuildRequires:  perl-Pod-Perldoc
%endif
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(Pod::Man)
BuildRequires:  perl(POSIX)
BuildRequires:  pkgconfig
Requires:       crontabs
Requires:       perl-LDAP
Requires:       glite-lb-logger%{?_isa}

%description
@DESCRIPTION@


%prep
%setup -q


%build
perl ./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --docdir=%{_pkgdocdir} --sysdefaultdir=%{_sysconfdir}/sysconfig --project=emi
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
rm -f %{buildroot}%{_libdir}/*.a
rm -f %{buildroot}%{_libdir}/*.la
find %{buildroot} -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root)
%doc ChangeLog LICENSE config/msg.conf.example
%dir %{_sysconfdir}/glite-lb/
%dir %{_libdir}/glite-lb/
%dir %{_libdir}/glite-lb/examples/
%config(noreplace) %{_sysconfdir}/cron.d/%{name}
%{_libdir}/activemq_cpp_plugin.so
%{_libdir}/glite-lb/examples/glite-lb-cmsclient
%{_sbindir}/glite-lb-msg-*
%{_mandir}/man8/*.8*


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
