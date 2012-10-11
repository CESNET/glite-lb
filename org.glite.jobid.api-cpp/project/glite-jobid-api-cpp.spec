Name:           glite-jobid-api-cpp
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        Virtual package for development with gLite jobid C++ API

Group:          Development/Libraries
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.jobid.api-c/%{version}/src/%{name}-@VERSION@.src.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  glite-jobid-api-c-devel
BuildRequires:  pkgconfig


%description
This is a virtual package providing runtime and development files for gLite
jobid C++ API.


%package        devel
Summary:        @SUMMARY@
Group:          Development/Libraries
Requires:       glite-jobid-api-c%{?_isa}
Provides:       %{name} = %{version}-%{release}


%description    devel
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module jobid.api-cpp
make


%check
make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*.la' -exec rm -rf {} \;
find $RPM_BUILD_ROOT -name '*.a' -exec rm -rf {} \;


%clean
rm -rf $RPM_BUILD_ROOT


%files devel
%defattr(-,root,root)
%dir /usr/include/glite
%dir /usr/include/glite/jobid
/usr/include/glite/jobid/JobId.h


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
