Summary: @SUMMARY@
Name: glite-lb-utils
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: ASL 2.0
Vendor: EMI
Group: System Environment/Base
BuildRequires: chrpath
BuildRequires: cppunit-devel
BuildRequires: glite-lb-types
BuildRequires: glite-jobid-api-c-devel
BuildRequires: glite-lb-client-devel
BuildRequires: glite-lb-state-machine-devel
BuildRequires: glite-lbjp-common-jp-interface-devel
BuildRequires: glite-lbjp-common-maildir-devel
BuildRequires: glite-lbjp-common-trio-devel
BuildRequires: libtool
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.utils/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.utils
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
%dir /usr/share/doc/%{name}-%{version}/
/usr/bin/glite-lb-bkpurge-offline
/usr/bin/glite-lb-dump
/usr/bin/glite-lb-dump_exporter
/usr/bin/glite-lb-load
/usr/bin/glite-lb-mon
/usr/bin/glite-lb-purge
/usr/bin/glite-lb-state_history
/usr/bin/glite-lb-statistics
/usr/share/doc/%{name}-%{version}/ChangeLog
/usr/share/doc/%{name}-%{version}/LICENSE
/usr/share/doc/%{name}-%{version}/package.description
/usr/share/doc/%{name}-%{version}/package.summary
/usr/share/doc/%{name}-%{version}/README.LB-monitoring
/usr/share/doc/%{name}-%{version}/README.LB-statistics
/usr/share/man/man1/glite-lb-mon.1.gz
/usr/share/man/man8/glite-lb-purge.8.gz


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
