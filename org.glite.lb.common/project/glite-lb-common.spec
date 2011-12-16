Summary: Virtual package for development with gLite L&B common library
Name: glite-lb-common
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: Apache Software License
Vendor: EMI
Group: Development/Libraries
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
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.common/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
This is a virtual package providing runtime and development files for gLite
L&B common library.


%package -n lib%{name}
Summary: @SUMMARY@
Group: System Environment/Libraries


%description -n lib%{name}
@DESCRIPTION@


%package -n %{name}-devel
Summary: Development files for gLite L&B common library
Group: Development/Libraries
Requires: lib%{name}%{?_isa} = %{version}-%{release}
Requires: glite-jobid-api-c-devel
Requires: glite-lbjp-common-gss-devel
Provides: %{name}%{?_isa} = %{version}-%{release}


%description -n %{name}-devel
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


%post -n lib%{name} -p /sbin/ldconfig


%postun -n lib%{name} -p /sbin/ldconfig


%files -n lib%{name}
%defattr(-,root,root)
%dir /usr/share/doc/%{name}-%{version}/
/usr/share/doc/%{name}-%{version}/LICENSE
/usr/share/doc/%{name}-%{version}/ChangeLog
/usr/share/doc/%{name}-%{version}/package.description
/usr/share/doc/%{name}-%{version}/package.summary
/usr/%{_lib}/libglite_lb_common.so.13.@MINOR@.@REVISION@
/usr/%{_lib}/libglite_lb_common.so.13


%files -n %{name}-devel
%defattr(-,root,root)
%dir /usr/include/glite/
%dir /usr/include/glite/lb/
/usr/include/glite/lb/*
/usr/%{_lib}/libglite_lb_common.so


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
