Summary: Virtual package for development with gLite L&B client library
Name: glite-lb-client
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: Apache Software License
Vendor: EMI
Group: Development/Libraries
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
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.client/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
This is a virtual package providing runtime and development files for gLite
L&B client library.


%package -n lib%{name}
Summary: @SUMMARY@
Group: System Environment/Libraries


%description -n lib%{name}
@DESCRIPTION@


%package -n %{name}-devel
Summary: Development files for gLite L&B client library
Group: Development/Libraries
Requires: lib%{name}%{?_isa} = %{version}-%{release}
Requires: glite-lb-common-devel
Requires: glite-jobid-api-c-devel
Requires: glite-jobid-api-cpp-devel
Provides: %{name}%{?_isa} = %{version}-%{release}


%description -n %{name}-devel
This package contains development libraries and header files for gLite L&B
client library.


%package -n %{name}-progs
Summary: gLite L&B client programs and examples
Group: Applications/Communications


%description -n %{name}-progs
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


%post -n lib%{name} -p /sbin/ldconfig


%postun -n lib%{name} -p /sbin/ldconfig


%files -n lib%{name}
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


%files -n %{name}-devel
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


%files -n %{name}-progs
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
