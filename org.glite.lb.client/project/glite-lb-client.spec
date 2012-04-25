Summary: @SUMMARY@
Name: glite-lb-client
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: ASL 2.0
Vendor: EMI
Group: System Environment/Libraries
BuildRequires: classads
BuildRequires: classads-devel
BuildRequires: cppunit-devel
BuildRequires: chrpath
BuildRequires: glite-lb-types
BuildRequires: glite-jobid-api-c-devel
BuildRequires: glite-jobid-api-cpp-devel
BuildRequires: glite-lb-common-devel
BuildRequires: glite-lbjp-common-gss-devel
BuildRequires: glite-lbjp-common-trio-devel
BuildRequires: libtool
BuildRequires: pkgconfig
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.client/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%package devel
Summary: Development files for gLite L&B client library
Group: Development/Libraries
Requires: %{name}%{?_isa} = %{version}-%{release}
Requires: glite-lb-common-devel
Requires: glite-jobid-api-c-devel
Requires: glite-jobid-api-cpp-devel


%description devel
This package contains development libraries and header files for gLite L&B
client library.


%package progs
Summary: gLite L&B client programs and examples
Group: System Environment/Base


%description progs
This package contains client programs and examples for gLite L&B.


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.client
make


%check
make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*.la' -exec rm -rf {} \;
find $RPM_BUILD_ROOT -name '*.a' -exec rm -rf {} \;
find $RPM_BUILD_ROOT -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'


%clean
rm -rf $RPM_BUILD_ROOT


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%defattr(-,root,root)
%dir /usr/share/doc/%{name}-%{version}/
/usr/%{_lib}/libglite_lb_client.so.11.@MINOR@.@REVISION@
/usr/%{_lib}/libglite_lb_client.so.11
/usr/%{_lib}/libglite_lb_clientpp.so.11.@MINOR@.@REVISION@
/usr/%{_lib}/libglite_lb_clientpp.so.11
/usr/share/doc/%{name}-%{version}/ChangeLog
/usr/share/doc/%{name}-%{version}/LICENSE
/usr/share/doc/%{name}-%{version}/README-notify
/usr/share/doc/%{name}-%{version}/package.description
/usr/share/doc/%{name}-%{version}/package.summary


%files devel
%defattr(-,root,root)
%dir /usr/share/emi/
%dir /usr/share/emi/build/
%dir /usr/share/emi/build/m4/
%dir /usr/share/doc/%{name}-%{version}/examples/
%dir /usr/include/glite/
%dir /usr/include/glite/lb/
/usr/include/glite/lb/*.h
/usr/%{_lib}/libglite_lb_client.so
/usr/%{_lib}/libglite_lb_clientpp.so
/usr/share/doc/%{name}-%{version}/examples/*
/usr/share/emi/build/m4/glite_lb.m4
/usr/share/man/man1/glite-lb-notify.1.gz
/usr/share/man/man1/glite-lb-logevent.1.gz
/usr/share/man/man8/glite-lb-dump.8.gz
/usr/share/man/man8/glite-lb-load.8.gz


%files progs
%defattr(-,root,root)
%dir /usr/%{_lib}/glite-lb/
%dir /usr/%{_lib}/glite-lb/examples/
/usr/bin/glite-lb-logevent
/usr/bin/glite-lb-notify
/usr/bin/glite-lb-register_sandbox
/usr/%{_lib}/glite-lb/examples/*
/usr/sbin/glite-lb-export.sh


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
