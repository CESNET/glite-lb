%{!?_pkgdocdir: %global _pkgdocdir %{_docdir}/%{name}-%{version}}

Name:           glite-lb-utils
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Base
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.utils/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  chrpath
BuildRequires:  cppunit-devel
BuildRequires:  glite-lb-types
BuildRequires:  glite-jobid-api-c-devel
BuildRequires:  glite-lb-client-devel
BuildRequires:  glite-lb-state-machine-devel
BuildRequires:  glite-lbjp-common-jp-interface-devel
BuildRequires:  glite-lbjp-common-maildir-devel
BuildRequires:  glite-lbjp-common-trio-devel
BuildRequires:  libtool
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
BuildRequires:  pkgconfig
Requires:       glite-lb-state-machine-plugins

%description
@DESCRIPTION@


%prep
%setup -q


%build
perl ./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --docdir=%{_pkgdocdir} --project=emi
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%doc LICENSE project/ChangeLog doc/README.LB-monitoring doc/README.LB-statistics
%{_bindir}/glite-lb-bkpurge-offline
%{_bindir}/glite-lb-dump
%{_bindir}/glite-lb-dump_exporter
%{_bindir}/glite-lb-load
%{_bindir}/glite-lb-mon
%{_bindir}/glite-lb-purge
%{_bindir}/glite-lb-state_history
%{_bindir}/glite-lb-statistics
%{_mandir}/man1/glite-lb-mon.1*
%{_mandir}/man1/glite-lb-dump.1*
%{_mandir}/man1/glite-lb-load.1*
%{_mandir}/man1/glite-lb-purge.1*


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
