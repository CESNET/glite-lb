Summary: @SUMMARY@
Name: emi-lb-nagios-plugins
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: ASL 2.0
Vendor: EMI
Group: System Environment/Daemons
BuildArch: noarch
Requires: nagios-common
Requires: glite-lb-client-progs
Requires: glite-lb-utils
Requires: glite-lb-ws-test
Requires: globus-proxy-utils
Provides: glite-lb-nagios-plugins = %{name}-%{version}-%{release}
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.nagios/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.nagios
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
%dir /usr/libexec/grid-monitoring/
%dir /usr/libexec/grid-monitoring/probes/
%dir /usr/libexec/grid-monitoring/probes/emi.lb/
%dir /var/lib/grid-monitoring/
%dir %attr(0755, nagios, nagios) /var/lib/grid-monitoring/emi.lb/
/usr/share/doc/%{name}-%{version}/package.summary
/usr/share/doc/%{name}-%{version}/ChangeLog
/usr/share/doc/%{name}-%{version}/package.description
/usr/libexec/grid-monitoring/probes/emi.lb/LB-probe
/usr/libexec/grid-monitoring/probes/emi.lb/IL-probe


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package
