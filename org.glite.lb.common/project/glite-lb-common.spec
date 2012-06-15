Summary: @SUMMARY@
Name: glite-lb-common
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: ASL 2.0
Vendor: EMI
Group: System Environment/Libraries
BuildRequires: c-ares-devel
BuildRequires: chrpath
BuildRequires: classads
BuildRequires: classads-devel
BuildRequires: cppunit-devel
BuildRequires: expat
BuildRequires: expat-devel
BuildRequires: glite-jobid-api-cpp-devel
BuildRequires: glite-jobid-api-c-devel
BuildRequires: glite-lbjp-common-gss-devel
BuildRequires: glite-lbjp-common-trio-devel
BuildRequires: libtool
BuildRequires: glite-lb-types
BuildRequires: pkgconfig
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.common/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%package devel
Summary: Development files for gLite L&B common library
Group: Development/Libraries
Requires: %{name}%{?_isa} = %{version}-%{release}
Requires: glite-jobid-api-c-devel
Requires: glite-lb-types
Requires: glite-lbjp-common-gss-devel


%description devel
This package contains development libraries and header files for gLite L&B
common library.


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.common
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
/usr/share/doc/%{name}-%{version}/LICENSE
/usr/share/doc/%{name}-%{version}/ChangeLog
/usr/share/doc/%{name}-%{version}/package.description
/usr/share/doc/%{name}-%{version}/package.summary
/usr/%{_lib}/libglite_lb_common.so.13.@MINOR@.@REVISION@
/usr/%{_lib}/libglite_lb_common.so.13


%files devel
%defattr(-,root,root)
%dir /usr/include/glite/
%dir /usr/include/glite/lb/
/usr/include/glite/lb/*
/usr/%{_lib}/libglite_lb_common.so


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package