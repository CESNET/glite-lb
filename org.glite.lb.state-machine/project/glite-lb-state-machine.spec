Summary: @SUMMARY@
Name: glite-lb-state-machine
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: ASL 2.0
Vendor: EMI
Group: System Environment/Libraries
BuildRequires: chrpath
BuildRequires: classads-devel
BuildRequires: expat-devel
BuildRequires: glite-jobid-api-c-devel
BuildRequires: glite-lb-common-devel
BuildRequires: glite-lb-types
BuildRequires: glite-lbjp-common-db-devel
BuildRequires: glite-lbjp-common-gss-devel
BuildRequires: glite-lbjp-common-jp-interface-devel
BuildRequires: glite-lbjp-common-trio-devel
BuildRequires: libtool
BuildRequires: libxslt
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.state-machine/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%package devel
Summary: Development files for gLite L&B state machine
Group: Development/Libraries
Requires: %{name}%{?_isa} = %{version}-%{release}
Requires: glite-lb-common-devel


%description devel
This package contains development libraries and header files for gLite L&B
state machine.


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.state-machine
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
/usr/%{_lib}/libglite_lb_statemachine.so.@MAJOR@.@MINOR@.@REVISION@
/usr/%{_lib}/libglite_lb_statemachine.so.@MAJOR@
/usr/%{_lib}/glite_lb_plugin.so.0
/usr/%{_lib}/glite_lb_plugin.so.0.0.0


%files devel
%defattr(-,root,root)
%dir /usr/include/glite/
%dir /usr/include/glite/lb/
%dir /usr/interface/
/usr/%{_lib}/glite_lb_plugin.so
/usr/%{_lib}/libglite_lb_statemachine.so
/usr/include/glite/lb/*.h
/usr/interface/*.xsd


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
