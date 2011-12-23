Summary: @SUMMARY@
Name: glite-lb-ws-test
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: Apache Software License
Vendor: EMI
Group: System Environment/Base
BuildRequires: chrpath
BuildRequires: gsoap-devel
BuildRequires: glite-lb-ws-interface
BuildRequires: glite-lbjp-common-gsoap-plugin-devel
BuildRequires: libtool
Requires: glite-lb-ws-interface
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.types/%{version}/src/%{name}-@VERSION@.src.tar.gz


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


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%dir /usr/%{_lib}/glite-lb/
%dir /usr/%{_lib}/glite-lb/examples/
/usr/%{_lib}/glite-lb/examples/glite-lb-ws_getversion
/usr/%{_lib}/glite-lb/examples/glite-lb-ws_joblog
/usr/%{_lib}/glite-lb/examples/glite-lb-ws_lb4agu_GetActivityStatus
/usr/%{_lib}/glite-lb/examples/glite-lb-ws_jobstat
/usr/%{_lib}/glite-lb/examples/glite-lb-ws_lb4agu_GetActivityInfo


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
