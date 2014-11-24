Name:           glite-lb-ws-test
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Base
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.types/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  chrpath
BuildRequires:  glite-lb-ws-interface
BuildRequires:  glite-lbjp-common-gsoap-plugin-devel
BuildRequires:  gsoap-devel
BuildRequires:  libtool
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
BuildRequires:  pkgconfig

%description
@DESCRIPTION@


%prep
%setup -q


%build
./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make %{?_smp_mflags}


%check
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make check


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
find %{buildroot} -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root)
%doc ChangeLog LICENSE
%dir %{_libdir}/glite-lb/
%dir %{_libdir}/glite-lb/examples/
%{_libdir}/glite-lb/examples/glite-lb-ws_getversion
%{_libdir}/glite-lb/examples/glite-lb-ws_joblog
%{_libdir}/glite-lb/examples/glite-lb-ws_lb4agu_GetActivityStatus
%{_libdir}/glite-lb/examples/glite-lb-ws_jobstat
%{_libdir}/glite-lb/examples/glite-lb-ws_lb4agu_GetActivityInfo


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
