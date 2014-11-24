Name:           glite-lb-state-machine
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Libraries
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.state-machine/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  chrpath
%if 0%{?rhel} <= 6 && ! 0%{?fedora}
BuildRequires:  classads-devel
%else
BuildRequires:  condor-classads-devel
%endif
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
This package contains L&B state machine plugins for L&B tools and Job Provenance.


%prep
%setup -q


%build
./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
rm -f %{buildroot}%{_libdir}/*.a
rm -f %{buildroot}%{_libdir}/*.la
find %{buildroot} -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'


%clean
rm -rf %{buildroot}


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%defattr(-,root,root)
%doc ChangeLog LICENSE
%{_libdir}/libglite_lb_statemachine.so.6
%{_libdir}/libglite_lb_statemachine.so.6.*

%files devel
%defattr(-,root,root)
%dir %{_datadir}/glite-lb/xsd/
%{_includedir}/glite/lb/*.h
%{_libdir}/libglite_lb_statemachine.so
%{_datadir}/glite-lb/xsd/*.xsd

%files plugins
%defattr(-,root,root)
%{_libdir}/glite_lb_plugin.so
%{_libdir}/glite_lb_plugin.so.*


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
