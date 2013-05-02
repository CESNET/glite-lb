# no binaries, only depends on arch-specific library
%global debug_package %{nil}

Name:           glite-jobid-api-cpp
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        Dummy base package for gLite jobid C++ API

Group:          Development/Libraries
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.jobid.api-cpp/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  pkgconfig


%description
This is a dummy package to build gLite jobid C++ API.


%package        devel
Summary:        @SUMMARY@
Group:          Development/Libraries
Requires:       glite-jobid-api-c-devel%{?_isa}
Provides:       %{name} = %{version}-%{release}


%description    devel
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi --module jobid.api-cpp
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make


%check
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files devel
%defattr(-,root,root)
%doc LICENSE project/ChangeLog
%dir %{_includedir}/glite
%dir %{_includedir}/glite/jobid
%{_includedir}/glite/jobid/JobId.h


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
