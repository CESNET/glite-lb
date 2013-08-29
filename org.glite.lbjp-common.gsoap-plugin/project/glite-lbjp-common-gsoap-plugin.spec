Name:           glite-lbjp-common-gsoap-plugin
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Libraries
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lbjp-common.gsoap-plugin/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  c-ares-devel
BuildRequires:  cppunit-devel
BuildRequires:  chrpath
# gssapi is needed explicitly for gsoap-plugin, but the proper package is
# known only in glite-lbjp-common-gss-devel:
#  - gssapi from Globus (globus-gssapi-gsi-devel)
#  - gssapi from MIT Kerberos (krb5-devel)
#  - gssapi from Heimdal Kerberos
#BuildRequires: globus-gssapi-gsi-devel
BuildRequires:  gsoap-devel
BuildRequires:  glite-lbjp-common-gss-devel
BuildRequires:  libtool
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
BuildRequires:  pkgconfig
Obsoletes:      glite-security-gsoap-plugin%{?_isa} < 2.0.1-1

%description
@DESCRIPTION@


%package        devel
Summary:        Development files for gLite gsoap-plugin
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       glite-lbjp-common-gss-devel%{?_isa}
Provides:       glite-security-gsoap-plugin%{?_isa} = %{version}-%{release}
Obsoletes:      glite-security-gsoap-plugin%{?_isa} < 2.0.1-1

%description    devel
This package contains development libraries and header files for gLite
gsoap-plugin.


%prep
%setup -q


%build
perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi --module lbjp-common.gsoap-plugin
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make %{?_smp_mflags}


%check
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make check


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
rm -f $RPM_BUILD_ROOT%{_libdir}/*.a
rm -f $RPM_BUILD_ROOT%{_libdir}/*.la
find $RPM_BUILD_ROOT -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'


%clean
rm -rf $RPM_BUILD_ROOT


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%defattr(-,root,root)
%doc project/ChangeLog LICENSE
%{_libdir}/libglite_security_gsoap_plugin_*.so.9
%{_libdir}/libglite_security_gsoap_plugin_*.so.9.*

%files devel
%defattr(-,root,root)
%dir %{_includedir}/glite
%dir %{_includedir}/glite/security
%{_includedir}/glite/security/glite_gscompat.h
%{_includedir}/glite/security/glite_gsplugin.h
%{_includedir}/glite/security/glite_gsplugin-int.h
%{_libdir}/libglite_security_gsoap_plugin_*.so
%{_libdir}/pkgconfig/*.pc


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
