Summary: Virtual package for development with gLite gsoap-plugin
Name: glite-lbjp-common-gsoap-plugin
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: Apache Software License
Vendor: EMI
Group: Development/Libraries
BuildRequires: c-ares-devel
BuildRequires: cppunit-devel
BuildRequires: chrpath
BuildRequires: globus-gssapi-gsi-devel
BuildRequires: gsoap
BuildRequires: gsoap-devel
BuildRequires: glite-lbjp-common-gss-devel
BuildRequires: libtool
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lbjp-common.gsoap-plugin/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
This is a virtual package providing runtime and development files for gLite
gsoap-plugin.


%package -n lib%{name}
Summary: @SUMMARY@
Group: System Environment/Libraries


%description -n lib%{name}
@DESCRIPTION@


%package -n %{name}-devel
Summary: Development files for gLite gsoap-plugin
Group: Development/Libraries
Requires: lib%{name}%{?_isa} = %{version}-%{release}
Requires: glite-lbjp-common-gss-devel
Provides: %{name}%{?_isa} = %{version}-%{release}
Provides: glite-security-gsoap-plugin%{?_isa} = %{version}-%{release}
Obsoletes: glite-security-gsoap-plugin < 2.0.1-1


%description -n %{name}-devel
This package contains development libraries and header files for gLite
gsoap-plugin.


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lbjp-common.gsoap-plugin
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
%dir /usr/share/doc/gsoap-plugin-%{version}
%doc /usr/share/doc/gsoap-plugin-%{version}/LICENSE
/usr/%{_lib}/libglite_security_gsoap_plugin_*.so.9.@MINOR@.@REVISION@
/usr/%{_lib}/libglite_security_gsoap_plugin_*.so.9


%files -n %{name}-devel
%defattr(-,root,root)
%dir /usr/include/glite
%dir /usr/include/glite/security
/usr/include/glite/security/glite_gscompat.h
/usr/include/glite/security/glite_gsplugin.h
/usr/include/glite/security/glite_gsplugin-int.h
/usr/%{_lib}/libglite_security_gsoap_plugin_*.so


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
