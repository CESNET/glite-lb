Name:           glite-lb-types
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          Development/Tools
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.types/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildArch:      noarch
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
Requires:       perl(:MODULE_COMPAT_%(eval "`%{__perl} -V:version`"; echo $version))

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
%dir %{_datadir}/glite-lb/
%dir %{_datadir}/glite-lb/at3/
%dir %{perl_vendorlib}/
%dir %{perl_vendorlib}/gLite/
%dir %{perl_vendorlib}/gLite/LB/
%dir %{_includedir}/glite/
%dir %{_includedir}/glite/lb/
%{_bindir}/glite-lb-at3
%{_bindir}/glite-lb-check_version.pl
%{_includedir}/glite/lb/*
%{perl_vendorlib}/gLite/LB/StructField.pm
%{perl_vendorlib}/gLite/LB/MultiStruct.pm
%{_datadir}/glite-lb/at3/events.T
%{_datadir}/glite-lb/at3/status.T
%{_datadir}/glite-lb/at3/types.T


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
