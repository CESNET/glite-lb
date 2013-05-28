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
BuildArch:      noarch


%description
This is a dummy package to build gLite jobid C++ API.


%package        devel
Summary:        @SUMMARY@
Group:          Development/Libraries
Requires:       glite-jobid-api-c-devel
Provides:       %{name} = %{version}-%{release}


%description    devel
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=%{_prefix} --project=emi --module jobid.api-cpp
make


%check
make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files devel
%defattr(-,root,root)
%doc LICENSE project/ChangeLog
%{_includedir}/glite/jobid/JobId.h


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
