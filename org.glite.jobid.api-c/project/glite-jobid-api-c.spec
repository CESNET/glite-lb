Summary: Virtual package for development with gLite jobid C API
Name: glite-jobid-api-c
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: Apache Software License
Vendor: EMI
Group: Development/Libraries
BuildRequires: chrpath
BuildRequires: libtool
BuildRequires: cppunit-devel
BuildRequires: openssl-devel
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.jobid.api-c/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
This is a virtual package providing runtime and development files for gLite
jobid C library.


%package -n lib%{name}
Summary: @SUMMARY@
Group: System Environment/Libraries
Obsoletes: glite-jobid-api-c%{?_isa} <= 2.1.0


%description -n lib%{name}
@DESCRIPTION@


%package -n %{name}-devel
Summary: Development files for gLite jobid C library
Group: Development/Libraries
Requires: lib%{name}%{?_isa} = %{version}-%{release}
Provides: %{name}%{?_isa} = %{version}-%{release}
Obsoletes: glite-jobid-api-c%{?_isa} <= 2.1.0


%description -n %{name}-devel
This package contains development libraries and header files for gLite jobid
C library.


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module jobid.api-c
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
/usr/%{_lib}/libglite_jobid.so.@MAJOR@.@MINOR@.@REVISION@
/usr/%{_lib}/libglite_jobid.so.@MAJOR@


%files -n %{name}-devel
%defattr(-,root,root)
%dir /usr/include/glite
%dir /usr/include/glite/jobid
/usr/include/glite/jobid/strmd5.h
/usr/include/glite/jobid/cjobid.h
/usr/%{_lib}/libglite_jobid.so


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
