%{!?_pkgdocdir: %global _pkgdocdir %{_docdir}/%{name}-%{version}}

Name:           glite-lb-client
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Libraries
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.client/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%if 0%{?rhel} <= 6 && ! 0%{?fedora}
BuildRequires:  classads-devel
%else
BuildRequires:  condor-classads-devel
%endif
BuildRequires:  cppunit-devel
BuildRequires:  chrpath
BuildRequires:  glite-lb-types
BuildRequires:  glite-jobid-api-c-devel
BuildRequires:  glite-jobid-api-cpp-devel
BuildRequires:  glite-lb-common-devel
BuildRequires:  glite-lbjp-common-gss-devel
BuildRequires:  glite-lbjp-common-trio-devel
BuildRequires:  libtool
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
BuildRequires:  pkgconfig

%description
@DESCRIPTION@


%package        devel
Summary:        Development files for gLite L&B client library
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       glite-lb-common-devel%{?_isa}
Requires:       glite-jobid-api-c-devel%{?_isa}
Requires:       glite-jobid-api-cpp-devel

%description    devel
This package contains development libraries and header files for gLite L&B
client library.


%package        progs
Summary:        gLite L&B client programs and examples
Group:          System Environment/Base
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description    progs
This package contains client programs and examples for gLite L&B.


%prep
%setup -q


%build
./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --docdir=%{_pkgdocdir} --project=emi
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
# another installed documentation can't be combined with %%doc on EPEL 5/6,
# install these files here instead
install -m 0644 ChangeLog LICENSE %{buildroot}%{_pkgdocdir}
rm -f %{buildroot}%{_libdir}/*.a
rm -f %{buildroot}%{_libdir}/*.la
find %{buildroot} -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'


%clean
rm -rf %{buildroot}


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%defattr(-,root,root)
%dir %{_pkgdocdir}/
%{_libdir}/libglite_lb_client.so.14
%{_libdir}/libglite_lb_client.so.14.*
%{_libdir}/libglite_lb_clientpp.so.14
%{_libdir}/libglite_lb_clientpp.so.14.*
%{_pkgdocdir}/ChangeLog
%{_pkgdocdir}/LICENSE

%files devel
%defattr(-,root,root)
%dir %{_pkgdocdir}/examples/
%dir %{_datadir}/emi/
%dir %{_datadir}/emi/build/
%dir %{_datadir}/emi/build/m4/
%{_includedir}/glite/lb/*.h
%{_libdir}/libglite_lb_client.so
%{_libdir}/libglite_lb_clientpp.so
%{_libdir}/pkgconfig/*.pc
%{_pkgdocdir}/examples/*
%{_datadir}/emi/build/m4/glite_lb.m4

%files progs
%defattr(-,root,root)
%dir %{_libdir}/glite-lb/
%dir %{_libdir}/glite-lb/examples/
%{_bindir}/glite-lb-logevent
%{_bindir}/glite-lb-notify
%{_bindir}/glite-lb-register_sandbox
%{_libdir}/glite-lb/examples/*
%{_pkgdocdir}/README-notify
%{_mandir}/man1/glite-lb-logevent.1*
%{_mandir}/man1/glite-lb-notify.1*
%{_mandir}/man1/glite-lb-register_sandbox.1*


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
