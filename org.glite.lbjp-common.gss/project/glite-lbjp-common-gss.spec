%global gssapi_provider_kerberos %{?gssapi_provider_kerberos:0}

Name:           glite-lbjp-common-gss
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Libraries
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lbjp-common.gss/%{version}/src/%{name}-@VERSION@.src.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  c-ares-devel
BuildRequires:  c-ares
BuildRequires:  chrpath
BuildRequires:  cppunit-devel
%if %gssapi_provider_kerberos
BuildRequires:  globus-common-devel
BuildRequires:  krb5-devel
%else
BuildRequires:  globus-gssapi-gsi-devel
%endif
BuildRequires:  libtool
BuildRequires:  openssl-devel
BuildRequires:  pkgconfig
Obsoletes:      glite-security-gss%{?_isa} < 2.1.5-1

%description
@DESCRIPTION@


%package        devel
Summary:        Development files for gLite GSS library
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}-%{release}
%if %gssapi_provider_kerberos
Requires:       globus-common-devel
Requires:       krb5-devel
%else
Requires:       globus-gssapi-gsi-devel
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
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lbjp-common.gss
if [ "%gssapi_provider_kerberos" == "1" ]; then
	echo Kerberos
	echo "gssapi_provider=kerberos" >> Makefile.inc
	echo "GLOBUS_COMMON_CFLAGS=`pkg-config --cflags globus-common`" >> Makefile.inc
	echo "GLOBUS_COMMON_LIBS=`pkg-config --libs globus-common`" >> Makefile.inc
fi

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
%dir /usr/share/doc/%{name}-%{version}
%doc /usr/share/doc/%{name}-%{version}/LICENSE
/usr/%{_lib}/libglite_security_gss.so.9.@MINOR@.@REVISION@
/usr/%{_lib}/libglite_security_gss.so.9

%files devel
%defattr(-,root,root)
%dir /usr/include/glite
%dir /usr/include/glite/security
/usr/include/glite/security/glite_gss.h
/usr/%{_lib}/libglite_security_gss.so
/usr/%{_lib}/pkgconfig/*.pc


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
