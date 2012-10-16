Name:           glite-lb-ws-test
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Base
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.types/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  chrpath
BuildRequires:  gsoap-devel
BuildRequires:  glite-lb-ws-interface
BuildRequires:  glite-lbjp-common-gsoap-plugin-devel
BuildRequires:  libtool
BuildRequires:  pkgconfig
Requires:       glite-lb-ws-interface

%description
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.ws-test
make


%check
make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%dir /usr/%{_lib}/glite-lb/
%dir /usr/%{_lib}/glite-lb/examples/
%{_libdir}/glite-lb/examples/glite-lb-ws_getversion
%{_libdir}/glite-lb/examples/glite-lb-ws_joblog
%{_libdir}/glite-lb/examples/glite-lb-ws_lb4agu_GetActivityStatus
%{_libdir}/glite-lb/examples/glite-lb-ws_jobstat
%{_libdir}/glite-lb/examples/glite-lb-ws_lb4agu_GetActivityInfo


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
