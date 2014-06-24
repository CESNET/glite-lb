Name:           glite-lb-ws-interface
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          Development/Libraries
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.ws-interface/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildArch:      noarch
BuildRequires:  glite-lb-types
BuildRequires:  libxslt
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
BuildRequires:  tidy

%description
@DESCRIPTION@


%prep
%setup -q


%build
perl ./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi
make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root)
%doc ChangeLog LICENSE
%dir %{_includedir}/glite/
%dir %{_includedir}/glite/lb/
%dir %{_datadir}/wsdl/
%dir %{_datadir}/wsdl/glite-lb/
%{_includedir}/glite/lb/ws_interface_version.h
%{_datadir}/wsdl/glite-lb/LB.wsdl
%{_datadir}/wsdl/glite-lb/glue2.xsd
%{_datadir}/wsdl/glite-lb/lb4agu.wsdl
%{_datadir}/wsdl/glite-lb/LBTypes.wsdl


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package
