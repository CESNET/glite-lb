Name:           emi-lb-nagios-plugins
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Daemons
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.nagios/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildArch:      noarch
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
Requires:       nagios-common
Requires:       glite-lb-client-progs
Requires:       glite-lb-utils
Requires:       glite-lb-ws-test
Requires:       globus-proxy-utils
Provides:       glite-lb-nagios-plugins = %{version}-%{release}

%description
@DESCRIPTION@


%prep
%setup -q


%build
./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi
make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}


%clean
rm -rf %{buildroot}


%files
%{!?_licensedir:%global license %doc}
%defattr(-,root,root)
%doc ChangeLog
%license LICENSE
%dir %{_libexecdir}/grid-monitoring/
%dir %{_libexecdir}/grid-monitoring/probes/
%dir %{_libexecdir}/grid-monitoring/probes/emi.lb/
%dir %{_localstatedir}/lib/grid-monitoring/
%dir %attr(0755, nagios, nagios) %{_localstatedir}/lib/grid-monitoring/emi.lb/
%{_libexecdir}/grid-monitoring/probes/emi.lb/LB-probe
%{_libexecdir}/grid-monitoring/probes/emi.lb/IL-probe


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
