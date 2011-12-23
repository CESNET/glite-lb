Summary: @SUMMARY@
Name: glite-lbjp-common-gss
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: Apache Software License
Vendor: EMI
Group: System Environment/Libraries
BuildRequires: c-ares-devel
BuildRequires: c-ares
BuildRequires: chrpath
BuildRequires: cppunit-devel
BuildRequires: globus-gssapi-gsi-devel
BuildRequires: libtool
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
Obsoletes: glite-security-gss%{?_isa} < 2.1.5-1
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lbjp-common.gss/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%package devel
Summary: Development files for gLite GSS library
Group: Development/Libraries
Requires: %{name}%{?_isa} = %{version}-%{release}
Requires: globus-gssapi-gsi-devel
Provides: glite-security-gss%{?_isa} = %{version}-%{release}
Obsoletes: glite-security-gss%{?_isa} < 2.1.5-1


%description devel
This package contains development libraries and header files for gLite GSS
library.


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lbjp-common.gss
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
%dir /usr/share/doc/gss-%{version}
%doc /usr/share/doc/gss-%{version}/LICENSE
/usr/%{_lib}/libglite_security_gss.so.9.@MINOR@.@REVISION@
/usr/%{_lib}/libglite_security_gss.so.9


%files devel
%defattr(-,root,root)
%dir /usr/include/glite
%dir /usr/include/glite/security
/usr/include/glite/security/glite_gss.h
/usr/%{_lib}/libglite_security_gss.so


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
