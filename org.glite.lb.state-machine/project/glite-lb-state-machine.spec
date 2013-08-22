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
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
BuildRequires:  pkgconfig

%description
@DESCRIPTION@


%package        devel
Summary:        Development files for gLite L&B state machine
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       glite-lb-common-devel%{?_isa}

%description    devel
This package contains development libraries and header files for gLite L&B
state machine.


%package        plugins
Summary:        L&B state machine plugins
Group:          System Environment/Libraries

%description    plugins
This package contains L&B state machine plugins for Job Provenance.


%prep
%setup -q


%build
perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi --module lb.state-machine
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make


%install
rm -rf $RPM_BUILD_ROOT
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
%{_libdir}/libglite_lb_statemachine.so.6
%{_libdir}/libglite_lb_statemachine.so.6.*

%files devel
%defattr(-,root,root)
%dir %{_includedir}/glite/
%dir %{_includedir}/glite/lb/
%dir /usr/interface/
%{_libdir}/libglite_lb_statemachine.so
%{_includedir}/glite/lb/*.h
/usr/interface/*.xsd

%files plugins
%defattr(-,root,root)
%{_libdir}/glite_lb_plugin.so
%{_libdir}/glite_lb_plugin.so.*


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
