Name:           glite-lb-state-machine
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Libraries
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.state-machine/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  chrpath
BuildRequires:  classads-devel
BuildRequires:  expat-devel
BuildRequires:  glite-jobid-api-c-devel
BuildRequires:  glite-lb-common-devel
BuildRequires:  glite-lb-types
BuildRequires:  glite-lbjp-common-db-devel
BuildRequires:  glite-lbjp-common-gss-devel
BuildRequires:  glite-lbjp-common-jp-interface-devel
BuildRequires:  glite-lbjp-common-trio-devel
BuildRequires:  libtool
BuildRequires:  libxslt
BuildRequires:  pkgconfig

%description
@DESCRIPTION@


%package        devel
Summary:        Development files for gLite L&B state machine
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       glite-lb-common-devel

%description    devel
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
%{_libdir}/libglite_lb_statemachine.so.*
%{_libdir}/glite_lb_plugin.so.*

%files devel
%defattr(-,root,root)
%dir %{_includedir}/glite/
%dir %{_includedir}/glite/lb/
%dir /usr/interface/
%{_libdir}/glite_lb_plugin.so
%{_libdir}/libglite_lb_statemachine.so
%{_includedir}/glite/lb/*.h
/usr/interface/*.xsd


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
