%global gssapi_provider_kerberos %{?gssapi_provider_kerberos:0}

Name:           glite-lbjp-common-gss
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Libraries
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lbjp-common.gss/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  c-ares-devel%{?_isa}
BuildRequires:  cppunit-devel%{?_isa}
%if %{?gssapi_provider_kerberos:0}
BuildRequires:  globus-common-devel%{?_isa}
BuildRequires:  krb5-devel%{?_isa}
%else
BuildRequires:  globus-gssapi-gsi-devel%{?_isa}
%endif
BuildRequires:  libtool
BuildRequires:  openssl-devel%{?_isa}
BuildRequires:  pkgconfig
Obsoletes:      glite-security-gss%{?_isa} < 2.1.5-1

%description
@DESCRIPTION@


%package        devel
Summary:        Development files for gLite GSS library
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}-%{release}
%if %{?gssapi_provider_kerberos:0}
Requires:       globus-common-devel%{?_isa}
Requires:       krb5-devel%{?_isa}
%else
Requires:       globus-gssapi-gsi-devel%{?_isa}
%endif
Requires:       pkgconfig
Provides:       glite-security-gss%{?_isa} = %{version}-%{release}
Obsoletes:      glite-security-gss%{?_isa} < 2.1.5-1

%description    devel
This package contains development libraries and header files for gLite GSS
library.


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi --module lbjp-common.gss
if [ "%gssapi_provider_kerberos" == "1" ]; then
    echo "gssapi_provider=kerberos" >> Makefile.inc
    echo "GLOBUS_COMMON_CFLAGS=`pkg-config --cflags globus-common`" >> Makefile.inc
    echo "GLOBUS_COMMON_LIBS=`pkg-config --libs globus-common`" >> Makefile.inc
fi

CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make


%check
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*.la' -exec rm -rf {} \;
find $RPM_BUILD_ROOT -name '*.a' -exec rm -rf {} \;

%clean
rm -rf $RPM_BUILD_ROOT


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%defattr(-,root,root)
%doc project/ChangeLog LICENSE
%{_libdir}/libglite_security_gss.so.9
%{_libdir}/libglite_security_gss.so.9.*

%files devel
%defattr(-,root,root)
%dir %{_includedir}/glite
%dir %{_includedir}/glite/security
%{_includedir}/glite/security/glite_gss.h
%{_libdir}/libglite_security_gss.so
%{_libdir}/pkgconfig/*.pc


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
