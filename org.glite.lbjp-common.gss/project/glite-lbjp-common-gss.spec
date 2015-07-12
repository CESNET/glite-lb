%global gssapi_provider_kerberos %{?gssapi_provider_kerberos:0}

Name:           glite-lbjp-common-gss
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Libraries
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lbjp-common.gss/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  c-ares-devel
BuildRequires:  cppunit-devel
%if %{?gssapi_provider_kerberos:0}
BuildRequires:  globus-common-devel
BuildRequires:  krb5-devel
%else
BuildRequires:  globus-gssapi-gsi-devel
%endif
BuildRequires:  libtool
BuildRequires:  openssl-devel
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
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
./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi
if [ "%gssapi_provider_kerberos" == "1" ]; then
    echo "gssapi_provider=kerberos" >> Makefile.inc
    echo "GLOBUS_COMMON_CFLAGS=`pkg-config --cflags globus-common`" >> Makefile.inc
    echo "GLOBUS_COMMON_LIBS=`pkg-config --libs globus-common`" >> Makefile.inc
fi

CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
rm -f %{buildroot}%{_libdir}/*.a
rm -f %{buildroot}%{_libdir}/*.la


%clean
rm -rf %{buildroot}


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%{!?_licensedir:%global license %doc}
%defattr(-,root,root)
%doc ChangeLog
%license LICENSE
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
