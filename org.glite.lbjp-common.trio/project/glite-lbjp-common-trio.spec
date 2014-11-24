Name:           glite-lbjp-common-trio
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Libraries
# Created under ASL 2.0 license.
# Core files taken from http://daniel.haxx.se/projects/trio/ are under MIT-like
# license (a variant identical to ISC): interface/trio.h, src/strio.c,
# src/strio.h, src/trio.c, and src/triop.h.
License:        ASL 2.0 and ISC
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lbjp-common.trio/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  cppunit-devel
BuildRequires:  libtool
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
BuildRequires:  pkgconfig

%description
@DESCRIPTION@


%package        devel
Summary:        Development files for gLite L&B/JP common trio library
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description    devel
This package contains development libraries and header files for gLite L&B/JP
common trio library.


%prep
%setup -q


%build
./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make %{?_smp_mflags}


%check
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make check


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
%defattr(-,root,root)
%doc ChangeLog LICENSE
%{_libdir}/libglite_lbu_trio.so.2
%{_libdir}/libglite_lbu_trio.so.2.*

%files devel
%defattr(-,root,root)
%dir %{_includedir}/glite
%dir %{_includedir}/glite/lbu
%{_includedir}/glite/lbu/escape.h
%{_includedir}/glite/lbu/trio.h
%{_libdir}/libglite_lbu_trio.so


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
