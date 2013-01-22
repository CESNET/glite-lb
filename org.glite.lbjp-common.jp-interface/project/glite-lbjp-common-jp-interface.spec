Name:           glite-lbjp-common-jp-interface
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Libraries
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lbjp-common.jp-interface/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  chrpath
BuildRequires:  cppunit-devel
BuildRequires:  glite-jobid-api-c-devel
BuildRequires:  glite-lbjp-common-db-devel
BuildRequires:  glite-lbjp-common-trio-devel
BuildRequires:  libtool
BuildRequires:  pkgconfig

%description
@DESCRIPTION@


%package        devel
Summary:        Development files for gLite L&B/JP interface library
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description    devel
This package contains development libraries and header files for gLite L&B/JP
interface library.


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi --module lbjp-common.jp-interface
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make


%check
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make check


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
%doc LICENSE project/ChangeLog
%{_libdir}/libglite_jp_common.so.2
%{_libdir}/libglite_jp_common.so.2.*

%files devel
%defattr(-,root,root)
%dir %{_includedir}/glite
%dir %{_includedir}/glite/jp
%{_includedir}/glite/jp/types.h
%{_includedir}/glite/jp/indexdb.h
%{_includedir}/glite/jp/attr.h
%{_includedir}/glite/jp/file_plugin.h
%{_includedir}/glite/jp/context.h
%{_includedir}/glite/jp/type_plugin.h
%{_includedir}/glite/jp/backend.h
%{_includedir}/glite/jp/known_attr.h
%{_includedir}/glite/jp/builtin_plugins.h
%{_libdir}/libglite_jp_common.so


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
